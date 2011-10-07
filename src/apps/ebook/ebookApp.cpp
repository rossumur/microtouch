
#include "Platform.h"
#include "Scroller.h"

//  This will all changef
// looks for .epb or .bks files in tools\MicrotouchSim\MicrotouchSim\microSD
short Mod320(long y);   // In scroller

namespace eBook
{

// Global font?
#define P(__x) pgm_read_byte(__x)
#define PW(__x) pgm_read_word(__x)
#define M(__x) *((u8*)(__x))
#define MW(__x) *((u16*)(__x))

#define BADGE_HEIGHT 80

#include "Sans8.h"

#define PC(_pos,_color) ((_pos << 4) | _color)

extern const u8 trackColors[9] PROGMEM;
const u8 trackColors[] = {
    PC(3,5),
    PC(1,6),
    PC(1,7),
    PC(0,8),
    PC(0,9),
    PC(0,10),
    PC(1,11),
    PC(1,12),
    PC(3,11)
};

extern const u8 thumbColors[7] PROGMEM;
const u8 thumbColors[] = {
    PC(3,15),
    PC(2,14),
    PC(1,13),
    PC(1,12),
    PC(1,11),
    PC(2,10),
    PC(3,9),
};

//	TODO
//	Inline Glyphs
//	Inline Images

//#ifdef _WIN32
enum MToken
{
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


void HEXLINE(const u8* d, int len)
{
    char buf[16+1];
    int i;
    for (i = 0; i < len; i++)
    {
        u8 c = d[i];
        printf("%02X ",c);
        if (c >= ' ' && c < 127 && c != '%')
            buf[i] = c;
        else
            buf[i] = '.';
    }
    buf[i]= 0;
    for (;i<16;i++)
        printf("   ");
    printf(buf);
    printf("\n");
}

void HEXDUMP(const u8* d, int len)
{
    while (len)
    {
        int c = min(len,16);
        HEXLINE(d,c);
        d += c;
        len-=c;
    }
    fflush(stdout);
}

void DUMP(int c)
{
    if (c != '\n' && c < 32)
    {
        const char* s = 0;
        switch (c)
        {
            case Token_Link_Set:        s = "[A]";    break;
            case Token_Link_Clear:      s = "[a]";    break;
            case Token_Style_Set:       s = "[S]";    break;
            case Token_Style_Clear:     s = "[s]";    break;
            case Token_MoveX:      s = "[dx]";    break;
            case Token_MoveY:    s = "[dy]";    break;
            case Token_Set_LineHeight:     s = "[lh]";    break;
            case Token_PageBreak:    s = "[pb]";    break;
            case Token_ImageRef:    s = "[ir]";    break;
        }
        if (s)
            printf(s);
        else
            printf("%02X",c);
    }
    else
        printf("%c",c);
}

#if 0
#define DUMP_MARKUP
#define DUMPCHAR(_c,_x) DumpChar(_c,_x)
#define DUMPTOK(_t) DumpTok(_t)
void DumpChar(char c, int x)
{
	printf("%c",c);
}
#else
#define DUMPCHAR(_c,_x)
#define DUMPTOK(_t) (_t)
#endif

enum {
	P_NONE,
	P_BLACK,
	P_RED,
	P_BLUE,
	P_GREY,
	P_BLUEINVERTED
};

u8 _ppalette[32];
u8 _defaultPalette;
u8 _currentPalette = 0;

void SetPalette(u8 p)
{
	if (p == _currentPalette)
		return;
	_currentPalette = p;

	for (u8 i = 0; i < 16; i++)
	{
		u8 c = i | (i << 4);
		u16 color = GRAY(c);	// Lookup
		if (p == P_BLACK)
			;
		else if (p == P_RED)
			color |= 0xF800;
		else if (p == P_BLUE)
			color |= 0x001F;
		else if (p == P_GREY)
		{
			c = 0x80 + (c >> 1);
			color = GRAY(c);
		}
		else {
			c = 0xFF - c;
			color = GRAY(c);
			color |= 0x001F;
		}
		u8* pp = _ppalette + i + i;
		pp[1] = color;
		pp[0] = color>>8;
	}

}


File* _diskFile;

typedef struct
{
    u8 format;
    u8 top;
    u16 width;
    u16 height;
} GlyphHdr; // __attribute__((__packed__));

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

static bool DirCallback(DirectoryEntry* d, int index, void* ref);

void BlitGlyph(short x, short y, short width, short height, short yoffset, short minY, short maxY)
{
    //printf("BlitGlyph y:%d h:%d yoffset:%d\n",y,height,yoffset);
    height -= yoffset;
    maxY = min(maxY,y+height);
	long skip = 0;
    if (minY > y)
    {
        skip = (long)(minY-y)*width*2;	// Clip lines off top
        y = minY;
    }
    if (y >= maxY)
        return;
    height = maxY-y;

    if (yoffset)
        skip += (long)yoffset*width << 1;
	_diskFile->SetPos(_diskFile->GetPos() + skip);

    Graphics.SetBounds(x,y,width,height);

    //	Draw pixels using file buffer
    long count = (long)width*height*2;		// TODO. These must be Word aligned
    while (count)
    {
        int c;
        const u8* b = _diskFile->GetBuffer(&c);
        c = min(c,count);
        Graphics.Blit(b,c>>1);
        _diskFile->Skip(c);
        count -= c;
    }
}

int DrawGlyphFromFile(long blob, short yoffset, short x, short y, short top, short bottom)
{
    GlyphHdr hdr;
    _diskFile->SetPos(blob);
    _diskFile->Read(&hdr,sizeof(hdr));
    BlitGlyph(x,y + hdr.top,hdr.width,hdr.height,yoffset,top,bottom);
    return hdr.width;
}

//========================================================================
//========================================================================
//  Disk based fonts

u8 _diskFont;
u8 _diskFontHeader[8 + ((128-32)+1)*4];
u8 _markBuf[32];
long _diskFonts[6]; // 0..5
long _diskFontPos;

const u8* _f;	// Font Ptr suddenly global
u8 _fontAscent;    // ascent of current font

static const u8* LoadFont(u8 index)
{
    if (index == _diskFont)
        return _diskFontHeader;
    _diskFont = index;
    _diskFontPos = _diskFonts[index];
    _diskFile->SetPos(_diskFontPos);
    _diskFile->Read(_diskFontHeader,sizeof(_diskFontHeader));
    return _diskFontHeader;
}

void SetFont(u8 i)
{
    SetPalette(i & 0x10 ? P_GREY : _defaultPalette);    // Italic?!

    i &= 0x0F;
    if (i == 0)
    {
        _f = defaultFont;
        _fontAscent = P(_f+4);
    }
    else
    {
        _f = LoadFont(i);
        _fontAscent = M(_f+4);
    }
}

//  Blit 4 bit data from memory
//  if phase&1 clip first pixel
void MemBlit(const u8* data, u8 count, u8 phase, u8 odd)
{
    count += count-phase-odd;
    u8 d = 0;
    u8 i = 0;
    u8 buf[64];
    if (phase)
        d = *data++;
    while (count--)
    {
		u8 p;
        if (!phase)
        {
            d = *data++;
            p = d >> 4;
        } else {
            p = d & 0x0F;
        }
        buf[i++] = p;
        phase ^= 1;
    }
    Graphics.BlitIndexed(buf,_ppalette,i);
}

// Blit font from Pgm or Disk
void FontBlit(const u8* font, short offset, short len, u8 phase)
{
    len += phase;
    u8 odd = len & 1;
    len = (len + 1)>>1;
    if (font)
        font += offset;
    else
        _diskFile->SetPos(offset + _diskFontPos);

    u8 buffer[32];  // 32 byte or 64 pixel buffer
    while (len)
    {
        u8 count = sizeof(buffer);
        if (sizeof(buffer) > (u16)len)
            count = len;
        if (font)
        {
            memcpy_P(buffer,font,count);
            font += count;
        } else
            _diskFile->Read(buffer,count);
        len -= count;

        MemBlit(buffer,count,phase,(len == 0) & odd);
        phase = 0;
    }
}

static byte DrawChar(char c, short dx, short dy, short minY, short maxY)
{
    const u8* font = _f;
    bool disk = font == _diskFontHeader;
    u8 width,height,top;
    short src,end,i;

    if (disk)
    {
        height = M(font+3);
        i = c - M(font+1);
        i = (i + 2) << 2;
        width = M(font+i);
        top = M(font+i+1);
        src = MW(font+i+2);
        end = MW(font+i+6);
    }
    else
    {
        height = P(font+3);
        i = c - P(font+1);
        i = (i + 2) << 2;
        width = P(font+i);
        top = P(font+i+1);
        src = PW(font+i+2);
        end = PW(font+i+6);
    }

    short toDraw = ((end-src)<<1)-(top&1);  // Number of pixels to draw
    top >>= 1;
    dy += top;
    height -= top;


    //  Clip top
    short clip = minY - dy;
    u8 phase = 0;       // # of pixels to skip before drawing
    if (clip > 0)
    {
        dy += clip;
        short pixSkip = width*clip;
        src += pixSkip >> 1;
        phase = pixSkip & 1;       // Skip a pixel at start
        toDraw -= pixSkip;
        if (toDraw <= 0 ||clip >= height)
            return width;
        height -= clip;
    }

    //  Clip bottom
    clip = (dy + height) - maxY;
    if (clip > 0)
    {
        if (clip >= height)
            return width;
        height -= clip;
        short maxPix = width*height;
        if (toDraw > maxPix)
            toDraw = maxPix;
    }

    if (height == 0 || dx+width > 240 || dy+height > 320)
        return width;
    Graphics.SetBounds(dx,dy,width,height);

    if (disk)
        font = 0;
    FontBlit(font,src,toDraw,phase);
    return width;
}


extern const char _c4[] PROGMEM;
extern const char _c3[] PROGMEM;
extern const char _c2[] PROGMEM;

//	Codes >= 128 represent 2,3 or 4 chars
#define _c4code 128
const char _c4[] = " thethe  of  andand  is  in ing  to n thtionf thof tThe "; // 128 to 141
#define _c3code 142
const char _c3[] = "the thhe  ofand in aned of is nd inger in  iss a toas to Theng iones on n tre  a enthertertioan "; // 142 to 173
#define _c2code 174
const char _c2[] = "e heth ts an ainern d  ire ot onorisarenndteates, alstr edof wtiy  sitas. tongleri||ntmero cic bf o eaha flaouravesedelia  phillneiococa hmaomta ml sichbeh elThceus"; // 174 to 255
//  255 won't ever get used

static const char* Decompress(byte c, byte* len)
{
    if (c >= _c2code)
    {
        *len = 2;
        c -= _c2code;
        return _c2 + c + c; // Normal case
    }
    if (c >= _c3code)
    {
        *len = 3;
        c -= _c3code;
        return _c3 + (c<<1) + c;
    }
    *len = 4;
    c -= _c4code;
    return _c4 + (c<<2);
}

class LayoutBlock
{
    public:
    long Y0;
    long D0;
    long Y1;
    long D1;

