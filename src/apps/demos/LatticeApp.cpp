
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

//	Raycasting image of a nice lattice

class LatticeState
{
	int _delta;
	u8 _sample;
	//int _gits;
	//int _ss;
	u16 _hitRange[8];
	u8 _palette[128*2];

	int pin(int h)
	{
	  if (h < 0)
		  return 0;
	  if (h > 31)
		  return 31;
	  return h;
	}

	short ToColor(int p)
	{
		int c = p >> 5;	// color
		p &= 0x1F;
		switch (c)
		{
		case 1:
			p = (p << 11) | p;
			break;
		case 2:
			p = (p << 6) | p;
			break;
		case 3:
			p = (p << 11) | (p << 6);
			break;
		default:
			p = (p << 11) | (p << 6) | p;
		}
		return p;
	}

	int Probe(short* o, short* d)
	{
		u8 stepshift = 7;
		u8 hit = 0;
		u8 lasthit = 0;
		u8 i;
		u8 it = 0;
		for (it = 0; it < 100; it++)
		{
			//_gits++;
			//_ss += stepshift;

			//	step
			for (i = 0; i < 3; i++)
			{
				short dd = d[i] >> stepshift;
				if (hit)
					dd = -dd;
				o[i] += dd;
			}

			short test = 0x3000;		// at 6
			//long blowup = 0x8C00 >> stepshift;
			u16 blowup = _hitRange[stepshift];

			// hit
			long ax = abs(o[0]);	// only need to be long because of integer type promotion
			long ay = abs(o[1]);
			long az = abs(o[2]);
			hit = 0;

		//	octohedron
			long t = ax+ay+az;
			if ((t - test*2) < blowup)
				hit = 1;

		//	bar
			t = abs(ax-az) + abs(ay-ax) + abs(az-ay);
		//		t = o[0]*o[0] + o[1]*o[1] + o[2]*o[2];	// ballz etc
			if (t - test < blowup)
			{
				hit = 2;
				if ((o[0] ^ o[1] ^ o[2]) & 0x8000)
					hit = 3;
			}

			if (hit)
			{
				stepshift++;		// finer
				t = 8 - (it >> 3);
				if (stepshift >= t)
				{
					int c = 31-it+stepshift;
					if (c < 0)
						c = 0;
					if (c > 31)
						c = 31;
					return c | (hit << 5);
				}
				it--;
			} else {
				if (lasthit == 0 && stepshift > 1)
					stepshift--;	// coarser
			}
			lasthit = hit;
		}
		return 0;
	}

	void Init()
	{
		_delta = 17000;
		u32 range = 0x8C00L;
		for (u8 r = 0; r < 8; r++)
		{
			_hitRange[r] = range;
			range >>= 1;
		}
		for (int i = 0; i < 256; i+=2)
		{
			int c = ToColor(i>>1);
			_palette[i] = c >> 8;
			_palette[i+1] = c;
		}
		_sample = 16;
		Graphics.Rectangle(0,0,240,320,0x7800);
	}

	int Draw(int step, int size)	// 320x240,160x128,128x128
	{
		_delta += 0x1000;
		//_gits = 0;
		//_ss = 0;

		int xx,yy;
		short d[3];
		u8 buf[240];

		int WIDTH = Graphics.Height();
		int HEIGHT = Graphics.Width();
	//	int dx = 205;
	//	int dy = 191;
		int dx = 167;
		int dy = 163;
		if (WIDTH < 240)
		{
			// 128x128, 160x128
			dx += dx;
			dy += dy;
		}
		dx *= step;
		dy *= step;

		Graphics.OpenBounds();

		u8 s;
		d[0] = -32767;	// x
		for (xx = 0; xx < WIDTH; xx += step)
		{
			d[1] = -24576;	// y
			long x = d[0];
			short x2 = x*x >> 16;
			if (_sample == 1)
			{
				Graphics.Rectangle(0,xx,240,step,0xFFFF);	// Cursor
			}
			for (yy = 0; yy < HEIGHT;)
			{
				long y = d[1];
				short y2 = y*y >> 16;
				d[2] = 21168 - x2 - y2;

				short o[3];
				o[0] = _delta+0;
				o[1] = _delta+45056;
				o[2] = _delta+24576;	// origin

				u8 pixel = Probe(o,d);
				s = step;
				while (s--)
					buf[yy++] = pixel;
				d[1] += dy;
			}
			s = step;
			while (s--)
				Graphics.BlitIndexed(buf,(u8*)_palette,HEIGHT);
			d[0] += dx;

			//	Check for cancel click
			if (((xx/step) & 7) == 0)
			{
				TouchData t;
				if (Hardware.GetTouch(&t))
				{
					while (Hardware.GetTouch(&t))
						;
					return -1;
				}
			}
		}
		return 0;
	}

public:
	int OnEvent(Event* e)
	{
		switch (e->Type)
		{
			case Event::OpenApp:
				Init();
				break;
			case Event::None:
				if (_sample > 1)
					_sample >>= 1;
				return Draw(_sample,1);
			default:
				break;
		}
		return 0;
	}
};

//	Register the application, start getting events on OnEvent method
INSTALL_APP(lattice,LatticeState);
