
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

#define _S 1    // Steel Wall
#define _W 2    // Wood wall
#define WD 3    // wood door
#define _H 4    // Hex brick wall

#define _C 5    // Clinker brick wall
#define _A 5    // Ammo
#define _B 7    // Blue wall
#define SD 6    // Steel door

extern const u8 _map[64] PROGMEM;
const u8 _map[64] =
{
    _S,_S,_S,_S,_A,_W,_W,_W,
    SD, 0, 0,_S, 0, 0, 0,WD,
    _S,_S, 0,_S, 0,_W,_W,_W,
    _A, 0, 0, 0, 0, 0, 0,_C,
    _H,_B,_B, 0,_B,_B, 0,_C,
    _H, 0, 0, 0,_B,_C, 0,WD,
    WD, 0, 0, 0, 0,SD, 0,_C,
    _H,_H,_H,_H,_H,_C,_C,_C,
};

//  Range if uv is 0..2 in 16:16
//  Always positive

//	Not all that accurate, but fast
long multfix(long a, long b)
{
	u16 aa = a >> 8;
	return aa*b >> 8;
}

long multfix16(u16 a16, long b24)
{
	b24 >>= 8;
	return (a16*b24)>>8;
}

long reciprocalfix(u16 d)
{
    u8 count = 0;
    while (((short)d) > 0)
	{
        ++count;
        d <<= 1;
	}
     
    // Single n-r iteration
    long x = 0x02EA00 - d;
	x -= d;
    x = multfix(x, 0x20000L-multfix16(d,x));
    
    if (count>0)
		x = x << count;    
    return x;  
}

//  Range if uv is 0..2 in 16:16
//  Always positive
long RECIP(u16 uv)
{
    if (uv < 4)
        return 0x3FFFFFFF;
	//return 0x80000000/(uv>>1);   // Long divide to provide 16:16 result
	return reciprocalfix(uv);
}

//  dduv is always positive 16:16 number may be very large, might resonably be trimmed
long MUL8(u8 a, long dduv)
{
    return (dduv >> 8)*a;
}

#define SCREEN_WIDTH 240
#define SCREEN_HEIGHT 320

short SIN(u8 angle);  // angle is 0..1024
short COS(u8 angle);

// a little lerping
// QQTTTTTTFFFF
//	Q: Quadrant
//	T: Table lookup
//	F: Lerp fraction
short SIN16(u16 angle)
{
	short a = SIN(angle >> 4);
	short b = SIN((angle+15) >> 4);
	u8 f = angle & 0x0F;
	return a*(16-f) + b*f;	//+-16
}

short COS16(u16 angle)
{
    return SIN16(angle + (64 << 4));
}

#define JOY_X 120
#define JOY_Y 260	// below center

class DoomState
{
    short _playerPosX;    
    short _playerPosY;
    
    u16 _angle;
    signed char _moveRate;
    signed char _turnRate;

	short _x;
	short _y;

	u8 _phase;
	u8 _allglow;
	u16 _glow;
            
    public:
    short Init()
    {
        _angle = 0x2EBA;	// a cool place on the map
        _playerPosX = 0x3AC;
        _playerPosY = 0x344;
        _moveRate = _turnRate = 0;
		_x = -1;
		_phase = 0;
		_allglow = 0;
        return 0;
    }
    
    #define PLAYERWIDTH 0x20
    u8 InWall(short dx, short dy)
    {
        dx += _playerPosX;
        dy += _playerPosY;
        u8 x0 = (dx-PLAYERWIDTH)>>8;
        u8 y0 = (dy-PLAYERWIDTH)>>8;
        u8 x1 = (dx+PLAYERWIDTH)>>8;
        u8 y1 = (dy+PLAYERWIDTH)>>8;
        while (x0 <= x1)
        {
            for (u8 y = y0; y <= y1; y++)
			{
				u8 t = pgm_read_byte(&_map[y * 8 + x0]);
				if (t)
					return t;
			}
            x0++;
        }
        return 0;
    }

    // Add a little acceleration to movement
	int damp(int i)
	{
		if (i < 0)
			return ++i;
		if (i > 0)
			i--;
		return i;
	}

	void glow()
	{
		_phase += 7;
		int c = 128 + (SIN(_phase++) >> 1);
		if (c > 255)
			c = 255;
		_glow = Graphics.ToColor(c,0,0);
		if (_allglow)
			--_allglow;
	}