    bool Contains(long y)
    {
        return (y >= Y0 && y < Y1);
    }
};

#define GREY(_x) (((_x & 0xF8) << 8) | ((_x & 0xFC) << 3) | ((_x & 0xF8) >> 3))

//========================================================================
//========================================================================

class SpatialIndex
{
    File* _file;
    long _table;
    long _count;
    long _lastIndex;
    long _lastY;
public:
    LayoutBlock L0; // top of layout range
    LayoutBlock L1; // bottom of layout range
    long MaxY;

    void Init(File& file, long table, long len)
    {
        L0.Y0 = L0.Y1 = L1.Y0 = L1.Y1 = 0;
        _file = &file;
        _table = table;
        _count = len >> 3;

        _file->SetPos(_table + len-8);
        _file->Read(&MaxY,4);

        _lastIndex = _lastY = 0;
    }

    void ReadSI(long index, long s[2])
    {
        _file->SetPos(_table + (index << 3));
        _file->Read(s,8);
    }

    // Look for the first record > y
    long FindY(long y)
    {
        long s[2];
        long mid = 0;
        long from = 0;
        long to = _count;
        while (from < to)
        {
            mid = (from + to) >> 1;
            ReadSI(mid,s);
            if (y < s[0])
                to = mid;
            else if (y >= s[0])
                from = mid + 1;
            else
                break;
        }
        if (s[0] <= y)
            ++mid;
        return mid; // first record > y
    }

