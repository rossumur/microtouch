
#include "Platform.h"
#include "Scroller.h"

//	This will all change
//	uses wiki.blb file from tools\MicrotouchSim\MicrotouchSim\microSD

extern const char DATA_FILE[] PROGMEM;
extern const char ERROR_MSG[] PROGMEM;

const char DATA_FILE[] = "wiki.blb";
const char ERROR_MSG[] = " file missing";

#define KEYBOARD_PATH 	0
#define WIKI_PATH 		1
#define BUTTONBAR_PATH	2
#define HISTORYT_PATH	3
#define SEARCHT_PATH	4

// Global font?
#define P(__x) pgm_read_byte(__x)
#define PW(__x) pgm_read_word(__x)
#define M(__x) *((u8*)(__x))
#define MW(__x) *((u16*)(__x))


#include "WikiFonts.h"

//	TODO
//	Quality
//	Inline Glyphs
//	Inline Images
//	Font compression

//#ifdef _WIN32
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

const u8* _ppalette = NormalPalette;	// TODO hack

#define LINE_HEIGHT 20 // Seach and History line height

long _diskImages[5];
File* _diskFile;

typedef struct
{
	u8 format;
	u8 top;
	u8 width;
	u8 height;
} GlyphHdr;

void BlitGlyph(int x, int y, int width, int height, int minY, int maxY)
{
	maxY = min(maxY,y+height);
	if (minY > y)
	{
		_diskFile->SetPos(_diskFile->GetPos() + (minY-y)*width*2);	// Clip lines off top
		y = minY;
	}
	if (y >= maxY)
		return;
	height = maxY-y;

	Graphics.SetBounds(x,y,width,height);

	//	Draw pixels using file buffer
	long count = (long)width*height*2;		// TODO. These must be Word aligned
	while (count)
	{
        int c;
        const u8* b = _diskFile->GetBuffer(&c);
		c = min(c,count);
		Graphics.Blit(b,c>>1);
		count -= c;
		_diskFile->Skip(c);
	}
}

int DrawGlyphFromFile(long blob, int x, int y, int top, int bottom)
{
	GlyphHdr hdr;
	_diskFile->SetPos(blob);
	_diskFile->Read(&hdr,4);
	BlitGlyph(x,y + hdr.top,hdr.width,hdr.height,top,bottom);
	return hdr.width;
}

//	Glyphs
//	Width and Height are of actual data
//	top moves the vpos

void DrawImage(u8 path, int x, int y, int top = 0, int bottom = 320)
{
	DrawGlyphFromFile(_diskImages[path],x,y,top,bottom);
}

void DrawImageWrap(u8 path, int x, int y, int height)
{
	int lines = min(height,320-y);
	DrawImage(path,x,y,y,y+lines);
	if (lines < height)
		DrawImage(path,x,y-320,0,height-lines);
}

//========================================================================
//========================================================================

u8 _diskFont;
u8 _diskFontHeader[8 + ((128-32)+1)*4];
u8	_markBuf[1];
long _diskFonts[4];
long _diskFontPos;
const u8* _f;			// Eww suddenly global
short _lineHeight;

const u8* LoadFont(u8 index)
{
	if (index == _diskFont)
		return _diskFontHeader;

	_diskFont = index;
	_diskFontPos = _diskFonts[index];
	_diskFile->SetPos(_diskFontPos);
	_diskFile->Read(_diskFontHeader,sizeof(_diskFontHeader));
	return _diskFontHeader;
}

void SetFont(int i)
{
	if (i == 0)
	{
		_f = Sans7;
		_lineHeight = P(_f+3);
	}
	else
	{
		_f = LoadFont(i);
		_lineHeight = M(_f+3);
	}
}


