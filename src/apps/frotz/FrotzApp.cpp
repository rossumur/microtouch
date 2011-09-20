
/* Copyright (c) 2010, Peter Barrett  
**  
** Permission to use, copy, modify, and/or distribute this software for  
** any purpose with or without fee is hereby granted, provided that the  
** above copyright notice and this permission notice appear in all copies.  
**  
** THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL  
** WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED  
** WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR  
** BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES  
** OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,  
** WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,  
** ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS  
** SOFTWARE.  
*/

#include "Platform.h"
#include "Scroller.h"

// Enable for Pretty fonts and a binary >28k
//#define FROTZ_CLEARTYPE

typedef unsigned char zchar;
typedef unsigned char zbyte;
typedef unsigned short zword;


extern "C"
{
	void z_show_status();
};

#define LEFT_MARGIN 4
#define TOP_MARGIN 4
#define RIGHT_MARGIN (240-5)
#define LINE_HEIGHT 12

#ifdef FROTZ_CLEARTYPE
#include "FrotzFonts.h"
const u8* _ff = FSans7;	// font
const u8* _fpalette = FNormalPalette;
#endif

#define P(__x) pgm_read_byte(__x)
#define PW(__x) pgm_read_word(__x)
#define M(__x) *((u8*)(__x))
#define MW(__x) *((u16*)(__x))

extern const zchar zscii_to_latin1_t[] PROGMEM;
const zchar zscii_to_latin1_t[] =
{
    0xe4, 0xf6, 0xfc, 0xc4, 0xd6, 0xdc, 0xdf, 0xab,
    0xbb, 0xeb, 0xef, 0xff, 0xcb, 0xcf, 0xe1, 0xe9,
    0xed, 0xf3, 0xfa, 0xfd, 0xc1, 0xc9, 0xcd, 0xd3,
    0xda, 0xdd, 0xe0, 0xe8, 0xec, 0xf2, 0xf9, 0xc0,
    0xc8, 0xcc, 0xd2, 0xd9, 0xe2, 0xea, 0xee, 0xf4,
    0xfb, 0xc2, 0xca, 0xce, 0xd4, 0xdb, 0xe5, 0xc5,
    0xf8, 0xd8, 0xe3, 0xf1, 0xf5, 0xc3, 0xd1, 0xd5,
    0xe6, 0xc6, 0xe7, 0xc7, 0xfe, 0xf0, 0xde, 0xd0,
    0xa3, 0x00, 0x00, 0xa1, 0xbf
};

extern "C"
zchar zscii_to_latin1(zbyte c)
{
	return P(zscii_to_latin1_t + c);
}
#if 0


#endif

static void BlitIndexedPP(const u8* pgm, const u8* pgmp, int len)
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

#ifdef FROTZ_CLEARTYPE
static byte DrawChar(char c, short dx, short dy, short minY, short maxY)
{
	const u8* font = _ff;
//	bool disk = font == _diskFontHeader;
	u8 width,height,top;
	short src,end,i;

	if (0)
		;
#if 0
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
#endif
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
	//if (disk)
	//	DiskBlit(src,end-src);
	//else
		BlitIndexedPP(src+font,_fpalette,end-src);	// needs PROGMEM palette form?
	return width;
}
#endif

int MeasureString(const char* c, int len)
{
#ifndef FROTZ_CLEARTYPE
	return Graphics.MeasureString(c,len);
#else
	int x = 0;
	while (len--)
		x += DrawChar(*c++,0,0,0,0);
	return x;
#endif
}

int DrawString(const char* c, int len, short dx, short dy)
{
	#ifndef FROTZ_CLEARTYPE
		//Graphics.DrawString(c,len,dx+1,dy+1,TOCOLOR(0xD0,0xD0,0xD0));
		return Graphics.DrawString(c,len,dx,dy,TOCOLOR(0x30,0x30,0x30));
	#else
		while (len--)
			dx += DrawChar(*c++,dx,dy,0,320);
		return dx;
	#endif
}

//=====================================================================
//=====================================================================
//	Scrollback buffer
//	128 bytes for scrollback lines

#define SCROLLMAXLINES 64
u16 _t = 0;
u16 _inputMark = 0;
u16 _lineStarts[SCROLLMAXLINES];

long _lines = 0;
long _lineScroll = -1;

extern "C"
u8* Scrollback(zword p, bool write);

char ReadText(u16 t)
{
	return *Scrollback(t & 0x7FFF,false);
}

