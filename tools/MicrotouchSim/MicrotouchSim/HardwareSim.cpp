
#include "Platform.h"
#include "LCD.h"
#include "math.h"

#include <deque>
using namespace std;

u8 _screen[320*240*4];		// ARGB
int _hardwareScroll = 0;	//	Scroll
bool _screenDirty;
deque<int> _getchar;

//  h and v bounds
int _h0 = 0;
int _h1 = 240;
int _v0 = 0;
int _v1 = 320;

//  GRAM current position
int _gh = 0;
int _gv = 0;
int _landscape = 0;

int _mousedown = 0;
int _mousepressure = 24;
int _mousex = 0;
int _mousey = 0;

int _backlight = 0x100;

#define SCREEN_LEFT 16
#define SCREEN_TOP 16


int Backlight(int p)
{
	return (p*_backlight)>>8;
}

u8 MMC_ReadSector(u8* buffer, u32 sector)
{
	return 1;
}

u8 MMC_WriteSector(u8* buffer, u32 sector)
{
	return 1;
}

u8 MMC_Init()
{
	return 1;
}

void Blit32(HDC dc, u8* bits,
	int xDest, int yDest, int cx, int cy, 
	int xSource, int ySource)
{
	LPBITMAPINFO lpbi;

	char buf[sizeof(BITMAPINFOHEADER)];
	lpbi = (LPBITMAPINFO)buf;

	lpbi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	lpbi->bmiHeader.biWidth = cx;
	lpbi->bmiHeader.biHeight = cy;
	lpbi->bmiHeader.biPlanes = 1;
	lpbi->bmiHeader.biBitCount = 32;
	lpbi->bmiHeader.biCompression = BI_RGB;
	lpbi->bmiHeader.biSizeImage = 0;
	lpbi->bmiHeader.biXPelsPerMeter = 0;
	lpbi->bmiHeader.biYPelsPerMeter = 0;
	lpbi->bmiHeader.biClrUsed = 0;
	lpbi->bmiHeader.biClrImportant = 0;

	StretchDIBits(dc,xDest,yDest+cy-1,cx,-cy,xSource,ySource,cx,cy,bits,lpbi,DIB_RGB_COLORS,SRCCOPY);
}

void WritePixel(int x, int y, int color)
{
	int width = LCD.Width();
	int height = LCD.Height();

	if (x < 0 || x >= width || y < 0 || y >= height)
		return;
	byte* dst = _screen + (y*width + x)*4;
    int r = (color >> 11) & 0x1F;
    int g = (color >> 5) & 0x3F;
    int b = color & 0x1F;
    dst[2] = (r << 3) | (r >> 2);
    dst[1] = (g << 2) | (g >> 4);
    dst[0] = (b << 3) | (b >> 2);
    dst[3] = 0xFF;
    _screenDirty = true;
}

void LCD_Update(HDC dc, bool force)
{
    if (_screenDirty || force)
    {
        int width = LCD.Width();
        int height = LCD.Height();
        if (_hardwareScroll == 0)
            Blit32(dc,_screen,SCREEN_LEFT,SCREEN_TOP,width,height,0,0);
        else {
            int split = _hardwareScroll;
            Blit32(dc,_screen+split*width*4,SCREEN_LEFT,SCREEN_TOP,width,320-split,0,0);
            Blit32(dc,_screen,SCREEN_LEFT,SCREEN_TOP + 320-split,width,split,0,0);   
        }
    }
    _screenDirty = false;
}


//======================================================================================================
//======================================================================================================

void LCD_::Init()
{
}

void LCD_::SetBounds(int x, int y, int width, int height)
{
    ASSERT(x >= 0 && (x + width <= (int)Width()) && y >= 0 && (y + height <= (int)Height()) && width > 0 && height > 0);
	_h0 = x;
	_h1 = x + width;
	_v0 = y;
	_v1 = y + height;
	_gh = x;
	_gv = y;
}

void LCD_::Scroll(int y)
{
    while (y < 0)
        y += 320;
    while (y >= 320)
        y -= 320;
	if (y != _hardwareScroll)
	{
		_hardwareScroll = y;
		_screenDirty = true;
	}
}

