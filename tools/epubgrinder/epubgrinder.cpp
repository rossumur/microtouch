#include "epubgrinder.h"
#include "zipfile.h"
#include "builder.h"

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>

#include <QtGui>

string toString(wstring w)
{
    string s;
    s.resize(w.length());
    for (int i = 0; i < w.length(); i++)
        s[i] = w[i];
    return s;
}

wstring toWString(string s)
{
    wstring w;
    w.resize(s.length());
    for (int i = 0; i < s.length(); i++)
        w[i] = s[i];
    return w;
}

string toString(int i)
{
    char buf[16];
    sprintf(buf,"%d",i);
    return buf;
}

template<typename T>
inline T left_trim(const T& src, const T& to_trim)
{
    if(!src.length())
        return src;
    return src.substr(src.find_first_not_of(to_trim));
}

template<typename T>
inline T right_trim(const T& src, const T& to_trim)
{
    if(!src.length())
        return src;
    return src.substr(0, src.find_last_not_of(to_trim) + 1);
}

template<typename T>
inline T trim(const T& src, const T& to_trim) { return right_trim(left_trim(src, to_trim), to_trim); }

Sax::~Sax()
{
}

void Sax::startDocument()
{
}

bool Sax::endDocument()
{
    return true;
}

void Sax::start(const string& tag, map<string,string>& attr)
{
}

void Sax::end(const string& tag)
{
}

void Sax::text(const wstring& text)
{
}

void Sax::whitespace(const wstring& w)
{
}

class OpfLoader : public Sax
{
    string _metadataTag;
    wstring _metadataValue;
    Builder& _builder;

public:
    OpfLoader(Builder& b) : _builder(b)
    {
    }
    virtual void start(const string& tag, map<string,string>& attr)
    {
        if (tag == "item")
        {
            ManifestItem item;
            item.href = attr["href"];
            item.mediaType = attr["media-type"];
            _builder.Manifest[attr["id"]] = item;
        }
        else if (tag == "itemref")
        {
            SpineItem item;
            item.idref = attr["idref"];
            item.linear = attr["linear"];
            _builder.Spine.push_back(item);
        }
        else if (tag.substr(0,3) == "dc:")
        {
            _metadataTag = tag;
        }
    }

    virtual void end(const string& tag)
    {
        if (tag == _metadataTag)
        {
            _builder.Metadata[_metadataTag] = _metadataValue;
            _metadataTag.clear();
            _metadataValue.clear();
        }
    }

    void addText(const wstring& t)
    {
        if (_metadataTag.empty())
            return;
        _metadataValue += t;
    }

    virtual void text(const wstring& text)
    {
        addText(text);
    }

    virtual void whitespace(const wstring& w)
    {
        addText(w);
    }
};

class NcxLoader : public Sax
{
    wstring _text;
    string _src;
    string _playOrder;
    Builder& _builder;
public:
    NcxLoader(Builder& b) : _builder(b)
    {
    }

    virtual void start(const string& tag, map<string,string>& attr)
    {
        if (tag == "navPoint")
        {
            _src.clear();
            _text.clear();
            _playOrder = attr["playOrder"];
        }
        else if (tag == "content")
        {
            _src = attr["src"];
        }
    }

    virtual void end(const string& tag)
    {
        if (tag == "navPoint")
        {
            NavpointItem n = { trim(_text,wstring(L"\r\n\t ")),_src };
            _builder.Navpoints[_playOrder] = n;
        }
    }

    virtual void text(const wstring& text)
    {
        _text += text;
    }
};


class MetaLoader : public Sax
{
    Builder& _builder;
public:
    string Path;
    MetaLoader(Builder& b) : _builder(b)
    {
    }

    virtual void start(const string& tag, map<string,string>& attr)
    {
        if (tag == "rootfile")
            Path = attr["full-path"];
    }
};

EpubGrinder::EpubGrinder() : _file(0)
{
}

EpubGrinder::~EpubGrinder()
{
    if (_file)
        fclose(_file);
}

u8* EpubGrinder::readAll(const string& name, int& len)
{
    len = 0;
    const char* path = name.c_str();    // BUGBUG
    for (int i = 0; i < _files.size(); i++)
    {
        if (strcasecmp(path,_files[i].name.c_str()) == 0)
        {
            len = _files[i].info.uncompressedSize;
            return ZipDecompress(_file,_files[i]);
        }
    }
    printf("### can't find %s\n",path);
    return 0;
}

bool EpubGrinder::loadXml(const string& path, Sax& b)
{
    int len;
    u8* d = readAll(path,len);
    if (!d)
    {
        printf(string("Failed to open " + path + "\n").c_str());
        return false;
    }
    printf("applyXml %s : %d bytes\n",path.c_str(),len);
    bool result = applyXml((const char*)d,len,b);
    delete [] d;
    return result;
}

void EpubGrinder::grind(const string& path, const string& out)
{
    _file = fopen(path.c_str(),"rb");
    ZipInfo(_file,_files);

    Builder b;

    // Load root path
    MetaLoader meta(b);
    loadXml("META-INF/container.xml",meta);

    // Load toplevel opf info
    OpfLoader opf(b);
    loadXml(meta.Path,opf);
    int clip = meta.Path.find_last_of('/');
    b.Path = meta.Path.substr(0,clip+1);

    // Load ncx (navigation control)
    NcxLoader ncx(b);
    loadXml(b.pathById("ncx"),ncx);

    // Apply images
    map<string,ManifestItem>::iterator it;
    for (it = b.Manifest.begin(); it != b.Manifest.end();++it)
    {
        ManifestItem& item = it->second;
        if (item.mediaType.find("image") != -1)
        {
            int len;
            u8* d = readAll(b.Path + item.href,len);
            b.addImage(item.href,d,len);
        }
    }

    // Load pages in order
    // Apply based on spine rather than navpoint
    b.startBook();
#if 0
    for (int i = 0; i < b.Navpoints.size(); i++)
    {
        NavpointItem item = b.Navpoints[toString(i+1)];
        b.startNavpoint(item);
        loadXml(b.Path + item.src,b);
    }
#endif
    for (int i = 0; i < b.Spine.size(); i++)
    {
        string url = b.Manifest[b.Spine[i].idref].href;
        NavpointItem item;
        item.src = url;
        b.startNavpoint(item);
        loadXml(b.Path + url,b);
    }
    b.endBook(out);
}
