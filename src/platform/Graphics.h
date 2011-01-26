
/* Copyright (c) 2009, Peter Barrett  
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

class File;
class Graphics_
{
public:
	static void Init();
	static int	Height();
	static int	Width();

	static void	Clear(int color = 0xFFFF);
	static void	Rectangle(int x, int y, int width, int height, int color);
	static void	Circle(int cx, int cy, int radius, int color, bool fill);

	static int	MeasureString(const char* s, int len);
	static int	DrawString(const char* s, int len, int x, int y, int color);
	static int	DrawString(const char* s, int x, int y, int color);

	static void	SetPixel(int x, int y, int color);

	static void	DrawImage(File& file, int x, int y, int scroll = 0, int lines = 0);

	//	Low level direct graphics
	static void	OpenBounds();
	static void SetBounds(int x, int y, int width, int height);
	static void Fill(int color, u32 count);
	static void Blit(const u8* data, u32 count);
	static void BlitIndexed(const u8* data, const u8* palette, u32 count);
	static void Scroll(int y);
	static void Direction(u8 landscape, u8 dx = 1, u8 dy = 1);

	static u16	ToColor(u8 r, u8 g, u8 b);
};

extern Graphics_ Graphics;

#define TOCOLOR(_r,_g,_b) ((((_r) & 0xF8) << 8) | (((_g) & 0xFC) << 3) | (((_b) & 0xF8) >> 3))
#define GRAY(_g) (TOCOLOR(_g,_g,_g))
#define RED		(TOCOLOR(0xFF,0x00,0x00))
#define GREEN	(TOCOLOR(0x00,0xFF,0x00))
#define BLUE	(TOCOLOR(0x00,0x00,0xFF))

typedef struct {
	byte sig[4];
	long hdrSize;
	long width;
	long height;
	byte format;
	byte reserved0;
	byte colors;
	byte restartInterval;
	long reserved1;
} Img2;

// Nasty dot string hack
void DotDrawStr(const char* s, int len, int x, int y, int dotColor, bool erase, int bgColor = -1);