    long GetY(long index)
    {
        long s[2];
        ReadSI(index,s);
        return s[0];
    }

    void Seek(LayoutBlock& l, long y)
    {
        long index = _lastIndex;
        if (y < 0)
            index = 0;
        else if (y >= MaxY)
            index = _count-1;
        else {
            if (abs(y - _lastY) < 512)
            {
                index = _lastIndex;     // Local Linear search...
                if (y < _lastY)
                {
                    while (index && GetY(index-1) > y) // earlier
                        index--;
                } else {
                    while (GetY(index) <= y)
                        index++;
                }
            } else
                index = FindY(y);       // Binary search for big steps
        }

        if (index)
            ReadSI(index-1,(long*)&l.Y0);
        else
            l.Y0 = l.D0 = 0;
        ReadSI(index,(long*)&l.Y1);
        _lastY = y;
        _lastIndex = index;
    }

    //  often L1 becomes L0 when smooth scrolling; should check L0/L1 before doing a search
    //  avoids spilling disk buffers
    void Seek(long y0, long y1)
    {
        --y1;
        if (!L0.Contains(y0))
        {
            if (L1.Contains(y0))
                L0 = L1;
            else
                Seek(L0,y0);
        }
        if (!L1.Contains(y1))
        {
            if (L0.Contains(y1))
                L1 = L0;
            else
                Seek(L1,y1);
        }
        if (L0.Y0 == L0.Y1 || L1.Y0 == L1.Y1)
        {
            y0 = y1 = 0;
        }
    }
};

//========================================================================
//========================================================================

class EPubState
{
    short _x;
    short _y;   // TODO tall?