void StoreText(const char* t, u8 len)
{
	while (len--)
		*Scrollback(_t++ & 0x7FFF,true) = (u8)*t++;
}

void StoreTextNewLine()
{
	_lineStarts[_lines++ & (SCROLLMAXLINES-1)] = _t;
}

int StoreTextRange(u16 line, u16* start)
{
	u16 a = 0;
	if (line)
		a = _lineStarts[(line-1) & (SCROLLMAXLINES-1)];
	u16 b;
	if (_lines == line)
		b = _t;
	else
		b = _lineStarts[line & (SCROLLMAXLINES-1)];
	*start = a;
	return b-a;
}

void DrawLine(int line)
{
	int y = line + 1;
	y *= LINE_HEIGHT;
	Graphics.Rectangle(LEFT_MARGIN,y,RIGHT_MARGIN-LEFT_MARGIN,LINE_HEIGHT,0xFFFF);

	u8 x = LEFT_MARGIN;
	line += _lineScroll;
	if (line >= 0 && line <= _lines)
	{
		u16 start;
		u8 len = StoreTextRange(line,&start);
		while (len--)
		{
			char c = ReadText(start++);
			x = DrawString(&c,1,x,y);
		}
	}
}

u8 _cx = LEFT_MARGIN;
u8 _cx0;

char _wbuf[16];
u8 _wmark = 0;
u8 _window = 0;

void NewLine()
{
	_cx = LEFT_MARGIN;
	StoreTextNewLine();
}

void FlushWord()
{
	if (_wmark == 0)
		return;
	int w = MeasureString(_wbuf,_wmark);
	if (_window == 0 && w + _cx > RIGHT_MARGIN)
		NewLine();
	_cx += w;
	if (_window == 0)
		StoreText(_wbuf,_wmark);
	else
	{
		int x = _cx-w;
		Graphics.Rectangle(x,0,RIGHT_MARGIN-x,LINE_HEIGHT-1,0xFFFF);
		DrawString(_wbuf,_wmark,x,0);
	}
	_wmark = 0;
}

extern "C"
void set_status_x(int x)
{
	FlushWord();
	_cx = x;
}

extern "C"
void set_window (zword win)
{
	FlushWord();
	_window = win;
	//printf("set_window(%d)\n",win);
	if (win == 7)
	{
		_cx0 = _cx;
		_cx = LEFT_MARGIN;
	} else {
		_cx = _cx0;
	}
}

void DrawChar(char c)
{
	//printf("%c",c);
	if (c == '\n')
	{
		FlushWord();
		NewLine();
		return;
	}
	_wbuf[_wmark++] = c;
	if (c == ' ' || _wmark == sizeof(_wbuf))
		FlushWord();
}

extern "C"
{

#define ZC_NEW_STYLE 0x01
#define ZC_NEW_FONT 0x02

void flush_buffer()
{
}

zchar _cflag = 0;
void print_char (zchar c)
{
	if (_cflag)
	{
		_cflag = 0;	// change mode
		return;
	}

	if (c == ZC_NEW_STYLE)
	{
		_cflag = 1;
	}
	else if (c == ZC_NEW_FONT)
	{
		_cflag = 2;
	}
	else
		DrawChar((char)c);
}

void new_line()
{
	print_char('\n');
}

void deadmeat()
{
	//printf("Death\n");
	Graphics.Clear(0xF800);
	for (;;)
		;
}

void erase_window (zword win)
{
}

void erase_screen(zword win)
{
}

void z_sound_effect()
{
}

u8 _inputPhase = 0;
zbyte InputReady()
{
	if (_inputPhase == 1)
	{
		_inputPhase = 2;
		NewLine();
		return 1;
	}
	return 0;
}

zchar console_read_input(int maxLen, zchar *buf, zword timeout, bool continued)
{
	u16 len = _t - _inputMark;
	if (--maxLen > (int)len)
		maxLen = len;
	while (len--)
		*buf++ = ReadText(_inputMark++);
	buf[0] = 0;
	_inputMark = 0;
	return 0x0D;
}

zchar stream_read_input(int max, zchar *buf,
			  zword timeout, zword routine,
			  bool hot_keys,
			  bool no_scripting )
{
    return console_read_input (max, buf, timeout, true);
}

void z_input_stream()
{
}
void z_output_stream()
{
}

zchar stream_read_key ( zword timeout, zword routine, bool hot_keys )
{
	return 0x0D;
}
void 	os_fatal (const char *){};
void 	os_restart_game (int) {};
void 	os_init_screen (void){};
void 	os_reset_screen (void) {};
int		os_random_seed (void){return 0;};

typedef unsigned short zword;

//	stack is
//	2k - 8x256 pages
//	main mem is 
//	256k 1024 pages

void frotzInit();
int interpret();

}

