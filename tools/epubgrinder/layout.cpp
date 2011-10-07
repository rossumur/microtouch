#include "layout.h"

int MapUnicode(int c);

bool loadResouce(QString p, ByteArray& dst)
{
    QResource r(p);
    dst.resize(r.size());
    memcpy(&dst[0],r.data(),r.size());
    return r.size() > 0;
}

void defaultCover(ByteArray& dst)
{
    QResource r(":images/defaultCover.jpg");
    dst.resize(r.size());
    memcpy(&dst[0],r.data(),r.size());
}

void WriteId(u32 id, ByteArray& data)
{
    int i = 0;
    int rev[5];
    while (id > 127)
    {
        rev[i++] = id & 0x7F;
        id >>= 7;
    }
    rev[i++] = id;
    while (i > 0)
    {
        --i;
        data.write(rev[i] | (i == 0 ? 0 : 0x80));
    }
}

//================================================================================
//================================================================================

typedef struct
{
    u8 format;
    u8 top;
    u16 width;
    u16 height;
} GlyphHdr __attribute__((__packed__));

int alpha(int c, int a)
{
    return (c*a + (0xFF-a)*0xFF) >> 8;
}


static long randomBits(u8 bits)
{
    static long _lfsr = 0x8BADF00D;
    long t = bits;
    while (bits--)
    {
        t = _lfsr << 1;
        if (t <= 0)
            t ^= 0x20AA95B5;
        _lfsr = t;
    }
    return t;
}

int pin(int n)
{
    if (n < 0) return 0;
    if (n > 255) return 255;
    return n;
}

short RGBTo16(int rgb, int x, int y)
{
    int a = (u8)(rgb >> 24);
    if (a == 0)
        return 0xFFFF;
    int r = (u8)(rgb >> 16);
    int g = (u8)(rgb >> 8);
    int b = (u8)rgb;
    if (a != 0xFF)
    {
        r = alpha(r,a);
        g = alpha(g,a);
        b = alpha(b,a);
    }
    r = pin(r + (randomBits(3) & 7)) & 0xF8;
    g = pin(g + (randomBits(2) & 3)) & 0xFC;
    b = pin(b + (randomBits(3) & 7)) & 0xF8;
    return (r << 8) | (g << 3) | (b >> 3);
}

void ConvertTo16(QImage& img, ByteArray& data)
{
    QImage image = img.convertToFormat(QImage::Format_ARGB32);
    int width = image.width();
    int height = image.height();

    data.resize(sizeof(GlyphHdr) + width*height*2);
    GlyphHdr* hdr = (GlyphHdr*)&data[0];
    hdr->format = 'g';
    hdr->top = 0;
    hdr->width = width;
    hdr->height = height;
    u8* dst = &data[sizeof(GlyphHdr)];
    for (int y = 0; y < height; y++)
    {
        int* src = (int*)(image.bits() + image.bytesPerLine()*y);
        for (int x = 0; x < width; x++)
        {
            short p = RGBTo16(src[x],x,y);
            dst[(x << 1)+0] = p >> 8;
            dst[(x << 1)+1] = p;
        }
        dst += width*2;
    }
}

//================================================================================
//================================================================================
//	Global tokens

#define TOK(_n,_f) T_##_n
enum Token
{
        T_UNKNOWN,
        #include "Tokens.h"
        TOK(END,0)
};
#define T_COUNT (T_END-1)

#undef TOK
#define TOK(_n) #_n
#define TOK(_n,_f) #_n
const char* _tokens[T_COUNT+1]=
{
        #include "Tokens.h"
        0
};

Token Tokenize(const char* s, const char** tokens, int count)
{
        int begin = 0;
        int end = count-1;
        while (begin <= end)
        {
                int i = (begin + end) >> 1;
                int cmp = strcmp(tokens[i], s);
                if (cmp == 0)
                        return (Token)(i+1);
                if(cmp < 0)
                        begin = i + 1;
                else
                        end = i - 1;
        }
        return T_UNKNOWN;		// not found
}

