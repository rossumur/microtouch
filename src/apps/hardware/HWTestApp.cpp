
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
#include "Fat32.h"

ROMSTRING(S_TouchX);
ROMSTRING(S_TouchY);
ROMSTRING(S_TouchZ);
ROMSTRING(S_AccelX);
ROMSTRING(S_AccelY);
ROMSTRING(S_AccelZ);
ROMSTRING(S_MicroSD);
ROMSTRING(S_FAT);
ROMSTRING(S_FAT16);
ROMSTRING(S_FAT32);
ROMSTRING(S_NotFound);
ROMSTRING(S_Ready);
ROMSTRING(S_Battery);
ROMSTRING(S_Backlight);
ROMSTRING(S_DrawNum);

const char S_TouchX[] = "Touch X";
const char S_TouchY[] = "Touch Y";
const char S_TouchZ[] = "Touch Pressure";
const char S_AccelX[] = "Accel X";
const char S_AccelY[] = "Accel Y";
const char S_AccelZ[] = "Accel Z";
const char S_MicroSD[] = "Micro SD";
const char S_FAT[] = "FAT";
const char S_FAT16[] = "FAT16";
const char S_FAT32[] = "FAT32";
const char S_NotFound[] = "Not Found";
const char S_Ready[] = "Ready";
const char S_Battery[] = "Battery";
const char S_Backlight[] = "Backlight";
const char S_DrawNum[] = "%d";

void MillivoltToStr(char* s, int mv);

byte MMC_Init();
byte MMC_ReadSector(byte *buffer, u32 sector);
void TouchHWTest(int* config);

class Widget
{
public:
	const char*	name;
	char text[16];
	int	 value;
	bool sign;
	bool dirty;

	void Init(const char* n, bool s = false)
	{
		name = n;
		sign = s;
		dirty = true;
		value = 0;
		text[0] = 0;
	}

	void Set(int v)
	{
		if (v != value)
			dirty = true;
		value = v;
	}

	void Set(const char* s)
	{
		strcpy(text,s);
		dirty = true;
	}

	void DrawNum(int v, int x, int y)
	{
		char s[16];
		sprintf_P(s,S_DrawNum,v);
		DrawText(s,x,y);
	}

	void DrawText(const char* s, int x, int y)
	{
		Graphics.Rectangle(x,y,100,16,0xFFFF);
		Graphics.DrawString(s,x+2,y+2,0);
	}

	#define TEXT_COLOR ((0x80 << 8) | (0x80 << 3) | (0x80 >> 3))

	void Draw(int y)
	{
		char str[32];
		strcpy(str,PStr(name));	// Ah prog memory

		int x = 100 - Graphics.MeasureString(str,strlen(str));
		Graphics.DrawString(str,x,y+2,TEXT_COLOR);

		if (text[0])
			DrawText(text,100+2,y);
		else
			DrawNum(value,100+2,y);
		dirty = false;
	}
};

#define WIDGET_COUNT 10
#define BLRECT_WIDTH 128
#define BLRECT_LEFT 102
#define BLRECT_TOP (32+9*16)
#define BLRECT_HEIGHT 16

class HWTestState
{
	int _fade;
	u32	_lastTicks;
	Widget _widgets[WIDGET_COUNT];

public:
	void Init()
	{
		memset(this,0,sizeof(*this));

		Graphics.Rectangle(0,0,240,320,0xFFFF);
		_widgets[0].Init(S_TouchX);
		_widgets[1].Init(S_TouchY);
		_widgets[2].Init(S_TouchZ);
		_widgets[3].Init(S_AccelX,true);
		_widgets[4].Init(S_AccelY,true);
		_widgets[5].Init(S_AccelZ,true);

		_widgets[6].Init(S_MicroSD);
		_widgets[7].Init(S_FAT);
		_widgets[6].Set(PStr(S_NotFound));
		_widgets[7].Set(PStr(S_NotFound));

		_widgets[8].Init(S_Battery);
		_widgets[9].Init(S_Backlight);
		Update();
	}

	void Update()
	{
		int y = 32;
		for (int i = 0; i < WIDGET_COUNT; i++)
		{
			if (_widgets[i].dirty)
				_widgets[i].Draw(y);
			y += 16;
		}
	}

	void CheckFS()
	{
		u8 microSD = MMC_Init();	// Called repeatedly
		if (microSD == 0)
		{
			_widgets[6].Set(PStr(S_Ready));
			if (_widgets[7].value == 0)
			{
				u8 buffer[512];
				u8 fat = FAT_Init(buffer,MMC_ReadSector);
				const char* s;
				_widgets[7].value = 1;
				switch (fat)
				{
					case 16:	s = S_FAT16;	break;
					case 32:	s = S_FAT32;	break;
					default:
						s = S_NotFound;
						_widgets[7].value = 0;
				}
				_widgets[7].Set(PStr(s));
			}
		} else {
			_widgets[6].Set(PStr(S_NotFound));
			_widgets[7].Set(PStr(S_NotFound));
			_widgets[7].value = 0;
		}
	}

	void Battery()
	{
		char s[16];
		MillivoltToStr(s,Hardware.GetBatteryMillivolts());
		_widgets[8].Set(s);
	}

	void Sample()
	{
		signed char xyz[3];
		u32 t = Hardware.GetTicks();
		if (t - _lastTicks > 1000)
		{
			Hardware.GetAccelerometer(xyz);
			_widgets[3].Set(xyz[0]);
			_widgets[4].Set(xyz[1]);
			_widgets[5].Set(xyz[2]);
			Battery();
			CheckFS();
			_lastTicks = t;
		}
		Update();
	}

	void DrawBacklightRect(u8 value)
	{
		u8 x = value>>1;
		Graphics.Rectangle(BLRECT_LEFT,BLRECT_TOP,x,BLRECT_HEIGHT,GRAY(0xF0));
		Graphics.Rectangle(BLRECT_LEFT+x,BLRECT_TOP,128-x,BLRECT_HEIGHT,GRAY(0xA0));
	}

	int OnEvent(Event* e)
	{
		switch (e->Type)
		{
			case Event::OpenApp:
				Init();
				DrawBacklightRect(Hardware.GetBacklight());
				_fade = 0;
				break;

			case Event::None:
				if (_fade < 64)
				{
					int color = 0xF800 | ((_fade << 5) & 0x07C0) | (_fade >> 1);
					DotDrawStr(PStr(Shell_AppName()),6,16,68,color,_fade == 0);
					_fade++;
				}
				Sample();
				break;

			case Event::TouchMove:
			case Event::TouchDown:
			case Event::TouchUp:
				{
					TouchData* t = (TouchData*)e->Data;
					if (e->Type == Event::TouchDown)
					{
						if (t->y >= 320)
							return -1;	// touched black bar quit
					}
					_widgets[0].Set(t->x);
					_widgets[1].Set(t->y);
					_widgets[2].Set(t->pressure);
					Sample();

					if (e->Type == Event::TouchMove)
					{
						int bl = (t->x - BLRECT_LEFT) << 1;
						if (bl < 0)
							bl = 0;
						else if (bl > 255)
							bl = 255;
						Hardware.SetBacklight(bl,500);
						DrawBacklightRect(bl);
					}
				}
				break;
			default:
				break;
		}
		return 0;
	}
};

//	Register the application, start getting events on OnEvent method
INSTALL_APP(hwtest,HWTestState);
