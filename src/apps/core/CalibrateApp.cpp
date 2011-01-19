
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

void TouchLast(TouchData& t);
void TouchCalibrate(int* config);

bool InCircle(TouchData* t, int x, int y, int r)
{
	x = x-t->x;
	y = y-t->y;
	return (x*x + y*y <= r*r);
}

class CalibrateState
{
	int _attempts;
	int _phase;
	int _color;
	int _x;
	int _y;

	int _x0;
	int _y0;
	int _ymid;
	int _xmid;
	int _x1;
	int _y1;

	int _fade;

public:
	void DrawDot(bool done)
	{
		int r = 8;
		if (done)
			_color = 0;
		int c = _color++ & 0x1F;
		c <<= 11;
		if (_color == 0)
			c = -1;	// White
		if (_color == -1)
			c = 0x07E0;
		Graphics.Circle(_x,_y,r,c,!done);
		if (!done && _color != -1)
			Graphics.Circle(_x,_y,2,-1,true);
		delay(20);
	}

	int mapp(int p, int src, int dst)
	{
		return (long)p*(long)dst/src;
	}

	bool Valid()
	{
		int c[4];
		int dx = (_x1-_x0+4)>>3;
		int dy = (_y1-_y0+4)>>3;
		c[0] = _x0 - dx;
		c[1] = _y0 - dy;	// zero
		c[2] = _x1 + dx;
		c[3] = _y1 + dy;

		int mx = mapp(_xmid-c[0],c[2]-c[0],240);
		int my = mapp(_ymid-c[1],c[3]-c[1],320);

		if (abs(mx-120) > 3 || abs(my-160) > 3)	// Needs to be self consistent
			return false;

		TouchCalibrate(c);
		return true;
	}

	bool Ok(TouchData* t)
	{
		return InCircle(t,_x,_y,6);
	}

	void Init()
	{
		_phase = 0;
		_color = 0;
		_x = 24;
		_y = 32;
		Graphics.Clear(0xFFFF);
		_attempts++;
	}

	int OnEvent(Event* e)
	{
		switch (e->Type)
		{
			case Event::OpenApp:
				_attempts = 0;
				Init();
				_fade = 0;
				break;

			case Event::None:
				if (_fade < 64)
				{
					int color = 0xF800 | ((_fade << 5) & 0x07C0) | (_fade >> 1);
					DotDrawStr(PStr(Shell_AppName()),9,16,68,color,_fade == 0);
					_fade++;
				}
				DrawDot(false);
				break;

			case Event::TouchMove:
				if (_phase == 3)
				{
					TouchData* t = (TouchData*)e->Data;
					Graphics.Circle(t->x,t->y,2,RED,true);
				}
				break;

			case Event::TouchDown:
				{
					TouchData wait,last;
					TouchLast(last);

					if (_phase != 3)
					{
						_color = -2;
						DrawDot(false);	// green
						while (Hardware.GetTouch(&wait));
					}

					_color = -1;
					DrawDot(false);
					DrawDot(true);

					switch (_phase)
					{
						case 0:
							_x0 = last.x;
							_y0 = last.y;
							_x = 240-24;
							_y = 320/2;
							break;
						case 1:
							_x1= last.x;
							_ymid = last.y;
							_x = 240/2;
							_y = 320-32;
							break;
						case 2:
							_xmid = last.x;
							_y1 = last.y;
							_y = 320/2;
							if (!Valid())
							{
								if (_attempts > 5)
									return -1;	// Can't seem to calibrate?
								Init();	// try again
								return 0;
							}
							break;

						case 3:
							if (Ok((TouchData*)e->Data))
								return -1;	// DONE
							return 0;
						default:
							;
					}
					_phase++;
				}
				break;
			default:
				break;
		}
		return 0;
	}
};

//	Register the application, start getting events on OnEvent method
INSTALL_APP(calibrate,CalibrateState);