Token Tokenize(const char* s)
{
        return Tokenize(s,_tokens,T_COUNT);
}


//	Flags defining tokens
#define F_BLOCK		0x01
#define	F_PRE		0x08
#define F_BOLD		0x10
#define F_ITALIC	0x20
#define F_SIZE0 FFONT(0)	// Default
#define F_SIZE1 FFONT(1)	// Small
#define F_SIZE2 FFONT(2)	// Medium
#define F_SIZE3 FFONT(3)	// Large


#define FFONT(_s) ((_s) << 6)

#undef TOK
#define TOK(_n,_f) _f
const u8 _tokenFlags[] =
{
        T_UNKNOWN,
        #include "Tokens.h"
};

ByteArray& Font::data()
{
    return _f;
}

int Font::height()
{
    return _f[3];
}

int Font::ascent()
{
    return _f[4];
}

int Font::descent()
{
    return _f[5];
}

int Font::measure(wchar_t c)
{
    c = MapUnicode(c); // Clumsy mapping to our charset
    int i = c - _f[1];
    if (i < 0 || i >= _f[2])
        return 0;
    i = (i + 2) << 2;
    return _f[i];
}

int Font::measure(const wchar_t* t, int len)
{
    int width = 0;
    while (len--)
        width += measure(*t++);
    return width;
}

bool Font::load(const char* file)
{
    QString p = QString(":") + file;
    return loadResouce(p,_f);
}

const char* _fontList[] = {

    "fonts/Sans8.ctf",
    "fonts/Sans8Bold.ctf",
    "fonts/Sans10.ctf",
    "fonts/Sans10Bold.ctf",
    "fonts/Sans12.ctf",
    "fonts/Sans12Bold.ctf",
    0
};

class Fonts
{
    vector<Font> _fonts;
public:
    Fonts()
    {
        const char* path;
        for (int i = 0; (path = _fontList[i]); i++)
        {
            Font f;
            f.load(path);
            _fonts.push_back(f);
        }
    }

    int fontIndex(int style)
    {
        if (style == 0)
            return 0;
        int f = style >> 2;  // 0:1:2:3X
        f = min(f,2);

        f <<= 1;
        if (style & 0x01)   // BOLD
            f++;
        return f;
    }

    int ascent(int style)
    {
        return _fonts[fontIndex(style)].ascent();
    }

    int descent(int style)
    {
        return _fonts[fontIndex(style)].descent();
    }

    int height(int style)
    {
        return _fonts[fontIndex(style)].height();
    }

    int measure(int style, const wchar_t* s, int len, int& height)
    {
        Font& f = _fonts[fontIndex(style)];
        height = f.height();
        return f.measure(s,len);
    }

    void toBlob(BlobFile& blob)
    {
        const char* path;
        for (int i = 0; (path = _fontList[i]); i++)
        {
            blob.add(_fonts[i].data()); // font at 0 never used?
        }
    }
};
Fonts _fonts;


/*
// a few annoying entities
<!ENTITY ndash   CDATA "&#8211;" -- en dash, U+2013 ISOpub -->
<!ENTITY mdash   CDATA "&#8212;" -- em dash, U+2014 ISOpub -->
<!ENTITY lsquo   CDATA "&#8216;" -- left single quotation mark, U+2018 ISOnum -->
<!ENTITY rsquo   CDATA "&#8217;" -- right single quotation mark,

<!ENTITY hellip   CDATA "&#8230;" -- horizontal ellipsis = three dot leader, U+2026 ISOpub  -->
*/

const char* _entities[] = {
        "lt","<",
        "gt",">",
        "amp","&",
        "apos","'",
        "quot","\"",
        "nbsp"," ",
        "#8230","...",

        "#8211","-",	// Do as unicode maps TODO
        "#8212","-",
        "#8216","'",
        "#8217","'",
        0
};