void BlitIndexedPP(const u8* pgm, const u8* pgmp, int len)
{
	while (len)
	{
		u8 b[32];
		u8 count = 16;
		if (len < count)
			count = len;
		u8 j = 0;
		while (count--)
		{
			u8 index = pgm_read_byte(pgm++);	// 
			const u8* pal = pgmp + index + index;
			b[j++] = pgm_read_byte(pal);
			b[j++] = pgm_read_byte(pal+1);
		}
		j >>= 1;
		Graphics.Blit(b,j);
		len -= j;
	}
}

void BlitIndexed(const u8* d, const u8* pgmp, int len)
{
	while (len)
	{
		u8 b[32];
		u8 count = 16;
		if (len < count)
			count = len;
		u8 j = 0;
		while (count--)
		{
			u8 index = *d++;	// 
			const u8* pal = pgmp + index + index;
			b[j++] = pgm_read_byte(pal);
			b[j++] = pgm_read_byte(pal+1);
		}
		j >>= 1;
		Graphics.Blit(b,j);
		len -= j;
	}
}

void DiskBlit(short offset, int len)
{
	long cpos = offset + _diskFontPos;
	_diskFile->SetPos(cpos);
	while (len)
	{
		int count;
		const u8* buffer = _diskFile->GetBuffer(&count);
		if (count > len)
			count = len;
		BlitIndexed(buffer,_ppalette,count);
		len -= count;
		_diskFile->Skip(count);
	}
}

byte DrawChar(char c, short dx, short dy, short minY, short maxY)
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

	//Graphics.Rectangle(dx,dy,width,height,0xFCFE);

	DUMPCHAR(c,dx);

    // TODO - font palettes!

    dy += top;
    height -= top;

    //  Clip top
    short clip = minY - dy;
    if (clip > 0)
    {
        dy += clip;
        src += width*clip;
        if (clip >= height)
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
        short e = src + width*height;
        end = min(end,e);
    }

    if (height == 0 || end <= src || dx+width > 240 || dy+height > 320)
        return width;

	Graphics.SetBounds(dx,dy,width,height);
	if (disk)
		DiskBlit(src,end-src);
	else
		BlitIndexedPP(src+font,_ppalette,end-src);	// needs PROGMEM palette form?
	return width;
}

enum MToken
{
	None = 0,
	Bold,
	Italic,
	BoldItalic,

	PlainText,
	NewLine,            // Move to next line
	BlankLine,
	HorizontalRule,

	Heading1Begin,      // 2 byte code? TODO
	Heading2Begin,
	Heading3Begin,
	Heading4Begin,
	Heading5Begin,
	Heading6Begin,

	Heading1End,
	Heading2End,
	Heading3End,
	Heading4End,
	Heading5End,
	Heading6End,

	WikiLinkBegin,      // These nest
	WikiLinkEnd,

	Glyph,
	Image
};

#ifdef DUMP_MARKUP
	const char* TokenName(MToken t)
	{
		switch (t)
		{
            case WikiLinkBegin:	return "<a>";
            case WikiLinkEnd:	return "</a>";

            case Heading1Begin:	return "<h1>";
            case Heading2Begin:	return "<h1>";
            case Heading3Begin:	return "<h1>";
            case Heading4Begin:	return "<h1>";
            case Heading5Begin:	return "<h1>";
            case Heading6Begin:	return "<h1>";

            case Heading1End:	return "</h1>";
            case Heading2End:	return "</h2>";
            case Heading3End:	return "</h3>";
            case Heading4End:	return "</h4>";
            case Heading5End:	return "</h5>";
            case Heading6End:	return "</h6>";

            case PlainText:  	return "</b>";
            case Bold:	return "<b>";
            case HorizontalRule:	return "<hr>";
            case NewLine:	return "<br/>";
            case BlankLine:	return "<p/>";
			case Glyph: return "[G]";
			}
		return "???";
	}

	void DumpTok(MToken t)
	{
		bool br = true;
		switch (t)
		{
		case PlainText:
		case Bold:
		case WikiLinkBegin:
		case WikiLinkEnd:
			br = false;
			break;
		}

		printf(TokenName(t));
		if (br)
			printf("\n");
	}