void LCD_::Direction(u8 landscape, u8 dx, u8 dy)
{
	_landscape = landscape;
}

int LCD_::Width()
{
    return 240;
}

int LCD_::Height()
{
    return 320;
}

void Pixel(int p)
{
	if (!_landscape)
	{
		WritePixel(_gh++,_gv,p);
		if (_gh == _h1)
		{
			_gh = _h0;
			if (++_gv == _v1)
				_gv = _v0;
		}
	}
	else
	{
		WritePixel(_gh,_gv++,p);
		if (_gv == _v1)
		{
			_gv = _v0;
			if (++_gh == _h1)
				_gh = _h0;  // V wrap too?
		}
	}
}

void LCD_::Blit(const u8* d, u32 count)
{
	while (count--)
	{
		int a = *d++;
		int b = *d++;
		Pixel((a << 8) | b);
	}
}

void LCD_::BlitIndexed(const u8* d, const u8* palette, u32 count)
{
	while (count--)
	{
		int p = *d++ << 1;
		int a = palette[p];
		int b = palette[p + 1];
		Pixel((a << 8) | b);
	}
}

void LCD_::Fill(int color, u32 count)
{
	while (count--)
		Pixel(color);
}

//======================================================================================================
//======================================================================================================

float _counter = 0;	// Simulate some changing values
unsigned __int64 _freq;
void Hardware_::Init()
{
	QueryPerformanceFrequency((LARGE_INTEGER*)&_freq);
}

u32 Hardware_::GetTicks()
{
	unsigned __int64 t;
	QueryPerformanceCounter((LARGE_INTEGER*)&t);
	t = t*_freq/1000;
	return t;
}

int Hardware_::GetBatteryMillivolts()
{
	return 4200;
}

u8 Hardware_::GetBacklight()
{
	return _backlight-1;
}

void Hardware_::SetBacklight(u8 level, int ms)
{
	_backlight = level + 1;// TODO
}

bool USBConnected()
{
	return true;
}

int SimGetChar();
int USBGetChar()
{
	return SimGetChar();
}

void Hardware_::PowerOff()
{
}

void Hardware_::GetAccelerometer(signed char* xyz)
{
	xyz[0] = sin(_counter) * 80;
	xyz[1] = sin(_counter/3) * 80;
	xyz[2] = sin(_counter/5) * 80;
	_counter += 0.03;
}

void Hardware_::Profile(bool on)
{
}

u16 ReadLcdReg(u16 s)
{
	return 0;
}

void _delay_ms(int ms)
{
	::Sleep(ms);
}

u8 Hardware_::GetTouch(TouchData* t)
{
	t->pressure = _mousedown*_mousepressure;
	t->x = _mousex - SCREEN_LEFT;
	t->y = _mousey - SCREEN_TOP;
	return t->pressure;
}

//======================================================================================================
//======================================================================================================

RECT _simBounds = { SCREEN_LEFT, SCREEN_TOP, 240 + SCREEN_LEFT, 320 + SCREEN_TOP};

DWORD WINAPI ThreadProc(LPVOID lpParameter)
{
	Hardware.Init();
	Shell_Init();
	for (;;)
	{
		Shell_Loop();
		delay(1);	// Gross approximation
	}
}

void SimInit(HWND hWnd)
{
	memset(_screen,0xAA,sizeof(_screen));
	DWORD tid;
	::CreateThread(0,0,ThreadProc,0,0,&tid);
	::SetTimer(hWnd,1,30,0);
}

void SimPaint(HDC hDC)
{
	RECT b = _simBounds;
	::InflateRect(&b,1,1);
	::FrameRect(hDC,&b,(HBRUSH)GetStockObject(LTGRAY_BRUSH));
	LCD_Update(hDC,true);
}

void SetConsole(const char* s);
void SimTimer(HWND hWnd)
{
	if (_screenDirty)
		::InvalidateRect(hWnd,&_simBounds,0);
}