const int _uncodes[] =
{
        0xA0,	' ',	// nbsp
        0xA9,	'c',	// copyright
        0x2013,	'-',	// en dash
        0x2014,	'-',	// em dash
        0x2018,	'\'',	// left single quotation mark
        0x2019,	'\'',	// right single quotation mark
        0x201C, '"',	// left double quotation mark
        0x201D, '"',	// right double quotation mark
        0,
};

//	Map a unicode entitiy to something we can understand
int MapUnicode(int c)
{
        if (c < 128)
            return c;
        const int* u = _uncodes;
        while (*u)
        {
                if (*u == c)
                        return u[1];
                u += 2;
        }
        return ' ';
}

//===========================================================================
//===========================================================================
//
enum {
    Token_None,
    Token_Link_Set,
    Token_Link_Clear,
    Token_Style_Set,
    Token_Style_Clear,
    Token_MoveX,
    Token_MoveY,
    Token_Set_LineHeight,
    Token_PageBreak,
    Token_ImageRef,
    Token_Newline = '\n',
    Token_PageNum
};

EPubBook::EPubBook() : _root(0)
{
    memset(&_pageInfo,0,sizeof(PageInfo));
    _pageInfo.leftMargin = 12;
    _pageInfo.rightMargin = 12;
    _pageInfo.pageSize = 320;
    _pageInfo.descent = _fonts.descent(0);
    _pageInfo.lineHeight = _fonts.height(0);
    _pageCount = _pageMark = 0;
}

PageInfo& EPubBook::pageInfo()
{
    return _pageInfo;
}

void EPubBook::startBook(BlobFile& root)
{
    _root = &root;
}

void EPubBook::endBook(BlobFile& images,BlobFile& links)
{
    _root->add(images);

    BlobFile fonts;
    _fonts.toBlob(fonts);
    _root->add(fonts);

    _root->add(_navPoints);
    _root->add(links);

    // Space for a few bytes of state...
    ByteArray s;
    for (int i = 0; i < 256; i++)
        s.write(0);
    _root->add(s);

    // Add a final padding record to align to 4 bytes...
    int align = _root->payloadSize() & 3;
    ByteArray pad;
    while (align++<4)
        pad.push_back(0);   // Always present
    _root->add(pad);
}

int EPubBook::navPointCount()
{
    return _navPoints.count();
}

void EPubBook::startDocument()
{
    _sindex.clear();
    _data.clear();
    _link = 0;
    _style = 0;
}

const char _c4[] = " thethe  of  andand  is  in ing  to n thtionf thof tThe "; // 128 to 141
const char _c3[] = "the thhe  ofand in aned of is nd inger in  iss a toas to Theng iones on n tre  a enthertertioan "; // 142 to 173
const char _c2[] = "e heth ts an ainern d  ire ot onorisarenndteates, alstr edof wtiy  sitas. tongleri||ntmero cic bf o eaha flaouravesedelia  phillneiococa hmaomta ml sichbeh elThceus"; // 174 to 255

class Compress
{
//  Compress a line of data
// Codes >= 128 represent 2,3 or 4 chars
//  255 won't ever get used
#define _c4code 128
#define _c3code 142
#define _c2code 174

public:
    map<unsigned long,int> _map;

    void add(const char* m, int n)
    {
        int code = _map.size() + 128;
        while (*m)
        {
            unsigned long key = 0;
            int j = n;
            for (int j = 0; j < n; j++)
                key = (key <<8) | *m++;
            _map[key] = code++;
        }
    }

    void init()
    {
        add(_c4,4);
        add(_c3,3);
        add(_c2,2);
    }

    int getCode(unsigned char* src, int n)
    {
        unsigned long key = 0;
        for (int j = 0; j < n; j++)
            key = (key << 8) | *src++;
        if (_map.find(key) != _map.end())
            return _map[key];
        return 0;
    }

