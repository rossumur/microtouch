
#ifndef __LCD_H__
#define __LCD_H__

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

//	Interface to LCD driver
//	Should only be called from graphics

class LCD_
{
public:
static	void Init();
static	int Width();
static	int Height();
static void Rectangle(int x, int y, int width, int height, int color);
static void SetBounds(int x, int y, int width, int height);
static void Fill(int color, u32 count);
static void Blit(const u8* data, u32 count);
static void BlitIndexed(const u8* data, const u8* palette, u32 count);
static void Scroll(int y);
static void Direction(u8 landscape, u8 dx = 1, u8 dy = 1);
};
extern LCD_ LCD;

#endif