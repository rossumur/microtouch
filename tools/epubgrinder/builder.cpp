#include "builder.h"

//===========================================================================
//===========================================================================
/*
    General Structure
    epb
        book
        book
            info - displayed in scrolling view
            toc - cover (if present) and table of contents
            chapter
            chapter
                spatialIndex
                text
            chapter
            glyphs:id = 8000000
            meta:id = 80000001  etc
        book
        book
*/

Builder::Builder()
{
}

string Builder::pathById(const string& id)
{
    return Path + Manifest[id].href;
}

//  called before startDocument on each navpoint
void Builder::startNavpoint(const NavpointItem& item)
{
    _layout.startUrl(item.src);
}

void Builder::startDocument()
{
    _layout.startDocument();
}

//  end of navpoint
bool Builder::endDocument()
{
    _layout.endDocument();
    return true;
}

void Builder::addImage(const string& path, const u8* data, int len)
{
    _layout.loadImage(path,data,len);
}


wstring& replaceAll(
  wstring& result,
  const wstring& replaceWhat,
  const wstring& replaceWithWhat)
{
  int pos = 0;
  while(1)
  {
    pos = result.find(replaceWhat,pos);
    if (pos==-1) break;
    result.replace(pos,replaceWhat.size(),replaceWithWhat);
    pos += replaceWithWhat.size();
  }
  return result;
}

int MapUnicode(int);    // In layout.cpp
wstring xmlEscape(const wstring& w)
{
    wstring s = w;
    for (int i = 0; i < s.length(); i++)    // Map unicode to our crappy charset.. '—' 8212 '’' 8217
        s[i] = MapUnicode(s[i]);

    replaceAll(s,L"&",L"&amp;");
    replaceAll(s,L"<",L"&lt;");
    replaceAll(s,L">",L"&gt;");
    replaceAll(s,L"'",L"&apos;");
    replaceAll(s,L"\"",L"&quot;");
    return s;
}

void Builder::writeBadge()
{
    _layout.startBadge();

    wstring title = Metadata["dc:title"];
    wstring author = Metadata["dc:creator"];

    printf("T:%s A:%s\n",toString(title).c_str(),toString(author).c_str());

    wstring b = L"<html><body><b>";
    b += xmlEscape(title);
    b += L"</b><br/>";
    b += xmlEscape(author);
    b += L"</body></html>";

    applyXml(b,*this);
    _layout.endBadge();
}

void Builder::writeToc()
{
    if (Navpoints.size() <= 1)
        return;

    wstring title = Metadata["dc:title"];
    wstring toc = L"<html><body>";
    toc += L"<h3>"+title+L"</h3><br/><br/>";
    toc += L"<h4>Table of Contents</h4><br/>";

    for (int i = 0; i < Navpoints.size(); i++)
    {
        NavpointItem& n = Navpoints[toString(i+1)];
        printf("Nav %s %s\n",toString(n.text).c_str(),n.src.c_str());
        toc += L"<a href='" + xmlEscape(toWString(n.src)) + L"'><h4>" + xmlEscape(n.text) + L"</h4></a>";
    }
    toc += L"</body></html>";
    NavpointItem item;
    startNavpoint(item);
    applyXml(toc,*this);
}

void Builder::writeMeta()
{
}

void Builder::writeGlyphs()
{
}

void Builder::startBook()
{
    _layout.startBook(_root);
    writeBadge();
    writeToc();
}

void Builder::endBook(const string& dst)
{
    _layout.endBook();
    writeGlyphs();
    writeMeta();

    _root.toFile(dst.c_str());
}

void Builder::start(const string& tag, map<string,string>& attr)
{
    _layout.start(tag,attr);
}

void Builder::end(const string& tag)
{
    _layout.end(tag);
}

void Builder::text(const wstring& text)
{
    _layout.text(text);
}

void Builder::whitespace(const wstring& w)
{
    _layout.whitespace(w);
}