    void compressLine(unsigned char* src, int len, ByteArray& dst)
    {
        if (_map.empty())
            init();

        ByteArray line;
        int i;
        for (i = 0; i < len; i++)
            line.push_back(*src++);

        for (int n = 4; n > 1; n--)
        {
            int j = 0;
            for (i = 0; i < len-(n-1); i++)
            {
                u8 code = getCode(&line[i],n);
                if (code)
                {
                    line[j] = code;
                    i += n-1;
                } else
                    line[j] = line[i];
                j++;
            }
            while (i < len)
                line[j++] = line[i++];
            len = j;
        }

        for (i = 0; i < len; i++)
            dst.push_back(line[i]);
    }
};
Compress _compress;

//  Compress data using fixed table
void EPubBook::compress()
{
    int mark = 0;
    ByteArray data;
    for (int i = 0; i < _sindex.size(); i++)
    {
        _compress.compressLine(&_data[mark],_sindex[i].mark-mark,data);
        mark = _sindex[i].mark;
        _sindex[i].mark = data.size();
    }
    int pc = 0;
    if (_data.size())
        pc = data.size()*100/_data.size();
    printf("Compressed markup from %d to %d (%d%%)\n",_data.size(),data.size(),pc);
    _data = data;
}

void EPubBook::endDocument()
{
    printf("Document has %d lines and %d bytes\n",_sindex.size(),_data.size());
    // Dont compress just yet....
    //compress();

    ByteArray sindex;
    sindex.set(&_sindex[0],_sindex.size()*sizeof(SIndex));

    ByteArray p;
    p.set(&_pageInfo,sizeof(PageInfo));

    BlobFile n;
    n.add(sindex);  // Spatial Index
    n.add(_data);   // Markup
    n.add(p);       // Page Info

    _navPoints.add(n);
}

void EPubBook::pageNum(int y, int dy, int dx, const wstring& s)
{
    updateSpatialIndex(y);
    write(Token_PageNum);
    write(dy);
    write(dx);
    write(s.c_str(),s.size());
}

void EPubBook::pageBreak(int y)
{
    updateSpatialIndex(y);
    write(Token_PageBreak);
}

void EPubBook::updateSpatialIndex(int y)
{
    if (_data.size() != 0)
    {
        if (_sindex.size() == 0 || _sindex[_sindex.size()-1].y < y)
        {
            SIndex n = { y, _data.size() }; // TODO finalize
            _sindex.push_back(n);
        }
    }
}

void EPubBook::startLine(int y, int height, int descent)
{
    updateSpatialIndex(y);
    if (descent == -1)
        descent = _pageInfo.descent;
    if (height != -1 && (height != _pageInfo.lineHeight || descent != _pageInfo.descent))
    {
        write(Token_Set_LineHeight);
        write(height);
        write(descent);
    }
    _style = 0;
    _link = 0;
}

void EPubBook::endLine()
{
    write(Token_Newline);
}

void EPubBook::setLink(int link)
{
    if (_link == link)
        return;
    _link = link;
    if (_link)
    {
        write(Token_Link_Set);
        writeId(_link);
    } else
        write(Token_Link_Clear);
}

void EPubBook::setStyle(int style)
{
    if (_style == style)
        return;
    _style = style;
    if (_style)
    {
        write(Token_Style_Set);
        int s = style & 2 ? 0x10 : 0x00; // ITALIC
        write(_fonts.fontIndex(style) | s); // 0x1X is italic
    } else
        write(Token_Style_Clear);
}

void EPubBook::write(const wchar_t* t, int len)
{
    for (int i = 0; i < len; i++)
        write(MapUnicode(*t++));
}

void EPubBook::write(int token)
{
    //  Strip spaces from end of line...
    if (token == '\n' && _data.size() && _data[_data.size()-1] == ' ')
    {
        _data[_data.size()-1] = '\n';
        return;
    }
    _data.write(token);
}

void EPubBook::writeId(u32 id)
{
    WriteId(id,_data);
}

void EPubBook::writeImageRef(u32 id, u32 yoffset)
{
    write(Token_ImageRef);
    writeId(id);
    writeId(yoffset);
}
//===========================================================================
//===========================================================================