    PageInfo _info;
    File _file;
    long _markup;
    long _markupLen;
    SpatialIndex _spatialIndex;
    int _lineHeight;
    u8  _descent;

    long _link;
    long _fontBlob;
    long _imageBlob;
    long _navBlob;
    long _linksBlob;
    long _saveBlob;

    long _trackLink;    // link we are currently tracking
    short _linkX;       // x position of click that start
    short _linkLeft;
    u8 _linkWidth;
    bool _inLink;       // Drawing tracked link

    short _trackPage;
    short _trackPageThumb;
    ushort _timer;
    short _badgeClick;

    Scroller _scroller;

    short _bookIndex;
    short _bookCount;
    short _bookCurrent;
    short _bookTarget;

    enum UIState
    {
        None,
        Scanning,
        InBadges,
        InBook
    };
    byte _uiState;
    signed char _vpad;  // vertical offset for lines with differing font sizes

public:
    void Init()
    {
        _link = -1;
        _linkX = -1;

        _diskFile = &_file;
        _diskFont = 0xff;

        _uiState = None;
        _trackLink = 0;
        _trackPage = 0;
        _badgeClick = -1;
        _file.Init();
        _bookCount = 0;
        _bookCurrent = -2;
        _bookTarget = -1;
        _defaultPalette = P_BLACK;
        InitScroll(0);
    }
#if 0
    void Dump()
    {
        long len;
        long navPoints = GetBlob(0,0,len);
        long n;
        int i = 0;
        while ((n = GetBlob(navPoints,i++,len)) > 0)
            printf("Nav %ld\n",len);
    }

    void DumpMarkup(int from, int to)
    {
        u8 buf[1024];
        printf("M %d:%d ",from,to);
        int len = min(to-from,sizeof(buf));
        {
            _file.SetPos(_markup+from);
            _file.Read(buf,len);
            HEXDUMP(buf,len);
        }
    }
#endif

    void InitScroll(long height)
    {
        _scroller.Init(height,DrawProc,this,_info.pageSize);
    }

    static void DrawProc(long scroll, int y, int height, void* ref)
    {
        ((EPubState*)ref)->Draw(scroll,y,height);
    }

    void UpdateFontAscent()
    {
        _vpad = _lineHeight - _descent - _fontAscent;
    }

    void SetFontHeight(u8 i)
    {
        SetFont(i);
        UpdateFontAscent();
    }

    void BeginLine()
    {
        _x = _info.leftMargin;
        _lineHeight = _info.lineHeight;   // RESET
        _descent = _info.descent;
        SetFontHeight(0);
    }

    void NewLine()
    {
        _y += _lineHeight;
        BeginLine();
    }

    void HRule(short top, short bottom)
    {
        if (_x != _info.leftMargin)
            NewLine();
        if (_y >= top && _y < bottom)
            Graphics.Rectangle(_info.leftMargin,_y,240-(_info.leftMargin+_info.rightMargin),1,GREY(0xC0));
        _lineHeight = 4;
        NewLine();
    }

    void Seek(long m)
    {
        _file.SetPos(m);
    }

    long Read32(long offset)
    {
        Seek(offset);
        long n;
        _file.Read(&n,4);
        return n;
    }

    void Read(long offset, void* dst, int len)
    {
        Seek(offset);
        _file.Read(dst,len);
    }

    // Get the file offset of a child blob
    long GetBlob(long parentBlob, long index, long& len)
    {
        len = 0;
        long count = Read32(parentBlob + 8);
        if (index < 0 || index >= count)
            return -1;
        long start = Read32(parentBlob + 12 + index * 4);
        long end = Read32(parentBlob + 12 + index * 4 + 4);
        len = end - start;
        return start + parentBlob;
    }

    void Load(long* t, long blob, u8 n)
    {
        for (u8 i = 0; i < n; i++)
        {
            long len;
            t[i] = GetBlob(blob,i,len);
        }
    }

    long OnLink(long id)
    {
        if (id == 0)
            return -1;
        long len;
        long m = GetBlob(_linksBlob,id-1,len);
        if (m == -1)
            return -1; // Bad link
        BeginRead(m);
        ReadByte();	// link type
        short n = len;
        return ReadId(n) - (320/2);   // Center fragment in screen
    }

    bool OpenNavPoint(int index)
    {
        long len;
        long current = GetBlob(_navBlob,index,len);
        if (current == -1)
            return false;

        long table = GetBlob(current,0,len);
        _spatialIndex.Init(_file,table,len);
        _markup = GetBlob(current,1,_markupLen);

        long pageInfo = GetBlob(current,2,len);
        Read(pageInfo,&_info,sizeof(PageInfo));

        // Opening book
        if (index)
        {
            InitScroll(0);
            long savedPos;
            Read(_saveBlob,&savedPos,4);
            _scroller.ScrollTo(savedPos);
			_scroller.SetHeight(_spatialIndex.MaxY);
        }
		return true;
    }

