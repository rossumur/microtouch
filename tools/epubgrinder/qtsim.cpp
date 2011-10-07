
#include "Platform.h"
#include "LCD.h"
//#include "mmc.h"

// LCD
void LCD_::Init(){}
int LCD_::Width() { return 240; }
int LCD_::Height() { return 320; }

int _bx = 0;
int _by = 0;
int _px = 0;
int _py = 0;
int _bright = 0;
int _bbottom = 0;
int _bscroll = 0;
bool _bdirty = true;

u32* _lcdBuffer = 0;

// returns -1 if no update is required, 0..320 to indicate update + scroll factor
int UpdateLCD(u8* lcdBuffer)
{
    _lcdBuffer = (u32*)lcdBuffer;
    int r = _bdirty ? _bscroll : -1;
    _bdirty = false;
    return r;
}

u32 to32(u16 p)
{
    int b = (p & 0x1F) << 3;
    int g = (p >> 3) & 0xFC;
    int r = (p >> 8) & 0xF8;
    return 0xFF000000 | (r << 16) | (g << 8) | b;
}

void pixel(u16 pixel)
{
    int i = _px + _py*240;
    if (_lcdBuffer == 0 || i < 0 || i >= 240*320)
    {
        printf("LCD pixel pos is bad %d %d\n",_px,_py);
        return;
    }
    _bdirty = true;
    _lcdBuffer[i] = to32(pixel);
    _px++;
    if (_px == _bright)
    {
        _px = _bx;
        _py++;
        if (_py == _bbottom)
            _py = _by;
    }
}

void LCD_::Rectangle(int x, int y, int width, int height, int color)
{
    SetBounds(x,y,width,height);
    Fill(color,width*height);
}

void LCD_::SetBounds(int x, int y, int width, int height)
{
    _bx = _px = max(x,0);
    _by = _py = max(y,0);
    _bright = min(x+width,Width());
    _bbottom = min(y+height,Height());
}

void LCD_::Fill(int color, u32 count)
{
    while (count--)
        pixel(color);
}

void LCD_::Blit(const u8* data, u32 count)
{
    while (count--)
    {
        int p = data[1] + (data[0] << 8);
        data += 2;
        pixel(p);
    }
}

void LCD_::BlitIndexed(const u8* data, const u8* palette, u32 count)
{
    while (count--)
    {
        const u8* d = palette + (*data++ << 1);
        int p = d[1] + (d[0] << 8);
        pixel(p);
    }
}

void LCD_::Scroll(int y)
{
    _bscroll = y;
    _bdirty = true;
}

void LCD_::Direction(u8 landscape, u8 dx, u8 dy){}


byte MMC_Init() { return 0; }
byte MMC_ReadSector(byte *buffer, unsigned long sector) { return 0; }
byte MMC_WriteSector(byte *buffer, unsigned long sector) { return 0; }

void Hardware_::Init()
{
}

int _mpress = 0;
int _mx;
int _my;

u8 Hardware_::GetTouch(TouchData* d)
{
    d->x = _mx;
    d->y = _my;
    d->pressure = _mpress;
    return _mpress;
}

void Hardware_::GetAccelerometer(signed char *xyz)
{
    xyz[0] = 128;
    xyz[1] = 128;
    xyz[2] = 128;
}

void Hardware_::Profile(bool on)
{
}

int ReadLcdReg(u16)
{
    return 0;
}

int USBGetChar()
{
    return -1;
}

void _delay_ms(int ms)
{
}