Glyph::Glyph() : x(0),width(0),height(0),descent(0),style(0),link(0)
{
}

Glyph::~Glyph()
{
}

void Glyph::write(EPubBook& doc)
{
    doc.setStyle(style);
    doc.setLink(link);
}

void TextGlyph::write(EPubBook& doc)
{
    Glyph::write(doc);
    doc.write(c_str(),size());
}

void PadGlyph::write(EPubBook& doc)
{
    doc.write(Token_MoveX);
    doc.write(width);
}

void ImageGlyph::write(EPubBook &doc)
{
    doc.write(Token_ImageRef);
    doc.writeId(id);
    doc.writeId(yoffset);
}

//===========================================================================
//===========================================================================

#define SCREEN_WIDTH 240
#define SCREEN_HEIGHT 320

Layout::Layout() : _text(0),_imageId(0)
{
    clear();
}

void Layout::clear()
{
    clearLine();
    _stack.clear();
    _state = StripWhitespace;

    _left = _document.pageInfo().leftMargin;
    _right = SCREEN_WIDTH - _document.pageInfo().rightMargin;
    _x = _left;
    _y = 0;
    _lastChar = ' ';

   delete _text;
    _text = 0;
    _style = 0;
    _link = 0;
    _imageUsed.clear();
}

void Layout::loadImage(const string& path, const u8* data, int len)
{
    QImage& img = _images[path];
    ByteArray dCover;

    // Add cover...
    if (path.find("cover") != string::npos)
    {
        _cover = path;
        if (len == 19263)   // ugly project gutenberg cover gets replaced
        {
            printf("Replacing Gutenberg cover...\n");
            defaultCover(dCover);
            data = &dCover[0];
            len = dCover.size();
        }
    }
    int csum = 0;
    for (int i = 0; i < len; i++)
        csum += data[i];
    _imageHash[path] = csum*len;

    img.loadFromData(data,len);
    printf("Loaded %s (%dx%d)\n",path.c_str(),img.width(),img.height());
}

int Layout::measure(int style, const wchar_t* t, int len, int& height)
{
    return _fonts.measure(style,t,len,height);
}

void Layout::clearLine()
{
    for (int i = 0; i < _line.size(); i++)
        delete _line[i];
    _line.clear();
}

void Layout::addLine(vector<Glyph*>& line, int height,int descent)
{
    // At top of page?
    if (atStartOfPage() && !(_state & HidePageNum))
    {
        _document.pageBreak(_y);
        _y += _document.pageInfo().lineHeight;
    }

    _document.startLine(_y,height,descent);
    for (int i = 0; i < line.size(); i++)
        line[i]->write(_document);
    _document.endLine();
    _y += height;
    _x = _left;
}

void Layout::padY(int height)
{
    while (height)
    {
        int h = min(height,255);
        vector<Glyph*> line;
        addLine(line,h,h); // Sufficient to start a new line
        height -= h;
    }
}

void Layout::padX(int width)
{
    addWord();
    PadGlyph* pad = new PadGlyph();
    pad->width = width;
    addGlyph(pad);
}

bool Layout::atLeft()
{
    return _x == _left && _text == 0 && _line.size() == 0;
}

//  Finalize page with page number
//  Bottom of page is 1.5 lineHeight high 1:0.5
//  Top of page is lineHeight high
//
void Layout::addPage(bool lastPage)
{
    int pageSize = _document.pageInfo().pageSize;
    int lineHeight = _document.pageInfo().lineHeight;
    int pageY = _y % pageSize;
    int pageNum = _y/pageSize + 1;
    int pad = pageSize-pageY;

    if (pad < lineHeight*3/2)
        padY(pad);      // No room for a page number
    else {
        pad -= lineHeight*3/2;
        while (pad > 255)
        {
            padY(255);
            pad -= 255;
        }

        wstring s = toWString(toString(pageNum));
        int dx= measure(0, s.c_str(), s.size(),lineHeight);
        dx = SCREEN_WIDTH/2 - _document.pageInfo().leftMargin - dx/2; // center number

        //  Write page number: dy,dx,str
        _document.pageNum(_y,pad,dx,s);
        _y = pageNum*pageSize;
    }
}