#define VISIBLE_LINES 16

//================================================================
//================================================================
//	Keyboard

extern const char _kb[] PROGMEM;
extern const char _kb2[] PROGMEM;
const char _kb[] = 
"qwertyuiop"// 10
"asdfghjkl"	// 9
"zxcvbnm";	// 7
const char _kb2[] = 
"1234567890"	// 10
"-/:;()$@\""	// 9
".,?!'+=";		// 7

#define KEY_WIDTH 18
#define KEY_HEIGHT 22
#define KEY_PAD 3

#define KEY_TOP_MARGIN 12
#define KEY_LEFT_MARGIN_0 (240 - ((KEY_WIDTH + KEY_PAD)*10 - KEY_PAD))/2	//   qwertyuiop
#define KEY_LEFT_MARGIN_1 (240 - ((KEY_WIDTH + KEY_PAD)*9 - KEY_PAD))/2		//    asdfghjkl
#define KEY_LEFT_MARGIN_2 (240 - ((KEY_WIDTH + KEY_PAD)*7 - KEY_PAD))/2		// 1.5 zxcvbnm 1.5
#define KEY_LEFT_MARGIN_3 KEY_LEFT_MARGIN_0									// 2.5  space  2.5

#define KEYBOARD_TOP	((16+1)*LINE_HEIGHT)
#define KEYBOARD_HEIGHT (320 - KEYBOARD_TOP)
#define TEXTAREA_HEIGHT (KEYBOARD_TOP - LINE_HEIGHT)

#define KEY_NONE		0

#define KEY_LOOK		27
#define KEY_BACKSPACE	28

#define KEY_NUMBERS		29
#define KEY_SPACE		30
#define KEY_ENTER		31

#define KEY_COUNT		32

u8 _keyMode = 0;
static char ToChar(u8 key)
{
	return pgm_read_byte((_keyMode ? _kb2 : _kb) + key-1);
}

#define LOOK_WIDTH ((1.5*(KEY_WIDTH+KEY_PAD)-KEY_PAD)+1)
#define ENTER_WIDTH ((2.5*(KEY_WIDTH+KEY_PAD)-KEY_PAD)+1)
#define SPACE_WIDTH (5*(KEY_WIDTH+KEY_PAD)-KEY_PAD)

#define LINE2 KEY_TOP_MARGIN + 2*(KEY_HEIGHT + KEY_PAD)
#define LINE3 KEY_TOP_MARGIN + 3*(KEY_HEIGHT + KEY_PAD)

extern const u8 _keypos[] PROGMEM;
const u8 _keypos[] = {
	//	KEY_LOOK
	LINE2,
	KEY_LEFT_MARGIN_0,
	LOOK_WIDTH,

	//	KEY_BACKSPACE
	LINE2,
	KEY_LEFT_MARGIN_0 + LOOK_WIDTH + KEY_PAD + 7*(KEY_WIDTH+KEY_PAD),
	LOOK_WIDTH,

	//	KEY_NUMBER
	LINE3,
	KEY_LEFT_MARGIN_3,
	ENTER_WIDTH,

	//	KEY_SPACE
	LINE3,
	KEY_LEFT_MARGIN_3+ENTER_WIDTH+KEY_PAD,
	SPACE_WIDTH,

	//	KEY_ENTER
	LINE3,
	KEY_LEFT_MARGIN_3+ENTER_WIDTH+KEY_PAD+SPACE_WIDTH+KEY_PAD,
	ENTER_WIDTH
};

//	Keyboard def

static u8 ToRect(u8 key, u8* kx, u8* ky)
{
	if (key >= KEY_LOOK)
	{
		key -= KEY_LOOK;
		key += key + key;
		*ky = pgm_read_byte(_keypos+key++);
		*kx = pgm_read_byte(_keypos+key++);
		return pgm_read_byte(_keypos+key);
	}

	*kx = KEY_LEFT_MARGIN_0;
	u8 x = key-1;
	u8 y = 0;
	if (x >= 19)
	{
		x -= 19;
		y = 2;
		*kx = KEY_LEFT_MARGIN_2;
	}
	else if (x >= 10)
	{
		x -= 10;
		y = 1;
		*kx = KEY_LEFT_MARGIN_1;
	}
	*kx += x*(KEY_WIDTH + KEY_PAD);
	*ky = KEY_TOP_MARGIN + y*(KEY_HEIGHT + KEY_PAD);
	return KEY_WIDTH;
}