    int Open(const char* path)
    {
        if (!_file.Open(path))
            return -1;
		return Load(0);
	}

	int Load(long root)
	{
        long len;
        _imageBlob = GetBlob(root,0,len); // image blob first to keep it aligned
        _fontBlob = GetBlob(root,1,len);
        _navBlob = GetBlob(root,2,len);
        _linksBlob = GetBlob(root,3,len);
        _saveBlob = GetBlob(root,4,len);

        _diskFont = 0xFF;
        Load(_diskFonts,_fontBlob,6);

        OpenNavPoint(_uiState == InBook ? 1 : 0);
        return 0;
    }

    void Spaces(u8 depth)
    {
        depth <<= 1;
        while (depth--)
                printf(" ");
    }

    void Dump(const char* name, long blob, int depth)
    {
        Spaces(depth);
        printf("%s %ld\n",name,blob);
        for (int index = 0; index < 16; index++)
        {
            long len;    // TODO
            long p = GetBlob(blob,index,len);
            if (p == -1)
                break;
            printf("%d %ld %ld\n",index,p,len);
            //Dump(p,depth+1);
        }
    }

    #ifndef tolower
    char tolower(char _x)
    {
        return (_x >= 'A' && _x < 'Z') ? (_x - 'A' + 'a') : _x;
    }
    #endif

    void DrawSlice(long y0, long y1)
    {
        _spatialIndex.Seek(y0,y1);
    }

    u32 _pos;
    u8  _mark;

    //	slightly buffer reads
    //  Helps with drawing disk based glyphs
    void BeginRead(long pos)
    {
        _pos = pos;
        _mark = sizeof(_markBuf);
    }

    u8 ReadByte()
    {
        if (_mark == sizeof(_markBuf))
        {
            _file.SetPos(_pos);
            _file.Read(_markBuf,sizeof(_markBuf));
            _pos += sizeof(_markBuf);
            _mark = 0;
        }
        return _markBuf[_mark++];
    }

    long ReadId(short& n)
    {
        long id = 0;
        for (; ; )
        {
            u8 b = ReadByte();
            n--;
            id |= b & 0x7F;
            if (b < 128)
                break;
            id <<= 7;
        }
        return id;
    }

    void LinkClear(int linkLeft)
    {
        if (_linkX != -1)
        {
            if (_linkX >= linkLeft && _linkX < _x)
                FoundLink(_link,linkLeft,_x);
        }
        _link = -1;
        SetPalette(_defaultPalette);
    }

    int DrawGlyph(long id, short yoffset, short top, short bottom)
    {
        //printf("DrawGlyph yoff:%d top:%d bottom:%d\n",yoffset,top,bottom);
        long pos = _diskFile->GetPos();
        long len;
        long g = GetBlob(_imageBlob, (int)id-1, len);
        int w = DrawGlyphFromFile(g,yoffset,_x,_y,top,bottom);
        _diskFile->SetPos(pos);
        return w;
    }

    void OpenBook(short badgeIndex)
    {
        _uiState = InBook;
        _bookTarget = badgeIndex;
        short saveScroll = _scroller.CurrentScroll()/BADGE_HEIGHT;
        WalkAllBooks();
        _bookTarget = saveScroll;
    }

    void LoadBadge(short badgeIndex)
    {
        if (badgeIndex != _bookCurrent)
        {
            _bookTarget = badgeIndex;
            WalkAllBooks();
        }
    }

    void DrawBadge(bool hilite)
    {
        long y = _badgeClick*BADGE_HEIGHT;
        if (hilite)
            _defaultPalette = P_BLUEINVERTED;
        DrawBadges(y,Mod320(y),BADGE_HEIGHT);
        _defaultPalette = P_BLACK;
    }

    //  Draw book badges
    void DrawBadges(long srcY, short top, short height)
    {
        long badgeIndex = srcY / BADGE_HEIGHT;
        long badgeTop = badgeIndex * BADGE_HEIGHT;
        srcY -= badgeTop;
        short bottom = top + height;
        while (top < bottom)
        {
            short slice = min(bottom-top,BADGE_HEIGHT-srcY);
            LoadBadge(badgeIndex);

            if (badgeIndex < _bookCount)
            {
                DrawContent(srcY,top,slice);
                //  Draw Icon
                _y = 8 + top - srcY;
                _x = 8;
                DrawGlyph(1,0,top,top+slice);
            } else {
                Graphics.Rectangle(0,top,240,slice,TOCOLOR(0xE0,0xE0,0xE0));
            }

            //  Draw divider
            if (srcY == 0)
                Graphics.Rectangle(0,top,240,1,TOCOLOR(0xC0,0xC0,0xC0));

            top += slice;
            badgeIndex++;
            srcY = 0;
        }
    }

