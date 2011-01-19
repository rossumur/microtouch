
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

int GetBatteryMV();

ROMSTRING(S_VFormat);
const char S_VFormat[] = "%d.%03dV";

void MillivoltToStr(char* s, int mv)
{
	sprintf_P(s,S_VFormat,mv/1000,mv%1000);
}

class BatteryState
{
	int _count;
	u32 _lastTicks;

	int ToPixel(int mv)
	{
		return Graphics.Height() - ((mv-2500)>>3);	//
	}

	void DrawVoltage(int mv, int x, int y, int color)
	{
		char s[16];
		MillivoltToStr(s,mv);
		Graphics.DrawString(s,x,y,color);
	}

	void Line(int mv)
	{
		int y = ToPixel(mv);
		int color = GRAY(0xC0);
		Graphics.Rectangle(0,y,Graphics.Width(),1,color);
		DrawVoltage(mv,4,y-10,color);
	}

	int GetColor(int mv)
	{
		int a = (mv-2500)>>3;
		if (a < 0)
			a = 0;
		else if (a > 255)
			a = 255;	// constrain util TODO
		return Graphics.ToColor((255-a),a,0);
	}

	void Sample()
	{
		int mv = Hardware.GetBatteryMillivolts();
		int s = ToPixel(mv);
		int color = GetColor(mv);
		Graphics.Rectangle(_count,s-1,1,2,color);
		_count++;
		if (_count == Graphics.Width())
			_count = 0;
		Graphics.Rectangle(180,20,40,20,0xFFFF);
		DrawVoltage(mv,184,24,color);
	}

public:
	int OnEvent(Event* e)
	{
		switch (e->Type)
		{
			case Event::OpenApp:
				Graphics.Clear(0xFFFF);
				Line(4500);
				Line(4000);
				Line(3500);
				Line(3000);
				_count = 0;
				_lastTicks = 0;
				break;

			case Event::None:
				{
					u32 t = Hardware.GetTicks();
					if (t - _lastTicks > 200)
					{
						Sample();
						_lastTicks = t;	 // ~1 minute per screen
					}
				}
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
				}
				break;
			default:
				break;
		}
		return 0;
	}
};

//	Application definition
INSTALL_APP(battery,BatteryState);