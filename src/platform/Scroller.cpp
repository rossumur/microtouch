
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
#include "Scroller.h"

#define GREY(_x) (((_x & 0xF8) << 8) | ((_x & 0xFC) << 3) | ((_x & 0xF8) >> 3))
extern const short _greys[8] PROGMEM; 
const short _greys[8] = 
{
    GREY(0x60),GREY(0x70),GREY(0x80),GREY(0x90),GREY(0xA0),GREY(0xB0),GREY(0xC0),GREY(0xD0)
};  // dark to light

short Mod320(long y)
{
    short m = y % 320;
    if (m < 0)
        m += 320;
    return m;
}

//  Fill above and below the image
void FillScrollBG(long y, int height, int t)
{
    long end = y + height;
    while (y < end)
    {
        short c = abs(t-y);
        c = min(7,c);
        Graphics.Rectangle(0,Mod320(y++),240,1,pgm_read_word(_greys+c));
    }
}

//  Encapsulate fancy scrolling
void Scroller::Init(long height, ScrollDrawProc drawProc, void* ref, int pageSize)
{
    _drawProc = drawProc;
    _ref = ref;
    _velocity = 0;
    _scroll = 0;
    _scrollHeight = height;
    _pageSize = pageSize;
	_dragy = -1;
    Graphics.Scroll(0);
    if (height > 0)
        _drawProc(0,0,min(320,height),_ref);
}

void Scroller::Clear(int color)
{
    Graphics.Clear(color);
}

void Scroller::SetHeight(long height)
{
    if (height == _scrollHeight)
		return;
    long delta = height - _scrollHeight;
    _scrollHeight = height;

    if (delta > 0)
    {
        long top = _scrollHeight-delta-_scroll;
        if (_scrollHeight < 320)
            delta = 320-top;
        Invalidate(top,min(320,delta));	// Draw new area
    }
    else
        Invalidate(_scrollHeight,min(320,-delta));		// Erase exposed area
}

int Scroller::OnEvent(Event* e)
{
	TouchData* t = (TouchData*)e->Data;
	switch (e->Type)
	{
		case Event::TouchDown:
			_dragy = t->y;	// Drag in progress
			break;

		case Event::TouchMove:
			if (_dragy != -1 && _dragy != (short)t->y)
			{
				int delta = -_dragy + t->y;
				if (delta >= -2 && delta <= 2)
					break;
				ScrollBy(delta);        // Also velocity
				_dragy = t->y;
				_velocity = delta << 4; // 12:4 fixed point
			}
			break;

		case Event::TouchUp:
			_dragy = -1;
			break;

		case Event::None:
			AutoScroll();
			break;

		default:;
	}
	return 0;
}

void Scroller::Invalidate(long src, int lines)
{
    if ((src + _scroll >= 320) || (src + _scroll + lines < 0))
        return;

    // Scroll first
    Graphics.Scroll(Mod320(-_scroll));
    
    //  Draw whitespace before image
    if (src < 0)
    {
        int c = min(lines,_scroll);
        FillScrollBG(src,c,-1);
        src += c;
        lines -= c;
    }
        
    short imgLines = lines;
    if (_scrollHeight-src < lines)
        imgLines = _scrollHeight-src;
    if (imgLines > 0)
    {
        DrawBody(src,imgLines);
        src += imgLines;
        lines -= imgLines;
    }
                    
    //  Draw whitespace after image
    if (lines > 0)
        FillScrollBG(src,lines,_scrollHeight);
}

//
void Scroller::DrawBody(long y, int height)
{
    long py = y+_scroll;
    if (py >= 320 || py + height <= 0)
        return;
    int top = Mod320(y);
    int bottom = top + height;
    if (bottom > 320)	 // Wrap across screen bounds from bottom to top
    {
        int h1 = bottom-320;
        int h0 = height-h1;
        DrawBody(y,h0);		// Draw in two slices as this crosses physical bounds of the screen
        DrawBody(y+h0,h1);
        return;
    }
	_drawProc(y,top,height,_ref);
}
//  
void Scroller::ScrollBy(long delta)
{
    if (delta == 0)
        return;
    _scroll += delta;
    if (delta < 0)
    {
        if (delta <= -320)
            Invalidate(-_scroll,320);
        else
            Invalidate(-_scroll+320+delta,-delta);
    }
    else
        Invalidate(-_scroll,min(320,delta));
}

void Scroller::ScrollTo(long scroll)
{
    ScrollBy(-scroll - _scroll);
}

// round away from zero
inline short round(short x, short f)
{
    if (x == 0)
        return 0;
    if (x < 0)
		x -= f-1;
	else
		x += f-1;
    return x/f;
}

//	Different page sizes
int Scroller::Acceleration()
{
	int a = 0;  // acceleration
    if (_pageSize)
    {
        int scrollTarget = ((_scroll - (_pageSize>>1))/_pageSize)*_pageSize;
        scrollTarget = max(-_scrollHeight + 320,scrollTarget);  
		if (_scrollHeight < 320)
			scrollTarget = 0;
        a = scrollTarget - _scroll;
     } else {
		if (_scroll > 0)
			a = -_scroll;
		else {
			int minScroll = -_scrollHeight + 320;
			if (_scrollHeight < 320)
				minScroll = 0;
			if (_scroll < minScroll)
				a = minScroll - _scroll;
		}
    }
    return a;
}

bool Scroller::Stopped()
{
	return Acceleration() == 0 && _velocity == 0;
}

//  Scroll towards page boundaries or to visible part of page
void Scroller::AutoScroll()
{
   // if (_velocity == 0)
  //      return;

	if (_dragy != -1)
	{
		_velocity >>= 1;	// bleed off
		return;
	}
    
    //  Apply drag in the opposite direction to velocity
	int a = Acceleration();
    a = round(a-_velocity,4);      
    _velocity += a;
	if (_velocity)
		ScrollBy(round(_velocity,16));    // Also velocity
}