    //	srcY is logical, top is mapped to physical
    void Draw(long srcY, short top, short height)
    {
        if (_uiState == InBadges)
            DrawBadges(srcY,top,height);
        else
            DrawContent(srcY,top,height);
    }

    void StartLine(int linkLeft)
    {
        LinkClear(linkLeft);
        NewLine();
    }

    //	srcY is logical, top is mapped to physical
    void DrawContent(long srcY, short top, short height)
    {
        //printf("\nDraw %d T:%d H:%d\n",(int)srcY,top,height);
        while (top < 0)
            top += 320;
        while (top >= 320)
            top -= 320;
        short bottom = top + height;

        //  Draw a slice within 0-319 from srcY
        _spatialIndex.Seek(srcY,srcY+height);
        _y = top - (srcY - _spatialIndex.L0.Y0);   // _y <= top, may need to crop lines off top
       // _yVerify = _spatialIndex.L0.Y0 - _y;

        //  Set up data range
        long from = _spatialIndex.L0.D0;
        long to = _spatialIndex.L1.D1;
        BeginRead(from + _markup);

        //printf("%ld to %ld, y:%ld %ld\n",from,to,_spatialIndex.L0.Y0,_spatialIndex.L1.Y1);

        //  Draw BG - not required if just tracking a link!
        if (_trackLink == 0)
        {
            short color = 0xFFFF;
			u8 w = 240;
			if (_trackPage)
				w = 228;	// Don't overdraw scrollbar
            if (_defaultPalette == P_BLUEINVERTED)
                color = TOCOLOR(0,0,0xFF);
            Graphics.Rectangle(0,top,w,bottom-top,color);
        }

        short linkLeft = 0;

        //  Draw the slice
        BeginLine();
        short n = to - from;    // Number of bytes to draw
        while (n-- > 0)         // (or _y > bottom)
        {
            u8 c = ReadByte();
            if (c == 0)
                break;  // ?
            //DUMP(c);
            if (c >= 32)
            {
                if (c >= 128)
                {
                    u8 len;
                    const char* str = Decompress(c,&len);
                    while (len--)
                    {
                        _x += DrawChar(P(str), _x, _y+_vpad,top, bottom);
                        str++;
                    }
                } else {
                    _x += DrawChar((char)c, _x, _y+_vpad,top, bottom);
                }
            }
            else
            {
                MToken t = (MToken)c;
                switch (t)
                {
                case Token_Link_Set:
                    _link = ReadId(n);
                    linkLeft = _x;
                    if (_link == _trackLink && _inLink)
						SetPalette(P_RED);
                    else
                        SetPalette(P_BLUE);
                    break;

                case Token_Link_Clear:
                    LinkClear(linkLeft);
                    break;

                case Token_Set_LineHeight:
                    _lineHeight = ReadByte();
                    _descent = ReadByte();
                    UpdateFontAscent();
                    n -= 2;
                    break;

                case Token_MoveY:
                    _y += (signed char)ReadByte();
                    n--;
                    break;

                case Token_MoveX:
                    _x += (signed char)ReadByte();
                    n--;
                    break;

                case Token_Style_Set:
                    SetFontHeight(ReadByte());
                    n--;
                    break;

                case Token_Style_Clear:
                    SetFontHeight(0);
                    break;

                case Token_Newline:
                    StartLine(linkLeft);
                    break;

                //  Page numbers take up 3/2 lines with number centered horizontally
                case Token_PageNum:
                    {
                        StartLine(linkLeft);
                        _y += ReadByte() - _lineHeight;
                        _x += ReadByte();
                        _lineHeight += _lineHeight>>1;
                        n -= 2;
                    }
                    break;

                case Token_PageBreak:
                    StartLine(linkLeft);
                    Graphics.Rectangle(0,_y-_info.lineHeight,240,1,TOCOLOR(0xC0,0xC0,0xC0));
                    break;

                case Token_ImageRef:
                    {
                        u32 id = ReadId(n);
                        u32 yoffset = ReadId(n);
                        _x += DrawGlyph(id,yoffset,top,min(_y + _lineHeight,bottom));
                    }
                    break;

                    default:
						;
						//printf("bad token %d\n",c);

                }
            }
        }
        if (_trackPage)
            TrackPageDraw();
    }

    //  Found the link, track the bounds
    void FoundLink(long link, short x0, short x1)
    {
        if (link == -1)
            return;
        _trackLink = link;  //
        _linkLeft = x0;
        _linkWidth = x1 - x0;
    }

