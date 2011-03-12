
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
//	Flip Game

#define STEPS 8
#define RADIUS 20
#define LEFT ((240-(RADIUS*2*4))/2)
#define TOP ((320-(RADIUS*2*5))/2)

#define COLORA TOCOLOR(0xF5,0xB8,0x00)
#define COLORB TOCOLOR(0x33,0x66,0xFF)


class FlipState
{
	u8 _steps[STEPS];
	u8 _phase[20];
	u8 _state[20];
	int _counter;
	u16 _dotColor;	// eww. global

public:
	void DrawNum(int num, int x, int y)
	{
#define DIGITS 3
		char s[DIGITS];	//
		int d = DIGITS;
		while (d--)
		{
			s[d] = '0' + (num % 10);
			num /= 10;
		}
		DotDrawStr(s,DIGITS,x,y,_dotColor,true);
	}

	void S(u8 x, u8 y)
	{
		if (x < 0 || x >= 4 || y < 0 || y >= 5)
			return;
		u8 i = x + y*4;
		_state[i] ^= 1;
		_phase[i] = 0;
	}

	void Flip(u8 i)
	{
		u8 x = i & 3;
		u8 y = i >> 2;
		S(x,y-1);
		S(x-1,y);
		S(x,y);
		S(x+1,y);
		S(x,y+1);
	}

	signed char ToDot(int x, int y)
	{
		x -= LEFT;
		if (x < 0)
			return -1;
		y -= TOP;
		if (y < 0)
			return -1;

		x /= RADIUS*2;
		y /= RADIUS*2;
		x += y*4;
		if (x < 0 || x >= 20)
			return -1;
		return x;
	}

	void Draw(u8 i, u8 r,int color)
	{
		int x = i & 3;
		int y = i >> 2;
		x = RADIUS + x*RADIUS*2;
		y = RADIUS + y*RADIUS*2;
		Graphics.Circle(x+LEFT,y+TOP,r,color,true);
	}

	void Reset()
	{
		_dotColor = RED;
		_counter = 0;	
		u8 i = 20;
		while (i--)
			_phase[i] = _state[i] = 0;

		i = STEPS;
		while (i--)
		{
			_steps[i] = RandomBits(5);
			_steps[i] %= 20;
			Flip(_steps[i]);
		}
		DrawNum(0,3,3);
	}

	int OnEvent(Event* e)
	{
		switch (e->Type)
		{
			case Event::OpenApp:
				{
					Reset();
					DotDrawStr(PStr(Shell_AppName()),4,42,68,_dotColor,true);
				}
				break;

			case Event::TouchDown:
				{
					TouchData* t = (TouchData*)e->Data;
					if (t->y >= 320)
						return -1;	// touched black bar quit

					int f = ToDot(t->x,t->y);
					if (f != -1)
					{
						Flip(f);

						u8 c = 0;
						u8 i = 20;
						while (i--)
							if (_state[i])
								c++;

						_dotColor = (c == 0 || c == 20) ? GREEN : RED;	// won?

						DrawNum(++_counter,3,3);
					} else if (t->y < 40 && t->x < 100)
						Reset();
				}
				break;

			case Event::None:
				{
					u8 i = 20;
					while (i--)
					{
						if (_phase[i] < RADIUS)
						{
							_phase[i]++;
							Draw(i,_phase[i],_state[i] ? COLORA : COLORB);
						}
					}
				}
				break;
			default:;
		}
		return 0;
	}
};

//	Register the application, start getting events on OnEvent method
INSTALL_APP(flip,FlipState);