void Layout::flushPage(bool last)
{
    if (atStartOfPage())
        return;
    int pageSize = _document.pageInfo().pageSize;
    if (pageSize)
        addPage(last);
}

int Layout::remainingSpaceOnPage()
{
    int pageSize = _document.pageInfo().pageSize;
    if (pageSize == 0)
        return 0x7FFFFFFF;
    int pageY = _y % pageSize;
    int pageNum = _document.pageInfo().lineHeight*3/2;
    if (_state & HidePageNum)
        pageNum = 0;
    return pageSize - pageY - pageNum;
}

//  Add a line of glyphs to the document
void Layout::addLine()
{
    int height = 0;
    int descent = -1;
    for (int i = 0; i < _line.size(); i++)
    {
        height = max(height,_line[i]->height);
        descent = max(descent,_line[i]->descent);
    }

    if (height == 0)
        height = _document.pageInfo().lineHeight;

    //  Insert page break if needed
    int space = remainingSpaceOnPage();
    if (space < height)
        addPage();

    addLine(_line,height,descent);
    clearLine();
}

void Layout::addGlyph(Glyph *g)
{
    if (g->width + _x > _right)
        addLine();
    g->link = _link;
    _line.push_back(g);
    _x += g->width;
}

void Layout::addWord()
{
    if (_text == 0)
        return;

    int len = _text->size();
    if (len == 0)
        return;

    _text->width = measure(_text->style, _text->c_str(),_text->length(),_text->height);
    addGlyph(_text);
    _text = 0;
}

int Layout::getImageId(const string& url)
{
    if (_imageIdMap.find(url) == _imageIdMap.end())
    {
        _imageList.push_back(url);
        _imageIdMap[url] = ++_imageId;
    }
    return _imageIdMap[url];
}

void Layout::flushLine()
{
    addWord();
    if (!atLeft())
        newLine();
}

bool Layout::atStartOfPage()
{
    PageInfo info = _document.pageInfo();
    if (info.pageSize == 0)
        return _y == 0;
    return (_y % info.pageSize) == 0;
}

void Layout::addImage(const string& url)
{
    u32 id = getImageId(url);
    QImage& img = _images[url];

    int hash = _imageHash[url];
    if (_imageUsed.find(hash) != _imageUsed.end())
        if (hash == _imageHash["cover.jpg"])
            return;
    _imageUsed[hash] = url;

    PageInfo info = _document.pageInfo();
    int maxWidth = SCREEN_WIDTH - info.leftMargin - info.rightMargin;

    // Images always centered for now
    int slice = info.lineHeight;
    if (img.height() <= slice*2)
        slice = img.height();

    //  Insert page break if needed
    flushLine();
    if (!atStartOfPage() && remainingSpaceOnPage() < img.height())
        addPage();

    //  If it is still too tall, make it full page (no header, no numbers etc)
    if (remainingSpaceOnPage() < (img.height() + info.lineHeight))
        _state |= HidePageNum;

    int yoffset = 0;
    int dx = 0;
    if (img.height() > maxWidth)    // Center tall,wide pictures
        dx = (img.width()-maxWidth)/2;

    while (yoffset < img.height())
    {
        ImageGlyph* g = new ImageGlyph();
        g->width = img.width();
        g->height = min(slice,img.height()-yoffset);
        g->yoffset = yoffset;
        g->id = id;
        g->descent = info.descent;
        yoffset += g->height;
        if (false && dx)
        {
            PadGlyph* p = new PadGlyph();
            p->width = dx;
            addGlyph(p);
        }
        addGlyph(g);
        flushLine();
    };
    _state &= ~HidePageNum;
}

string Layout::absoluteUrl(string url)
{
    if (url.find("../") == 0)   // TODO
        return url.substr(3);
    return url;
}

