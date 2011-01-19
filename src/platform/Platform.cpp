
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


long _lfsr = 0x8BADF00D;
u32 RandomBits(u8 bits)
{
	long t = bits;
	while (bits--)
	{
		t = _lfsr << 1;
		if (t <= 0)
			t ^= 0x20AA95B5;
		_lfsr = t;
	}
	return t;
}

extern const byte _trig[64] PROGMEM;
const byte _trig[64] = 
{
    0,6,13,19,25,31,37,44,
    50,56,62,68,74,80,86,92,
    98,103,109,115,120,126,131,136,
    142,147,152,157,162,167,171,176,
    180,185,189,193,197,201,205,208,
    212,215,219,222,225,228,231,233,
    236,238,240,242,244,246,247,249,
    250,251,252,253,254,254,255,255
};

short SIN(byte angle)
{
    if ((angle & 0x7F) == 0)
        return 0;
    byte b = angle & 0x3F;
    if (angle & 0x40)
        b = 0x3F - b;
    int i = pgm_read_byte(_trig+b) + 1;
    if (angle & 0x80)
        return -i;
    return i;
}

short COS(byte angle)
{
    return SIN(angle + 64);
}


//	Keep shell alive while buring
void delay(ushort ms)
{
    while (ms)
    {
        _delay_ms(1);
//		HarwareLoopIfRequired();
        ms--;
    }
}

ROMSTRING(ASSERTPrint);
const char ASSERTPrint[] = "ASSERT: %s:%d";
void assertFailed(const char* str, int line)
{
	printf_P(ASSERTPrint,str,line);
    for (;;);
}