#endif

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

const char* Decompress(byte c, byte* len)
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

ROMSTRING(_keys);
const char _keys[] =
"qwertyuiop"\
"asdfghjkl"\
"zxcvbnm ";

// TODO - some kind of radix representation
//	major
//	minor
long ScrollRange(int y)
{
	while (y < 0)
		y += 320;
	while (y >= 320)
		y -= 320;
	return y;
}

class ButtonBar
{
	bool _visible;
	byte _tracking;

public:
	void Init()
	{
		_visible = 0;
	}

	void Show(long scroll)
	{
		scroll = ScrollRange(scroll + 320-20);
		DrawImageWrap(BUTTONBAR_PATH,0,scroll,20);	// Wrap around 320 boundary
		_visible = true;
	}

	bool Hide()
	{
		if (!_visible)
			return false;
		_visible = false;
		return true;	// if it was visible
	}
};

//========================================================================
//========================================================================

class Keyboard
{
public:
#define KEYBOARD_TOP 180
#define KEYBOARD_HEIGHT (320-KEYBOARD_TOP)
#define KEY_WIDTH 20 // really 22x25
#define KEY_HEIGHT 23
#define KEY_VMARGIN 40
#define KEY_VPAD 2
#define KEY_HPAD 2

#define TEXT_LEFT 16
#define TEXT_RIGHT 42
#define TEXT_TOP (KEYBOARD_TOP + 7)
#define TEXT_WIDTH (240 - (TEXT_LEFT+TEXT_RIGHT))
#define TEXT_HEIGHT 20

    bool Visible;
    bool Updated;
    char Text[16];
    signed char _tracking;

    void Init()
    {
        _tracking = -1;
        Visible = false;
        Updated = false;
        Text[0] = Text[1] = Text[2] = 0;
    }

#define BACKSPACE_KEY 26
#define CLEAR_TEXT 27

    void GetKeyRect(short index, short* left, short* top)
    {
		if (index == CLEAR_TEXT)
		{
			*left = TEXT_LEFT + TEXT_WIDTH;
			*top = TEXT_TOP;
			return;
		}

        byte row = 0;
        if (index > 9)
        {
            index -= 10;
            row++;
            if (index > 8)
            {
                index -= 9;
                row++;
            }
        }
        short center = (240 - ((KEY_WIDTH + KEY_HPAD)*(10-row))) >> 1;
        *top = KEYBOARD_TOP + KEY_VMARGIN + (KEY_HEIGHT + KEY_VPAD)*row;
        *left = center + (KEY_WIDTH + KEY_HPAD)*index;
    }

    char GetChar(byte index)
    {
        return P(_keys+index);
    }

    void HLine(short y, short color)
    {
        Graphics.Rectangle(0,y,240,1,color);
    }

    void DrawText()
    {
        short x = TEXT_LEFT + 4;
        short y = TEXT_TOP + 2;
		Graphics.Rectangle(x,y,TEXT_WIDTH,TEXT_HEIGHT,0xFFFF);
        byte i = 0;
        while (Text[i])
        {
            x += DrawChar(Text[i],x,y,y,y+TEXT_HEIGHT);
            i++;
        }
    }

    void DrawKey(int keyIndex, bool erase, bool on)
    {
        short top,left;
        GetKeyRect(keyIndex,&left,&top);
		_ppalette = on ? RedPalette : NormalPalette;
		SetFont(3);
        DrawChar(GetChar(keyIndex),left + 6,top + 4, top,top+KEY_HEIGHT);
        _ppalette = NormalPalette;
    }

    char FindKey(short x, short y)
    {
        for (char i = 0; i < 28; i++)
        {
            short left,top;
            GetKeyRect(i,&left,&top);
            if ((x >= left && x < (left + KEY_WIDTH)) && (y >= top && y < (top + KEY_HEIGHT)))
                return i;
        }
        return -1;
    }

