
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

#define GREY(_x) TOCOLOR(_x,_x,_x)
short SIN(byte angle);
short COS(byte angle);

#define BUTT_RADIUS 20
#define BUTT_Y 32
#define BUTT_COUNT 3

short RGBToColor(short r, short g, short b)
{
	r = bound(r,0,255);
	g = bound(g,0,255);
	b = bound(b,0,255);
	return TOCOLOR(r,g,b);
}

class PaintState
{
	enum Mode
	{
		Chroma,
		Luma,
		Accelerometer
	};

	u8 _mode;
	u8 _phase;
	u8 _chroma;
	u8 _fade;

	short _y;
	short _u;
	short _v;
	short _colors[3];

	signed char _acc[3];

public:
	void ButtonPos(int n, int* x, int* y)
	{
		*x = (240/BUTT_COUNT)*n + 120/BUTT_COUNT;
		*y = 8 + BUTT_RADIUS;
	}

	void DrawButton(int n, int color)
	{
		int x,y;
		ButtonPos(n,&x,&y);
		Graphics.Circle(x,y,BUTT_RADIUS,color,true);
	}

	void DrawButtons()
	{
		for (u8 n = 0; n < BUTT_COUNT; n++)
			DrawButton(n,_colors[n]);
	}

	bool ClickButton(TouchData* t, int n)
	{
		int x,y;
		ButtonPos(n,&x,&y);
		if (abs(t->x-x) > BUTT_RADIUS) return false;
		return (abs(t->y-y) <= BUTT_RADIUS);
	}

	void Fade()
	{
		if (_fade < 64)
		{
			int color = 0xF800 | ((_fade << 5) & 0x07C0) | (_fade >> 1);
			DotDrawStr(PStr(Shell_AppName()),5,32,68,color,_fade == 0);
			_fade++;
		}
	}

	int OnEvent(Event* e)
	{
		switch (e->Type)
		{
			case Event::OpenApp:
				_mode = Chroma;
				_colors[0] = 0x01f0;
				_colors[1] = GREY(150);
				_colors[2] = 0xF0F0;
				Graphics.Rectangle(0,0,240,320,0xFFFF);
				DrawButtons();
				_fade = 0;	// TODO Zero init app buffer?
				break;

			case Event::None:
				Fade();
				break;

			case Event::TouchDown:
			case Event::TouchMove:
				{
					TouchData* t = e->Touch;
					int oldMode = _mode;
					if (e->Type == Event::TouchDown)
					{
						if (t->y >= 320)
							return -1;	// touched black bar quit

						for (u8 n = 0; n < 3; n++)
							if (ClickButton(t,n))
								_mode = n;
					}

					short color;
					switch (_mode)
					{
						case Chroma:
							_u = SIN(_phase) >> 1;
							_v = COS(_phase) >> 1;
							break;
						case Luma:
							_y = (SIN(_phase) + 256) >> 1;
							break;
						case Accelerometer:
							break;
					}

					//	Always read accelerometer
					Hardware.GetAccelerometer(_acc);
					int y = (u8)_acc[0];
					_colors[Accelerometer] = RGBToColor(y+_acc[1],y,y+_acc[2]);

					if (_mode == Accelerometer)
						color = _colors[Accelerometer];
					else
					{
						color = RGBToColor(_y+_u,_y,_y+_v);
						_colors[_mode] = color;
					}
					_phase++;

					if (_mode == oldMode)
					{
						int r = t->pressure>>1;
						Graphics.Circle(t->x,t->y,r,color,true);
					}
					DrawButtons();
					Fade();
				}
				break;

			default:;
		}
		return 0;
	}
};

INSTALL_APP(paint,PaintState);