static u8 ToKey(TouchData* t)
{
	for (u8 i = 1; i < KEY_COUNT; i++)
	{
		u8 kx,ky;
		u8 w = ToRect(i,&kx,&ky);
		if ((t->y >= KEYBOARD_TOP + ky) && (t->y < KEYBOARD_TOP + ky + KEY_HEIGHT) && (t->x < kx + w) && (t->x >= kx))
			return i;
	}
	return KEY_NONE;
}

static void KeyOutline(u8 x, u8 y, u8 width, bool hilite)
{
	int c = TOCOLOR(0xA0,0xA0,0xA0);
	if (hilite)
		c = 0;
	Graphics.Rectangle(x,KEYBOARD_TOP+y-1,width,1,c);
	Graphics.Rectangle(x,KEYBOARD_TOP+y+KEY_HEIGHT,width,1,c);
	Graphics.Rectangle(x-1,KEYBOARD_TOP+y,1,KEY_HEIGHT,c);
	Graphics.Rectangle(x+width,KEYBOARD_TOP+y,1,KEY_HEIGHT,c);
}

extern const char S_look[] PROGMEM;
extern const char S_backspace[] PROGMEM;
extern const char S_123[] PROGMEM;
extern const char S_abc[] PROGMEM;
extern const char S_space[] PROGMEM;
extern const char S_enter[] PROGMEM;
extern const char S_Loading[] PROGMEM;

const char S_look[] = "look";
const char S_backspace[] = "<";
const char S_123[] = "123";
const char S_abc[] = "abc";
const char S_space[] = "space";
const char S_enter[] = "enter";
const char S_Loading[] = "Loading..";

static void DrawKey(u8 key, bool hilite)
{
	u8 kx,ky;
	u8 w = ToRect(key,&kx,&ky);
	int bg = key <= 26 ? 0xFFFF : TOCOLOR(0xF0,0xF0,0xFF);
	Graphics.Rectangle(kx,KEYBOARD_TOP+ky,w,KEY_HEIGHT,hilite ? TOCOLOR(0xD0,0xFF,0xD0) : bg);
	KeyOutline(kx,ky,w,hilite);

	char buf[8];
	const char* b = 0;
	buf[0] = ToChar(key);
	buf[1] = 0;
	switch (key)
	{
		case KEY_LOOK:		b = S_look;			break;
		case KEY_BACKSPACE:	b = S_backspace;	break;
		case KEY_NUMBERS:	b = _keyMode ? S_abc : S_123;		break;
		case KEY_SPACE:		b = S_space;	break;
		case KEY_ENTER:		b = S_enter;	break;
	}
	if (b)
		strcpy_P(buf,b);
	u8 len = strlen(buf);
	u8 m = MeasureString(buf,len);
	DrawString(buf,len,kx + ((w-m)>>1),KEYBOARD_TOP+ky+5);
}

//================================================================
//================================================================


void OpenZFile(u8* cachemem, int size);

void ShowStatus(const char* s)
{
	set_window(7);
	while (*s)
		print_char(*s++);
	set_window(0);
}

class FrotzState
{
	u8 _cachemem[750];
	u8  _tracking;
	int _yTrack;
	long _yScroll;

public:

	void Redraw()
	{
		for (int i = 0; i < VISIBLE_LINES; i++)
			DrawLine(i);
	}

	void ScrollRange(long* from, long* to)
	{
		*to = _lines - (VISIBLE_LINES-1);
		*from = max(0,_lines-(SCROLLMAXLINES-1));
	}

	void DrawScrollBar(int pos, int thumb)
	{
		int y = 2 + LINE_HEIGHT;
		Graphics.Rectangle(RIGHT_MARGIN,y,4,pos,0xFFFF);
		y += pos;
		Graphics.Rectangle(RIGHT_MARGIN,y,4,thumb,TOCOLOR(0xFF,0xC0,0xC0));
		y += thumb;
		Graphics.Rectangle(RIGHT_MARGIN,y,4,KEYBOARD_TOP-y,0xFFFF);
	}