    long ToScrollY(ushort y)
    {
        return y + _scroller.CurrentScroll();
    }

    bool InLink(ushort x, ushort y)
    {
        long ly = ToScrollY(y);
        long linkTop = _spatialIndex.L0.Y0;
        long linkBottom = _spatialIndex.L0.Y1;
        return ((int)x >= _linkLeft && (int)x < (_linkLeft + _linkWidth)) && (ly >= linkTop && ly < linkBottom);
    }

    void TrackLink(ushort x, ushort y)
    {
        if (_trackLink == 0)
            return;
        bool inLink = InLink(x,y);
        if (inLink == _inLink)
            return;
        _inLink = inLink;       // Need to redraw link
        long linkTop = _spatialIndex.L0.Y0;
        long linkBottom = _spatialIndex.L0.Y1;
        Draw(linkTop,Mod320(linkTop),linkBottom-linkTop);  // Redraw
    }

    //  Locate the link, if any, at this position
    bool FindLink(ushort x, ushort y)
    {
        _linkX = x;
        _trackLink = 0;
        _inLink = false;
        Draw(ToScrollY(y),0,0);    // completely clipped drawing, just looking for link
        _linkX = -1;
        return _trackLink != 0;
    }

    void Clear()
    {
        Graphics.Rectangle(0,0,240,320,0xFFFF);
        InitScroll(0);
    }

    short Back()
    {
        if (_uiState == InBadges)
            return -1;

        //  Save current position
        long savedPos = _scroller.CurrentScroll();
        _file.SetPos(_saveBlob);
        _file.Write(&savedPos,4);
        _file.Close();

        _bookCurrent = -1;
        ShowBadges(_bookTarget);
        return 0;
    }

    #define TRACK_HEIGHT 121
    #define THUMB_HEIGHT 41
    #define THUMB_RANGE (((TRACK_HEIGHT-THUMB_HEIGHT)/2)-1)

    void GradientRect(u8 x, u8 width, short y, short height, const u8* colors)
    {
        while (width--)
        {
            u8 p = P(colors++);
            u8 c = p & 0xF;
            p >>= 4;
            const u8* color = _ppalette + c + c;
            Graphics.Rectangle(x++,y+p,1,height-p-p,color[0] << 8 | color[1]);
        }
    }

    void TrackPageDraw()
    {
        bool red = ToScrollY(320) >= _spatialIndex.MaxY;
		SetPalette(red ? P_RED:P_BLACK);	// Red when we hit end.
        u8 x = 240-12;
        GradientRect(x,9,_trackPage-TRACK_HEIGHT/2,TRACK_HEIGHT,trackColors);
        GradientRect(x+1,7,_trackPageThumb-THUMB_HEIGHT/2,THUMB_HEIGHT,thumbColors);
    }

    bool TrackPageClick(short x, short y)
    {
        if (x < 240-20)
            return false;
        if (y < TRACK_HEIGHT/2 || y > (320 - TRACK_HEIGHT/2))
            return false;
        _timer = 0;
        _trackPage = _trackPageThumb = y;
        return true;
    }

    void TrackPageUp()
    {
        Graphics.Rectangle(240-12,_trackPage-TRACK_HEIGHT/2,10,TRACK_HEIGHT,0xFFFF);
        _trackPage = 0;
    }

    void TrackPageUpdate()
    {
        u8 rate = abs(_trackPage - _trackPageThumb);
        if (rate > 2)
        {
            ushort period = 2000/(rate-2);
            ushort t = Hardware.GetTicks() - _timer;
            if (t > period)
            {
                int delta = _info.pageSize;
                if (_trackPage > _trackPageThumb)
                    delta = -delta;
                long dst = _scroller.CurrentScroll() + delta;
                if (dst >= 0 && dst < _spatialIndex.MaxY)
                    _scroller.ScrollTo(dst);
                _timer = Hardware.GetTicks();
            }
        }
    }

    //
    bool OnFile(const char* path, long root)
    {
		// Opening....
        if (_bookTarget == _bookIndex)
        {
            _bookCurrent = _bookIndex;
			if (path)
				Open(path);
			else
				Load(root);
			return true;
        }

		//	Just counting
        _bookIndex++;
        if (_bookIndex > _bookCount)
            _bookCount = _bookIndex;
		return false;
    }

    bool Callback(DirectoryEntry* entry)
    {
        const char* n = entry->fatname;
        if ((n[8] == 'E' && n[9] == 'P' && n[10] == 'B') || (n[8] == 'B' && n[9] == 'K' && n[10] == 'S'))
        {
            char name[16];
            FAT_Name(name,entry);
			if (n[8] == 'E')
				return OnFile(name,0); // single book

			//	Bookshelf is a collection of books in a blobfile
			_file.Open(name);
			long len;
			long files = GetBlob(0,0,len);
			int i = 0;
			while (files != -1)
			{
				long len;
				long b = GetBlob(files,i++,len);
				if (b == -1)
					break;
				if (OnFile(0,b))
					return true;
			}
			_file.Close();
        }
        return false;
    }