void Layout::addImage(map<string,string>& attr)
{
    string url = absoluteUrl(attr["src"]);
    if (_images.find(url) == _images.end())
    {
        for (map<string,QImage>::iterator it = _images.begin(); it != _images.end(); ++it)
        {
            string u = it->first;
            if (u.find(url) != string::npos)
            {
                url = u;
                break;
            }
        }
        if (_images.find(url) == _images.end())
        {
            printf("Image Missing; %s\n",url.c_str());
            return;
        }
    }

    QImage& img = _images[url];
    int hash = _imageHash[url];
    PageInfo info = _document.pageInfo();

    bool scale = false;
    int maxWidth = SCREEN_WIDTH - info.leftMargin - info.rightMargin;
    int maxHeight = SCREEN_HEIGHT; // leave room for trailing caption
    if (img.width() > maxWidth || img.height() > maxHeight)
        scale = true;

    bool wider = (float)img.width()/img.height() > (float)maxWidth/maxHeight;

    if (scale)
    {
        QImage scaled = img.scaled(maxWidth,maxHeight,Qt::KeepAspectRatio,Qt::SmoothTransformation);
        printf("Scaling image %s (%dx%d to %dx%d\n",attr["src"].c_str(),img.width(),img.height(),scaled.width(),scaled.height());

        url = attr["src"] + "_" + toString(scaled.width()) + "x" + toString(scaled.height());
        _images[url] = scaled;
        _imageHash[url] = hash;
    }
    addImage(url);
}

void Layout::addChar(int c)
{
    bool space = isspace(c);
    if (space)
    {
        if (isspace(_lastChar))
            return;
    }
    _lastChar = c;
    _state &= ~StripWhitespace;

    if (_text == 0)
    {
        if (space && _line.size() == 0)
            return;
        _text = new TextGlyph();
        _text->style = _style;
        _text->descent = _fonts.descent(_style);
        _text->link = _link;
    }
    _text->push_back((wchar_t)c);
    if (space)
        addWord();
}

void Layout::setStyle(int style)
{
    if (style == _style)
        return;
    addWord();  // flush current word
    _style = style;
}

//  Merge styles into stack
int Layout::toStyle(int flags)
{
    int style = flags >> 4;
    int oldStyle = 0;
    if (_stack.size())
        oldStyle = _stack[_stack.size()-1];
    int size = max(style & 3,oldStyle & 3);
    int face = (style | oldStyle) & 0xC;
    style = face | size;
    _stack.push_back(style);
    return style;
}

void Layout::newLine()
{
    addWord();
    addLine();
}

void Layout::startBook(BlobFile& blob)
{
    bool hasCover = _cover.size();
    if (!hasCover)
    {
        // Create a cover from scratch
        _cover = "synth";
        QImage& img = _images[_cover];
        ByteArray d;
        defaultCover(d);
        img.loadFromData(&d[0],d.size());
    }

    QImage& img = _images[_cover];
    QImage scaled = img.scaled(48,64,Qt::KeepAspectRatio,Qt::SmoothTransformation);
    string url = _cover + "_" + toString(scaled.width()) + "x" + toString(scaled.height());
    _images[url] = scaled;
    getImageId(url);
    printf("Cover is %s\n",url.c_str());

    _document.startBook(blob);
    clear();
    _document.startDocument();
}

void Layout::startBadge()
{
    _document.pageInfo().leftMargin = 64;
    _document.pageInfo().pageSize = 0;
    clear();
}

void Layout::endBadge()
{
    flushLine();
    _document.startLine(_y,-1); //final spatial index mark
    _document.endDocument();

    _document.pageInfo().leftMargin = 12;
    _document.pageInfo().pageSize = 320;
    clear();
    _document.startDocument();

    // Add cover if present
    if (_cover != "synth")
    {
        map<string,string> attr;
        attr["src"] = _cover;
        addImage(attr);
    }
}