    void move()
    {
		if (_x == -1)
		{
			_turnRate = damp(_turnRate);
			_moveRate = damp(_moveRate);
		} else {
			_turnRate = (JOY_X - _x) >> 2;
			_moveRate = (JOY_Y - _y) >> 2;
		}

        if (_turnRate)
            _angle += _turnRate;
        
        //  Rather dumb wall avoidance
        while (_moveRate)
        {
            short dx = ((COS16(_angle) >> 5)*_moveRate) >> 7;
            short dy = ((SIN16(_angle) >> 5)*_moveRate) >> 7;
			u8 t = InWall(dx,dy);
            if (t)
            {
				if (t == 6)
					_allglow = 0xFF;
                if (!InWall(0,dy))
                    dx = 0;
                else if (!InWall(dx,0))
                    dy = 0;
            }               
            if (!InWall(dx,dy))
            {
                _playerPosX += dx;
                _playerPosY += dy;           
                break;
            }
            _moveRate = damp(_moveRate);
        }
    }

	int MakeColor(u8 h, u8 texture)
	{
		if (texture == SD || _allglow)
			return _glow;
		if (!(texture & 0x80))
			h += h >> 1;	// 0..240 (dark is 0..160)
		h += 15;
		u8 c = h;
		u8 r = (texture & 1) ? c : c >> 2;
		u8 g = (texture & 2) ? c : c >> 1;
		u8 b = (texture & 4) ? c : 0;
		return Graphics.ToColor(r,g,b);
	}
 
    short Loop(u8 key)
    {        
        move();
		glow();

        // cast all rays here
		// then draw on a second pass

        short sina = SIN16(_angle) << 2;
        short cosa = COS16(_angle) << 2;
        short u = cosa - sina;          // Range of u/v is +- 2 TODO: Fit in 16 bit
        short v = sina + cosa;
        short du = sina / (SCREEN_WIDTH>>1);     // Range is +- 1/24 - 16:16
        short dv = -cosa / (SCREEN_WIDTH>>1);
        
		Graphics.Direction(1,1,1);
		Graphics.OpenBounds();

		u8 hs[SCREEN_WIDTH];
		u8 ts[SCREEN_WIDTH];
        for (u8 ray = 0; ray < SCREEN_WIDTH; ray++, u += du, v += dv)
        {           
            long duu = RECIP(abs(u));
            long dvv = RECIP(abs(v));
            char stepx = (u < 0) ? -1 : 1;
            char stepy = (v < 0) ? -8 : 8;

            // Initial position
            u8 mapx = _playerPosX >> 8;
            u8 mapy = _playerPosY >> 8;      
            u8 mx = _playerPosX;
            u8 my = _playerPosY;
            if (u > 0)
                mx = 0xFF-mx;
            if (v > 0)
                my = 0xFF-my;
            long distx = MUL8(mx, duu);
            long disty = MUL8(my, dvv);
                
			u8 map = mapy*8 + mapx;
            u8 texture = 0;
			for (;;)
            {
                if (distx > disty)
				{
                    // shorter distance to a hit in constant y line
					map += stepy;
                    texture = pgm_read_byte(&_map[map]);
					if (texture)
						break;
                    disty += dvv;
                } else {
                    // shorter distance to a hit in constant x line
                    map += stepx;
                    texture = pgm_read_byte(&_map[map]);
					if (texture)
						break;
                    distx += duu;
                }
            }

			//	Texture loop
            long hitdist;
            if (distx > disty)
			{
				hitdist = disty;
				if (stepy <= 0)
				{
					map >>= 3;
					texture = pgm_read_byte(&_map[map+1]);
				}
				texture |= 0x80;
            }
			else
			{
				hitdist = distx;
				if (stepx <= 0)
				{
					map &= 7;
					texture = pgm_read_byte(&_map[map+1]);
				}
            }

            //short hh = SCREEN_WIDTH*2*0x10000L/hitdist;
			short hh = reciprocalfix(hitdist >> 8) >> 16;
			if (hh > SCREEN_HEIGHT/2)
				hh = SCREEN_HEIGHT/2;
			hs[ray] = hh;
			ts[ray] = texture;
		}

		// Draw
		for (u8 ray = 0; ray < SCREEN_WIDTH; ray++)
		{
            short y1 = 0;
            short y2 = SCREEN_HEIGHT-1;
			u8 hh = hs[ray];
            if (hh < (SCREEN_HEIGHT/2))
            {
                y1 = y2 = SCREEN_HEIGHT/2;
                y1 -= hh;
                y2 += hh;
            }

			short color = MakeColor(hh,ts[ray]);

			Graphics.Fill(0,y1);
			Graphics.Fill(color,y2-y1);
			Graphics.Fill(0,SCREEN_HEIGHT-y2);
        }
		Graphics.Direction(0,1,1);
        return 0;
    }

	int OnEvent(Event* e)
	{
		switch (e->Type)
		{
			case Event::OpenApp:
				Init();
				break;

			case Event::TouchDown:
				if (e->Touch->y > 320)
					return -1;

			case Event::TouchMove:
				_x = e->Touch->x;
				_y = e->Touch->y;
				break;

			case Event::TouchUp:
				_x = -1;
				break;
			
			default:
				break;
		}
		Loop(0);
		return 0;
	}
};

//	Register the application, start getting events on OnEvent method
INSTALL_APP(doomed,DoomState);