    void Draw()
    {
		DrawImage(KEYBOARD_PATH,0,180);
		SetFont(3);
        DrawText();
        for (byte i = 0; i < 27; i++)
            DrawKey(i,true,false);
    }

    void Show(bool show)
    {
        if (show)
            Draw();
        Visible = show;
    }

    void Keydown(int index)
    {
        short t = strlen(Text);
		switch (index)
		{
			case BACKSPACE_KEY:
				Text[t-1] = 0;	// Backspace
				break;
			case CLEAR_TEXT:
				Text[0] = 0;
				break;
			default:
				Text[t] = GetChar(index);
				Text[t+1] = 0;
        }
		SetFont(3);
        DrawText();
        Updated = true;
    }

    void OnEvent(Event* e)
    {
		TouchData* touch = (TouchData*)e->Data;
        switch (e->Type)
        {
            case Event::TouchDown:
                _tracking = FindKey(touch->x,touch->y);
                if (_tracking != -1)
                    DrawKey(_tracking,false,true);
                break;

            case Event::TouchMove:
                if (_tracking != -1)
                    DrawKey(_tracking,false,_tracking == FindKey(touch->x,touch->y));
                break;

            case Event::TouchUp:
                if (_tracking != -1)
                {
                    if (_tracking == FindKey(touch->x,touch->y))
                    {
                        DrawKey(_tracking,false,false);
                        Keydown(_tracking);
                    }
                    _tracking = -1;
                }
                break;

            default:
                break;
        }
    }
};

//========================================================================
//========================================================================

class SpatialIndex
{
    File* _file;
    long _table;
    long _tableLen;
public:
    LayoutBlock L0; // top of layout range
    LayoutBlock L1; // bottom of layout range
    long MaxY;

    void Init(File& file, long table, long len)
    {
        L0.Y0 = L0.Y1 = L1.Y0 = L1.Y1 = 0;
        _file = &file;
        _table = table;
        _tableLen = len; // Might be different sizes?

        _file->SetPos(_table + _tableLen-8);
        _file->Read(&MaxY,4);
    }

    //  This could be more concise
    void Seek(LayoutBlock& l, long y)
    {
        // TODO - BINARY SEARCH
        long f[2];
        long lastY = 0;
        long lastD = 0;
        long n = _tableLen >> 3;

		_file->SetPos(_table);
        while (n--)
        {
            _file->Read(f,8);
            if ((y >= lastY && y < f[0]) || (n == 0))
                break;
            lastY = f[0];
            lastD = f[1];
        }
        l.Y0 = lastY;
        l.D0 = lastD;
        l.Y1 = f[0];
        l.D1 = f[1];
    }

    //  often L1 becomes L0 when smooth scrolling; should check L0/L1 before doing a search
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

class WikiState
{
    short _x;
    short _y;   // TODO tall?

	enum UIState
	{
		None,
		InSearch,
		InHistory,
		InPage
	};

    File _file;
    long _link;
    long _pageBlob;      // Start of page blob
    long _alphaToPageIDBlob;
    long _linkToPageIDBlob;
    long _first3ToAlphaBlob;
	long _glyphBlob;
	long _fontBlob;
	long _imageBlob;
	long _pageCount;

    // Page
    // Slice to draw
    long _markup;
    SpatialIndex _spatialIndex;

    long _trackLink;    // link we are currently tracking
    short _linkX;       // x position of click that start

    short _linkLeft;
    u8 _linkWidth;
    bool _inLink;       // Drawing tracked link
    Scroller _scroller;