    short WalkAllBooks()
    {
        u8 buffer[512];
        _bookIndex = 0;
        return FAT_Directory(DirCallback,buffer,this);
    }

    short range(short n, short a, short b)
    {
        if (n < a)
            return a;
        if (n > b)
            return b;
        return n;
    }

    void BadgeClickUpdate()
    {
        if (_timer == 0)
            return;
        ushort t = Hardware.GetTicks() - _timer;
        if (t > 300) // Don't hilite for 300ms
        {
            DrawBadge(true);
            _timer = 0;
        }
    }

    void ShowBadges(short restore)
    {
        _scroller.Clear();
        _uiState = InBadges;    // Now display them
        long h = (long)_bookCount*BADGE_HEIGHT;
        if (h < 320)
            h = 320;
        _scroller.Init(0,DrawProc,this,BADGE_HEIGHT);
        _scroller.ScrollTo((long)restore*BADGE_HEIGHT);
		_scroller.SetHeight(h);	// 
    }


	int _xx,_yy;
	void DrawStr(const char* s)
	{
		SetFont(0);
		while (*s)
		{
			if (*s == '\n')
			{
				_yy += 12;
				_xx = 8;
			}
			else
				_xx += DrawChar(*s,_xx,_yy,0,320);
			s++;
		}
	}

    //  Touch the keyboard or the page
	short _downY;
    int OnEvent(Event* e)
    {
        if (e->Type == Event::OpenApp)
        {
			_yy = _xx = 8;
            Init();
            _uiState = Scanning;    // Count books
            WalkAllBooks();
            ShowBadges(0);
            return 0;
        }

        TouchData* touch = (TouchData*)e->Data;
        //Tracking touch
        //	Link
        //	Scroll
        switch (e->Type)
        {
            case Event::TouchDown:
                _badgeClick = -1;
                if (touch->y > 320)
                    _badgeClick = -2;
                else
                {
                    if (_uiState == InBadges)
                    {
                        _badgeClick = (touch->y + _scroller.CurrentScroll())/BADGE_HEIGHT;
						_downY = touch->y;
                        if (_badgeClick >= _bookCount)
                            _badgeClick = -1;
                        else
                            _timer = Hardware.GetTicks();
                        _scroller.OnEvent(e);
                    } else {
                        if (FindLink(touch->x,touch->y))
                            TrackLink(touch->x,touch->y);
                        else
                        {
                            if (TrackPageClick(touch->x,touch->y))
                                TrackPageDraw();
                            else
                                _scroller.OnEvent(e);
                        }
                    }
                }
                break;

            case Event::TouchMove:
                if (_trackLink)
                    TrackLink(touch->x,touch->y);	// TODO
                else
                {
                    if (_trackPage)
                    {
                        _trackPageThumb = range(touch->y,_trackPage-THUMB_RANGE,_trackPage+THUMB_RANGE);
                        TrackPageDraw();
                        TrackPageUpdate();
                    }
                    else if (_badgeClick == -2)
                        ;   // Tracking Back
                    else {
                        if (_uiState == InBadges && _badgeClick >= 0 && abs(touch->y - _downY) > 8)
                        {
							if (_timer == 0)
								DrawBadge(false);   // Cancel badge hilite on move
							_badgeClick = -1;
							_timer = 0;
                        }
                        _scroller.OnEvent(e);
                    }
                }
                break;

            case Event::TouchUp:
                if (_trackLink)
                {
                    long id = _trackLink;
                    _trackLink = 0;
                    long pos = OnLink(id);
                    if (pos != -1)
                        _scroller.ScrollTo(pos);
                }
                else if (_badgeClick == -2 && touch->y > 320)
                    return Back();
                else if (_trackPage)
                     TrackPageUp();
                else
                {
                    _scroller.OnEvent(e);
                    if (_badgeClick != -1 && _timer == 0)
                        OpenBook(_badgeClick);
					_timer = 0;
                }
                break;

            case Event::None:
                if (_trackPage)
                    TrackPageUpdate();
                if (_uiState == InBadges && _badgeClick >= 0)
                    BadgeClickUpdate();
                else
                    _scroller.OnEvent(e);
                break;

			// Idle
            default:
                break;
        }
        return 0;
    }
};

bool DirCallback(DirectoryEntry* d, int index, void* ref)
{
    return ((EPubState*)ref)->Callback(d);
}

};

//	Register the application, start getting events on OnEvent method

INSTALL_APP(ebook,eBook::EPubState);