void Layout::endBook()
{
    flushPage(true);
    _document.startLine(_y,-1); //final spatial index mark
    _document.endDocument();

    BlobFile imageBlob;
    for (int i = 0; i < _imageList.size(); i++)
    {
        ByteArray data;
        ConvertTo16(_images[_imageList[i]],data);
        imageBlob.add(data);
    }

    BlobFile linksBlob;
    for (int i = 0; i < _linksList.size(); i++)
    {
        ByteArray data;
        mapLink(_linksList[i],data);
        linksBlob.add(data);
    }
    _document.endBook(imageBlob,linksBlob);
}

void Layout::startDocument()
{
}

bool Layout::endDocument()
{
    flushPage(false);
    return true;
}

void Layout::startUrl(const string& url)
{
    _currentUrl = url;
    _fragments[url] = _y;
}

// Map href to spatial position data
void Layout::mapLink(const string& href,ByteArray& data)
{
    int pos = 0;
    if (_fragments.find(href) != _fragments.end())
        pos = _fragments[href];
    else
    {
        int i = href.find('#');
        if (i != -1)
        {
            mapLink(href.substr(0,i),data);
            return;
        }
        printf("Missing %s\n",href.c_str());
    }

    data.write(1);  // link format 1
    WriteId(pos,data);
}

int Layout::addLink(map<string,string>& attr)
{
    string href = attr["href"];
    string name = attr["name"];
    string id = attr["id"];
    _currentLinkAttr = attr;

    if (name.size())
        _fragments["#" + name] = _fragments[_currentUrl + "#" + name] = _y;   // fragment identifier
    if (id.size())
        _fragments["#" + id]  = _fragments[_currentUrl + "#" + id] = _y;     // fragment identifier

    if (href.size())
    {
        if (href.find("http://") == 0)
            return 0;   // Can't do external links yet.

        if (_links.find(href) != _links.end())
            return _links[href];
        int l = _links.size()+1;
        _links[href] = l;
        _linksList.push_back(href);
        return l;
    }
    return 0;
}

void Layout::endLink()
{
    _currentLinkAttr.clear();
}

void Layout::start(const string& tag, map<string,string>& attr)
{
    Token t = Tokenize(tag.c_str());
    if (t == T_script)
    {
        _state |= InScript;
        return;
    }

    int flags = _tokenFlags[t];
    switch (t)
    {
        case T_p:
            flushLine();
            padX(12);   // Paragraph indent
            break;

        case T_a:
             _link = addLink(attr);
            break;

        case T_br:
            newLine();
            break;

        case T_img:
            addImage(attr);
            break;

        case T_hr:
            flushPage(false);   // Seems overkill but works well for gutenberg frontmatter
            break;

        default:
            if (flags & F_BLOCK)
                flushLine();
    }

    _state |= StripWhitespace;
    int style = toStyle(flags);
    setStyle(style);

    if (t == T_body)
        _state |= InBody;
}

void Layout::end(const string& tag)
{
    Token t = Tokenize(tag.c_str());
    if (t == T_script)
    {
        _state &= ~InScript;
        return;
    }

    int flags = _tokenFlags[t];
    if (flags & F_BLOCK)
        flushLine();

    if (t == T_a)
    {
        addWord();
        endLink();
        _link = 0;
    } else if (t == T_p)
        newLine();

    _state |= StripWhitespace;
    _stack.pop_back();
    setStyle(_stack[_stack.size()-1]);

    if (t == T_body)
        _state &= ~InBody;
}

void Layout::text(const wstring& text)
{
    if (!(_state & InBody))
        return;
    if (_state & InScript)
        return;

    // TODO: ugly hacks for epubbooks.com
    if (text.find(L"http://creativecommons.org/licenses/by-nc-nd/3.0/") == 0)
        return;

    // block of text, all with the same style
    for (int i = 0; i < text.size(); i++)
    {
        int c = text[i];
        if (c == '\n')
            c = ' ';    // space handling in pre
        addChar(c);
    }
}

void Layout::whitespace(const wstring& text)
{
}


// Add runs of text with the same style