    Keyboard _kb;
	ButtonBar _buttonBar;
    long _currentAlphabeticalIndex;
	bool _showButtonsPending;
	byte _uiState;

#define MAX_HISTORY 8	// power of 2 please, TODO forward
	long _history[MAX_HISTORY];
	short _histHead;
	short _histTail;

public:
    void Init()
    {
        _link = -1;
        _linkX = -1;

		_diskFile = &_file;
		_diskFont = 0xff;

        _kb.Init();
		_buttonBar.Init();
		_showButtonsPending = false;
		_currentAlphabeticalIndex = -1;
		_histHead = _histTail = 0;
		_uiState = None;
		_trackLink = 0;
		_file.Init();
		InitScroll(0);
    }

	void InitScroll(long height)
	{
		_scroller.Init(height,DrawProc,this);
	}

	static void DrawProc(long scroll, int y, int height, void* ref)
	{
		((WikiState*)ref)->Draw(scroll,y,height);
	}

#define LEFT_MARGIN 8
#define RIGHT_MARGIN 8

    void CR()
    {
        if (_x != LEFT_MARGIN)
            NewLine_();
    }

    void NewLine_()
    {
        _x = LEFT_MARGIN;
        _y += _lineHeight;
    }

    void HRule(short top, short bottom)
    {
        if (_x != LEFT_MARGIN)
            NewLine_();
        if (_y >= top && _y < bottom)
            Graphics.Rectangle(LEFT_MARGIN,_y,240-(LEFT_MARGIN+RIGHT_MARGIN),1,GREY(0xC0));
        _lineHeight = 4;
        NewLine_();
        SetFont(0); // Plain text
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

    // Get the file offset of a child blob
    long GetBlob(long parentBlob, long index, long& len)
    {
        len = 0;
        long count = Read32(parentBlob + 4);
        if (index < 0 || index >= count)
            return -1;
        long start = Read32(parentBlob + 8 + index * 4);
        long end = Read32(parentBlob + 8 + index * 4 + 4);
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

    int Open()
    {
		if (!_file.Open(PStr(DATA_FILE)))
		{
			Graphics.Rectangle(40,40,160,160,RED);
			int x = Graphics.DrawString(PStr(DATA_FILE),70,100,0xFFFF);
			Graphics.DrawString(PStr(ERROR_MSG),x,100,0xFFFF);
			for (;;)
				;
		}

        long len;
		_imageBlob =			GetBlob(0,0,len);
		_glyphBlob =			GetBlob(0,1,len);
		_fontBlob =				GetBlob(0,2,len);
        _pageBlob =				GetBlob(0,3,len);
        _alphaToPageIDBlob =	GetBlob(0,4,len);
		_pageCount = len >> 2;
        _linkToPageIDBlob =		GetBlob(0,5,len);
        _first3ToAlphaBlob =	GetBlob(0,6,len);

		Load(_diskFonts,_fontBlob,4);
		Load(_diskImages,_imageBlob,5);
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

    //  Extract the lowercase title of the article
    void GetPageTitleAlpha(int alphaIndex, char* title)
    {
        OpenPageAlpha(alphaIndex);
        BeginRead(_markup+2);
        u8 i = 0;
        while (i < 15)
        {
            u8 c = ReadByte();
            if (c < 32)
                break;
            if (c > 128)
            {
                byte len;
                const char* str = Decompress(c,&len);
                len = min(len,15-i);
                while (len--)
                {
                    c = P(str++);
                    title[i++] = tolower(c);
                }
            } else
                title[i++] = tolower(c);
        }
        title[i] = 0;
    }

    void OpenPage(long index)
    {
        long len;
        long page = GetBlob(_pageBlob, index, len);
        _markup = GetBlob(page, 0, len);    // Start of page markup
        long table = GetBlob(page, 1, len);
       _spatialIndex.Init(_file,table,len);
    }

    void OpenPageAlpha(long alphaIndex)
    {
        long id = AlphaToPageID(alphaIndex);
        OpenPage(id-1);
    }

    void DrawSlice(long y0, long y1)
    {
        _spatialIndex.Seek(y0,y1);
    }

	u32	_pos;
	u8  _mark;

	//	slightly buffer reads
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

	int DrawGlyph(long id, int top, int bottom)
	{
		//if (_trackLink != 0)
		//	return;
		long pos = _diskFile->GetPos();
		long len;
		long g = GetBlob(_glyphBlob, (int)id-1, len);
		int w = DrawGlyphFromFile(g,_x,_y,top,bottom);
		_diskFile->SetPos(pos);
		return w;
	}

	//	srcY is logical, top is mapped to physical
    void Draw(long srcY, short top, short height)
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
        _x = LEFT_MARGIN;

        //  Set up data range
        long from = _spatialIndex.L0.D0;
        long to = _spatialIndex.L1.D1;
        BeginRead(from + _markup);

		//printf("%ld to %ld, y:%ld %ld\n",from,to,_spatialIndex.L0.Y0,_spatialIndex.L1.Y1);

        //  Draw BG - not required if just tracking a link!
		if (_trackLink == 0)
			Graphics.Rectangle(0,top,240,bottom-top,0xFFFF);

        _ppalette = NormalPalette;
        short linkLeft = 0;

        //  Draw the slice
       // SetFont(0);             // Font state (and link state) must be explicitly defined at the start of each line
        short n = to - from;    // Number of bytes to draw
        _lineHeight = 12;
        while (n-- > 0)         // (or _y > bottom)
        {
            u8 c = ReadByte();
            if (c == 0)
				break;  // ?
            if (c >= 32)
            {
                if (c >= 128)
                {
                    u8 len;
                    const char* str = Decompress(c,&len);
                    while (len--)
                    {
                        _x += DrawChar(P(str), _x, _y,top, bottom);
                        str++;
                    }
                } else
                    _x += DrawChar((char)c, _x, _y,top, bottom);
            }
            else
            {
                MToken t = (MToken)c;
				//DUMPTOK(t);
                switch (t)
                {
                    case WikiLinkBegin:
                        _link = ReadId(n);
                        linkLeft = _x;
                        if (_link == _trackLink && _inLink)
                            _ppalette = RedPalette;
                        else
                            _ppalette = BluePalette;
                        break;

                    case WikiLinkEnd:
                        if (_linkX != -1)
                        {
                            if (_linkX >= linkLeft && _linkX < _x)
                                FoundLink(_link,linkLeft,_x);
                        }
                        _link = -1;
                        _ppalette = NormalPalette;
                        break;

                    case Heading1Begin:
                        CR();
                        SetFont(3); // 9
                        break;

                    case Heading2Begin:
                        CR();
                        SetFont(2); // 8
                        break;

                    case Heading3Begin:
                    case Heading4Begin:
                    case Heading5Begin:
                    case Heading6Begin:
                        CR();
                        SetFont(1); // 7
                        break;

                    case Heading1End:
                    case Heading2End:
                    case Heading3End:
                        HRule(top,bottom);
                        SetFont(0);
                        break;

                    case Heading4End:
                    case Heading5End:
                    case Heading6End:
                        NewLine_();
                        SetFont(0);
                        break;

                    case PlainText:   // This is only a style token
                        SetFont(0);
                        break;

                    case Bold:
                        SetFont(1);
                        break;

                    case HorizontalRule:
                        HRule(top,bottom);
                        break;

                    case NewLine:
                        NewLine_();
                        break;

                    case BlankLine:
                        if (_x != LEFT_MARGIN)
                            NewLine_();
                        SetFont(0);
                        NewLine_();
                        break;

					case Glyph:
						{
							long id = ReadId(n);
							_x += DrawGlyph(id,top,bottom);
						}
                    default:;
                }
            }
        }
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
        Draw(linkTop,linkTop,linkBottom-linkTop);  // Redraw
    }

    //  Locate the link, if any, at this position
    void FindLink(ushort x, ushort y)
    {
        _linkX = x;
        _trackLink = 0;
        _inLink = false;
        Draw(ToScrollY(y),0,0);    // completely clipped drawing, just looking for link
        _linkX = -1;
    }

    //  Convert a link index to a pageID
    long LinkToPageID(long link)
    {
        return Read32(_linkToPageIDBlob + ((link-1)<<2));
    }

    long AlphaToPageID(long alphaIndex)
    {
        return Read32(_alphaToPageIDBlob + (alphaIndex<<2));
    }

    //  AAA is uppercase
    long First3ToAlpha(const char* aaa, long* next)
    {
        long a = 0;
        for (u8 i = 0; i < 3; i++)
        {
            char b = *aaa++;
            if (b < 'a' || b > 'z')
                b = 0;
            else
                b -= 'a';
            a = a*26 + b;
        }
		bool atEnd = a == 17575;
        a = _first3ToAlphaBlob + (a<<2);
		if (atEnd)
			*next = _pageCount;
		else
			*next = Read32(a+4);
        return Read32(a);
    }

	//	Display a page, update history
	void ShowPage(long id)
	{
		bool inHistory = false;
		for (short i = _histTail; i < _histHead; i++)
			if (id == _history[i & ~(MAX_HISTORY-1)])
				inHistory = true;
		if (!inHistory)
			PushHistory(id);

        OpenPage(id-1);
        InitScroll(max(320,_spatialIndex.MaxY));
		_showButtonsPending = true;
		_uiState = InPage;
	}

    //  Compare text to title of page at alpha index
    short TitleCmp(const char* text, long a)
    {
        char title[16];
        GetPageTitleAlpha(a,title);
        return strcmp(text,title);
    }

    //  Scroll to find a more precise alpha match
    long RefineAlpha(const char* text, long a, long end)
    {
        long aa = a;
        short count = end - a;
        for (short i = 0; i < count; i++)   // TODO! Binary search
        {
            if (TitleCmp(text,a+i) <= 0)
				break;
			aa = a+i;
        }
        return aa;
    }

	//	Update Search position, display list of 10 closest alpha matches at top
    void DrawSearch(const char* text)
    {
		if (text[0] == 0)
		{
			DrawImage(WIKI_PATH,0,20);
			return;
		}

        long end;
        long a = First3ToAlpha(text,&end);   // First approximation of page
        if (strlen(text) > 3)
            a = RefineAlpha(text,a,end);     // Could be much more so

		a = min(a,_pageCount-8);
        _currentAlphabeticalIndex = a;

        for (short y = 0; y < 8; y++)
        {
            OpenPageAlpha(a+y);
            Draw(10,(y+1)*LINE_HEIGHT,LINE_HEIGHT);	// Draw a slice of the page (Title) as the heading
        }
    }

	void ShowHistory()
	{
		_scroller.Clear();
		DrawImage(HISTORYT_PATH,0,0);
		short h = _histHead;
		for (short y = 0; y < 14; y++)
        {
			if (h == _histTail)
				break;
			long id = _history[--h & (MAX_HISTORY-1)];
            OpenPage(id-1);
            Draw(10,(y+1)*LINE_HEIGHT,LINE_HEIGHT);	// Draw a slice of the page (Title) as the heading
        }
		_uiState = InHistory;
		ShowButtons();
	}

	void ShowSearch()
	{
		DrawImage(SEARCHT_PATH,0,0);
		DrawSearch(_kb.Text);
		_kb.Show(true);
		_uiState = InSearch;
		ShowButtons();
	}

	void PushHistory(long page)
	{
		_history[_histHead++ & (MAX_HISTORY-1)] = page;
		_histTail = max(_histTail,_histHead-MAX_HISTORY);
	}

	long PopHistory()
	{
		if (_histHead == _histTail)
			return -1;
		return _history[--_histHead & (MAX_HISTORY-1)];
	}

	void Back()
	{
		long id = PopHistory();
		id = PopHistory();
		if (id != -1)
			ShowPage(id);
	}

	void Clear()
	{
		_buttonBar.Hide();
		Graphics.Rectangle(0,0,240,320,0xFFFF);
		InitScroll(0);
		_showButtonsPending = false;
	}

	void ButtonClick(int i)
	{
		switch (i)
		{
			case 0:
				Back();
				break;
			case 1:
				Clear();
				ShowSearch();
				break;
			case 2:
				Clear();
				ShowHistory();
				break;
		}
	}

    //  Touch the keyboard or the page
    int OnEvent(Event* e)
    {
		if (e->Type == Event::OpenApp)
		{
			Init();
			if (Open() < 0)
				return -1;
			ShowSearch();
			return 0;
		}

		TouchData* touch = (TouchData*)e->Data;

		//	Keyboard touch
        if (_uiState == InSearch)
        {
			//	Selected a item from the search list
			if (e->Type == Event::TouchDown && touch->y < KEYBOARD_TOP && _currentAlphabeticalIndex != -1)
            {
                _kb.Show(false);
				short index = touch->y/LINE_HEIGHT - 1;
				long id = AlphaToPageID(_currentAlphabeticalIndex+index);
				ShowPage(id);
                return 0;
            }

            _kb.OnEvent(e);
            if (_kb.Updated)
            {
                DrawSearch(_kb.Text);
                _kb.Updated = false;
            }
            return 0;
        }

		//	Click in buttons
		if (!_showButtonsPending && touch->y > (320-20) && e->Type == Event::TouchDown)
		{
			ButtonClick(touch->x/(240/3));
			return 0;
		}

		//	Click in History
		if (_uiState == InHistory)
		{
			if (e->Type == Event::TouchDown)
			{
				short h = (_histHead - 1) - (touch->y/LINE_HEIGHT - 1);
				if (h >= _histTail && h < _histHead)
					ShowPage(_history[h & (MAX_HISTORY-1)]);
			}
			return 0;
		}


		//	Tracking touch
		//	Link
		//	Scroll
		//	Buttons
        switch (e->Type)
        {
			case Event::TouchDown:
                FindLink(touch->x,touch->y);
                TrackLink(touch->x,touch->y);
                if (!_trackLink)
				{
                    _scroller.OnEvent(e);
				}
                break;

			case Event::TouchMove:
                if (_trackLink)
                    TrackLink(touch->x,touch->y);	// TODO
                else
				{
					HideButtons();
                    _scroller.OnEvent(e);
				}
                break;

            case Event::TouchUp:
                if (_trackLink)
                {
					long id = LinkToPageID(_trackLink);
					 _trackLink = 0;
					if (id && _inLink)
						ShowPage(id);
                }
				else
				{
					_showButtonsPending = true;
                    _scroller.OnEvent(e);
				}
                break;

			case Event::None:
				_scroller.OnEvent(e);	// TODO
				break;

			// Idle
            default:
                break;
        }

		//	Scroller was moving but stopped
		if (_scroller.Stopped() && _showButtonsPending)
		{
			ShowButtons();
			_showButtonsPending = false;
		}
		return 0;
    }

	void ShowButtons()
	{
		_buttonBar.Show(_scroller.CurrentScroll());
	}

	void HideButtons()
	{
		if (_buttonBar.Hide())
		{
			long y = (320-20) + _scroller.CurrentScroll();
			Draw(y,y,20);
		}
	}
};

int WikiProc(Event* e, void* state)
{
	return ((WikiState*)state)->OnEvent(e);
}

//	Register the application, start getting events on OnEvent method
#ifdef _WIN32
INSTALL_APP(wiki,WikiState);
#else
extern "C"
int vfprintf(FILE* f, const char* c, void* v)
{
	return 0;
}
//	Installs as shell on hardware
INSTALL_APP(shell,WikiState);
#endif
