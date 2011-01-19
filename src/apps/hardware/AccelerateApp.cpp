
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

class Point
{
public:
	void Init(int pp, int minpp, int maxpp)
	{
		v = 0;
		minp = minpp << 4;
		maxp = maxpp << 4;
		p = pp << 4;
	}
	void Bounce()
	{
		if (v < 0)
			v = -v >> 1;
		else
			v = -(v >> 1);
	}
	void Step(int a)
	{
		v += a>>3;
		p += v;
		if (p < minp)
		{
			p = minp;
			Bounce();
		}
		if (p > maxp)
		{
			p = maxp;
			Bounce();
		}
	}
	int minp;
	int maxp;
	int p;
	int v;
};

#define RADIUS 16

class AccState
{
	int _y;

	Point _px;
	Point _py;

public:
	void Draw(int h, signed char s, int color)
	{
		s >>= 1; //+-32 = 1G
		Graphics.Rectangle(h + s-1,_y,3,1,color);
	}

	void Move(signed char ax, signed char ay)
	{
		int x = _px.p >> 4;
		int y = _py.p >> 4;
		_px.Step(ax);
		_py.Step(ay);
		int x2 = _px.p >> 4;
		int y2 = _py.p >> 4;
		if (x2 != x || y2 != y)
			Graphics.Circle(x,y,RADIUS,0xFFFF,false);
		Graphics.Circle(x2,y2,RADIUS,0xAAAA,false);
	}

	void DrawGrid()
	{
		for (int h = 16+32; h < 240; h += 80)
			Graphics.Rectangle(h,0,1,320,GRAY(0xC0));
	}

	void Draw()
	{
		DrawGrid();

		signed char xyz[3];
		Hardware.GetAccelerometer(xyz);

		Graphics.Rectangle(0,_y,240,4,0xFFFF);	// scan

		int h = 16+32;
		Draw(h,xyz[0],RED);
		h += 80;
		Draw(h,xyz[1],GREEN);
		h += 80;
		Draw(h,xyz[2],BLUE);
		
		Move(xyz[0],xyz[1]);

		if (++_y == 320)
			_y = 0;
	}

	void Init()
	{
		_y = 0;
		_px.Init(120,RADIUS,240-RADIUS);
		_py.Init(160,RADIUS,320-RADIUS);
		Graphics.Rectangle(0,0,240,320,0xFFFF);
	}

	int OnEvent(Event* e)
	{
		switch (e->Type)
		{
			case Event::OpenApp:
				Init();
				break;

			case Event::None:
				delay(5);
				Draw();
				break;

			case Event::TouchDown:
				return -1;

				break;
			default:
				break;
		}
		return 0;
	}
};

//	Register the application, start getting events on OnEvent method
INSTALL_APP(accelerate,AccState);