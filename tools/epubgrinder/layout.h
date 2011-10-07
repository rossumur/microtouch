#ifndef LAYOUT_H
#define LAYOUT_H

#include "epubgrinder.h"
#include "blobfile.h"

#include <QtGui>

class Font
{
    ByteArray _f;
public:
    ByteArray& data();
    int height();
    int ascent();
    int descent();
    int measure(const wchar_t* t, int len);
    int measure(wchar_t c);
    bool load(const char* f);
};

class SIndex
{
public:
    int y;
    int mark;
};

typedef struct {
    u8 leftMargin;
    u8 rightMargin;
    u8 lineHeight;
    u8 descent;
    u8 font;
    u8 r0;
    u8 r1;
    u8 r2;
    u16 pageSize;
    u16 pageStart;
    u16 pageCount;
    u16 flags;
} PageInfo;

class EPubBook
{
    vector<SIndex> _sindex;
    ByteArray _data;
    int _style;
    int _link;

    BlobFile _navPoints;
    BlobFile* _root;
    PageInfo _pageInfo;

    int _pageCount;
    int _pageMark;

public:
    EPubBook();
    void startBook(BlobFile& root);
    void endBook(BlobFile& images,BlobFile&links);
    int navPointCount();
    PageInfo& pageInfo();

    void startDocument();
    void endDocument();
    void compress();

    void startLine(int y, int height, int descent=-1);
    void endLine();
    void padY(int height);

    void pageNum(int y, int dy, int dx, const wstring& s);
    void pageBreak(int y);
    void fullPage(int y);
    void updateSpatialIndex(int y);

    void setLink(int link);
    void setStyle(int style);
    void write(const wchar_t* t, int len);
    void write(int token);
    void writeId(u32 id);
    void writeImageRef(u32 id, u32 yoffset);
};

class Glyph
{
public:
    Glyph();
    virtual ~Glyph();
    int x;
    int width;
    int height;
    int descent;
    int style;
    int link;

    virtual void write(EPubBook& doc);
};

class TextGlyph : public Glyph, public wstring
{
public:
    virtual void write(EPubBook& doc);
};

class PadGlyph : public Glyph
{
public:
    virtual void write(EPubBook& doc);
};

class ImageGlyph : public Glyph
{
public:
    int id;
    int yoffset;
    virtual void write(EPubBook &doc);
};

class Layout : public Sax
{
    map<string,QImage> _images;
    map<string,int> _imageHash;
    map<int,string> _imageUsed;
    map<string,int> _imageIdMap;
    vector<string> _imageList;
    int _imageId;
    string _cover;

    vector<int> _stack;
    enum State {
        InBody = 1,
        StripWhitespace = 2,
        HidePageNum = 4,
        InScript = 8
    };
    int _state;

    vector<Glyph*> _line;
    TextGlyph* _text;
    int _style;
    int _link;
    map<string,string> _currentLinkAttr;

    int _x;
    int _y;
    int _left;
    int _right;
    int _lastChar;

    string _currentUrl;
    map<string,int> _links;
    vector<string>  _linksList;
    map<string,int> _fragments;

    EPubBook _document;

public:
    Layout();
    void clear();
    void clearLine();
    void loadImage(const string& path, const u8* data, int len);
    int getImageId(const string& url);

    int measure(int style, const wchar_t* t, int len, int& height);

    void padY(int y);
    void addPage(bool lastPage = false);
    void addLine(vector<Glyph*>& line, int height, int descent = -1);
    void addLine();
    void addWord();
    void addChar(int c);

    void addGlyph(Glyph* g);
    void addImage(const string& url);
    void addImage(map<string,string>& attr);

    int remainingSpaceOnPage();
    bool atStartOfPage();
    bool atLeft();
    void padX(int width);
    void newLine();
    void blankLine();
    void flushLine();
    void flushPage(bool last);

    void setStyle(int style);
    int toStyle(int flags);

    void startBook(BlobFile& root);
    void startBadge();
    void endBadge();
    void endBook();

    void startUrl(const string& href);
    int addLink(map<string,string>& attr);
    void endLink();
    void mapLink(const string& href,ByteArray& data);
    string absoluteUrl(string url);

    virtual void startDocument();
    virtual bool endDocument();
    virtual void start(const string& tag, map<string,string>& attr);
    virtual void end(const string& tag);
    virtual void text(const wstring& text);
    virtual void whitespace(const wstring& w);
};

#endif // LAYOUT_H