	void DrawScrollBar(bool erase = false)
	{
		long from,to;
		ScrollRange(&from,&to);
		short d = to - from;	// range
		if (d < 0 || erase)
		{
			Graphics.Rectangle(RIGHT_MARGIN,LINE_HEIGHT,4,TEXTAREA_HEIGHT,0xFFFF);
			return;
		}
		short thumb = VISIBLE_LINES*(TEXTAREA_HEIGHT-4)/(d+VISIBLE_LINES);
		short pos = ((int)(_lineScroll-from))*((TEXTAREA_HEIGHT-4)-thumb)/d;
		DrawScrollBar(pos,thumb);
	}

	void SetScroll(long line)
	{
		if (line == _lineScroll)
			return;
		_lineScroll = line;
		Redraw();
	}

	void ScrollTo(long line)
	{
		long from,to;
		ScrollRange(&from,&to);
		if (line < from)
			line = from;
		else if (line > to)
			line = to;
		SetScroll(line);
	}

	void Update()
	{
		if (_inputMark)		// TODO bug
			return;
		FlushWord();
		ScrollTo(_lines);	// Show bottom
		_inputMark = _t;
		z_show_status();
	}

	void DrawKB()
	{
		Graphics.Rectangle(0,LINE_HEIGHT-1,240,1,TOCOLOR(0xC0,0xC0,0xC0));
		for (u8 x = 0; x < KEYBOARD_HEIGHT; x++)
		{
			u8 c = 240-x;
			if (!x)
				c = 0xA0;
			Graphics.Rectangle(0,x+KEYBOARD_TOP,240,1,TOCOLOR(c,c,c+15));
		}
		for (u8 i = 1; i < KEY_COUNT; i++)
			DrawKey(i,false);
	}

	void FlushChar(char c)
	{
		print_char(c);
		FlushWord();
		ScrollTo(_lines);
		DrawLine(VISIBLE_LINES-1);
	}

	void OnKey(u8 key)
	{
		if (key == KEY_NUMBERS)
		{
			_keyMode ^= 1;
			DrawKB();
			return;
		}

		if (key == KEY_LOOK)
		{
			FlushChar('l');	// look
			_inputPhase = 1;
			return;
		}
		
		if (key == KEY_BACKSPACE)
		{
			if (_t > _inputMark)
			{
				_t--;
				DrawLine(VISIBLE_LINES-1);
			}
			return;
		}
		
		if (key == KEY_ENTER)
		{
			_inputPhase = 1;
			return;
		}

		if (key == KEY_SPACE)
		{
			FlushChar(' ');
			return;
		}

		FlushChar(ToChar(key));
	}

	int OnEvent(Event* e)
	{
		switch (e->Type)
		{
			case Event::OpenApp:
				{
					Graphics.Clear(0xFFFF);
					_tracking = 0;
					_yTrack = -1;
					DrawKB();
					ShowStatus(PStr(S_Loading));
					OpenZFile(_cachemem,sizeof(_cachemem));
					frotzInit();
				}
				break;

			case Event::TouchDown:
				{
					TouchData* t = e->Touch;
					//if (t->y >= 320)
					//	return -1;	// touched black bar quit

					_tracking = ToKey(t);
					if (_tracking)
						DrawKey(_tracking,true);

					else if (t->y < KEYBOARD_TOP && _lines > VISIBLE_LINES)
					{
						_yScroll = _lineScroll;
						_yTrack = t->y;			// 
					}
				}
				break;

			case Event::TouchMove:
			case Event::TouchUp:
				if (_tracking != 0)
				{
					u8 t = ToKey(e->Touch);
					bool on = t == _tracking;
					if (e->Type == Event::TouchUp)
					{
						DrawKey(_tracking,false);
						if (on)
							OnKey(_tracking);
						_tracking = false;
					} else
						DrawKey(_tracking,on);
				}
				else if (_yTrack != -1)
				{
					if (e->Type == Event::TouchUp)
					{
						DrawScrollBar(true);
						_yTrack = -1;
					}
					else
					{
						int d = (_yTrack - e->Touch->y)/LINE_HEIGHT;
						ScrollTo(_yScroll + d);
						DrawScrollBar();
					}
				}
				break;

			case Event::None:
				{
					Hardware.Profile(true);
					if (interpret() == 2)	// READ
						Update();
					Hardware.Profile(false);
				}
				break;
			default:;
		}
		return 0;
	}
};

//	Register the application, start getting events on OnEvent method

INSTALL_APP(frotz,FrotzState);