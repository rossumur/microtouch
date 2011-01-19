
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

ROMSTRING(S_boom);
const char S_boom[] = " boom";

ROMSTRING(S_sweet);
const char S_sweet[] = "sweet";

class MinehuntState
{
#define CELL_H 12
#define CELL_V 15
#define LEFT_MARGIN 24
#define TOP_MARGIN 24
#define CELL_SIZE ((240-2*LEFT_MARGIN)/CELL_H)

#define DIRTY 0x80
#define MINE 0x40
#define REVEALED 0x20

	u8	_cells[CELL_H*CELL_V];
	u8	_fade;
	int	_mineCount;
	int _remaining;
	int _tracking;

	char _title[16];

public:
	u8 GetCell(int x, int y)
	{
		if (x < 0 || x >= CELL_H) return 0;
		if (y < 0 || y >= CELL_V) return 0;
		return _cells[x + y * CELL_H];
	}

	int CountNeighbors(int x, int y)
	{
		int count = 0;
		for (int yy = y-1; yy <= y+1; yy++)
			for (int xx = x-1; xx <= x+1; xx++)
				if (GetCell(xx,yy) & MINE)
					count++;
		return count;
	}

	void DrawCell(int i, bool hilite = false)
	{
		int x = i % CELL_H;
		int y = i / CELL_H;

		u8 c = _cells[i];
		int n = 0;
		int color = TOCOLOR(0xF0,0x40,0xE0);
		if (c & REVEALED)
		{
			color = TOCOLOR(0xFF,0xFF,0x00);
			if (c & MINE)
			{
				n = -1;
			} else {
				n = CountNeighbors(x,y);
			}
		}

		if (hilite)
			color = RED;
		x = LEFT_MARGIN + x*CELL_SIZE;
		y = TOP_MARGIN + y*CELL_SIZE;
		Graphics.Rectangle(x+2,y+2,CELL_SIZE-4,CELL_SIZE-4,color);

		if (c & REVEALED)
		{
			if (n == -1)
			{
				int r = CELL_SIZE/2;
				Graphics.Circle(x + r,y + r,r-4,RED,true);
			}
			else if (n > 0)
			{
				char m = '0' + n;
				Graphics.DrawString(&m,1,x+6,y+3,0x2132);
			}
		}
	}

	int ToCell(TouchData* t)
	{
		int x = (t->x - LEFT_MARGIN)/CELL_SIZE;
		if (x < 0 || x >= CELL_H) return -1;
		int y = (t->y - TOP_MARGIN)/CELL_SIZE;
		if (y < 0 || y >= CELL_V) return -1;
		return x + y*CELL_H;
	}

	void Clicked(int x, int y)
	{
		if (x < 0 || x >= CELL_H) return;
		if (y < 0 || y >= CELL_V) return;
		u8 c = GetCell(x,y);
		if (c & REVEALED)
			return;				// Already revealed

		int i = x + y*CELL_H;
		_cells[i] |= REVEALED;
		DrawCell(i);			// reveal cell

		if (--_remaining == 0)
		{
			_mineCount += 3;	// make it harder
			strcpy_P(_title,S_sweet);
			_fade = 0;
			return;
		}

		if (c & MINE)
		{
			i = CELL_H*CELL_V;
			while (i--)
			{
				_cells[i] |= REVEALED;
				DrawCell(i);
			}
			_remaining = 0;
			strcpy_P(_title,S_boom);
			_fade = 0;	
			return;	// Boom

		} else {
			if (CountNeighbors(x,y) != 0)
				return;
			for (int yy = y-1; yy <= y+1; yy++)
				for (int xx = x-1; xx <= x+1; xx++)
				{
					c = GetCell(xx,yy);
					if (!(c & MINE))
						Clicked(xx,yy);
				}
		}
	}

	void NextLevel()
	{
		_remaining = CELL_H*CELL_V - _mineCount;
		_tracking = -1;

		memset(_cells,0,sizeof(_cells));
		for (int i = 0; i < _mineCount;)
		{
			u16 r = RandomBits(7);
			r %= CELL_H*CELL_V;
			if ((_cells[r] & MINE) == 0)
			{
				_cells[r] |= MINE;
				i++;
			}
		}

		Graphics.Clear(0xFFFF);
		u8 c = CELL_H*CELL_V;
		while (c--)
			DrawCell(c);
		strcpy_P(_title,Shell_AppName());
		_fade = 0;	// TODO Zero init app buffer?
	}

	int OnEvent(Event* e)
	{
		TouchData* t = (TouchData*)e->Data;
		switch (e->Type)
		{
			case Event::OpenApp:
				_mineCount = 15;
				NextLevel();
				break;

			case Event::None:
				if (_fade < 64)//
				{
					int color = 0xF800 | ((_fade << 5) & 0x07C0) | (_fade >> 1);
					DotDrawStr(_title,8,24,68,color,_fade == 0);
					_fade++;
				}
				break;

			case Event::TouchDown:
				if (t->y >= 320)
					return -1;	// touched black bar quit

				if (_remaining == 0)
				{
					NextLevel();
					return 0;
				}

				_tracking = ToCell(t);
				if (_tracking != -1)
				{
					if (_cells[_tracking] & REVEALED)
						_tracking = -1;
					else
						DrawCell(_tracking,true);
				}
				break;

			case Event::TouchMove:
				if (_tracking != -1)
				{
					bool on = ToCell(t) == _tracking;
					DrawCell(_tracking,on);
				}
				break;

			case Event::TouchUp:
				if (_tracking != -1)
				{
					if (_tracking == ToCell(t))
					{
						int x = _tracking % CELL_H;
						int y = _tracking / CELL_H;
						Clicked(x,y);
					}
					_tracking = -1;
				}
				break;

			default:;
		}
		return 0;
	}
};

//	Register the application, start getting events on OnEvent method
INSTALL_APP(mines,MinehuntState);