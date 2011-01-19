

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

#include "Platform.h"
#include "math3D.h"


#if 0

// a little lerping
// QQTTTTTTFFFFFFFF
//	Q: Quadrant
//	T: Table lookup
//	F: Lerp fraction
short SIN16(u16 angle)
{
	short a = SIN(angle >> 8);
	short b = SIN((angle+0xFF) >> 8);
	u8 f = angle;
	f >>= 2;
	return a*(64-f) + b*f;	// +-1 in 16 bit fixed
}

short COS16(u16 angle)
{
    return SIN16(angle + (64 << 8));
}
#endif

//  someone clever should rewrite this

void Matrix::Translate(long x, long y, long z)
{
    _m[3] += x;
    _m[7] += y;
    _m[11] += z;
}

void SC(int a, long& s, long& c)
{
    s = SIN(a);
    c = COS(a);
    c <<= 8;
    s <<= 8;
}

void Matrix::RotateX( int a )
{	
	long s,c;	
	SC(a,s,c);
	Identity();
	_m[5] = c;
	_m[6] = s;
	_m[9] = -s;
	_m[10] = c;
}

void Matrix::RotateY( int a  )
{	
	long s,c;	
	SC(a,s,c);
	Identity();
	_m[0] = c;
	_m[2] = -s;
	_m[8] = s;
	_m[10] = c;
}

void Matrix::RotateZ( int a  )
{	
	long s,c;	
	SC(a,s,c);
	Identity();
	_m[0] = c;
	_m[1] = s;
	_m[4] = -s;
	_m[5] = c;
}

//  Integer coordinates to 12:4, shifting sucks on avr
void Matrix::Transform( Vec3& v )
{	
	long ox = v.x, oy = v.y, oz = v.z;
	v.x = (ox*_m[0] + oy*_m[1] + oz*_m[2] + _m[3]) >> 12;
	v.y = (ox*_m[4] + oy*_m[5] + oz*_m[6] + _m[7]) >> 12;
	v.z = (ox*_m[8] + oy*_m[9] + oz*_m[10] + _m[11]) >> 12;
}

void Matrix::Identity()
{	
    memset(_m,0,sizeof(_m));
	_m[0] = _m[5] = _m[10] = _m[15] = FPONE;
}

//  Preserve more fidelity on the translation components of the matix
static long MLine(long* m, long* m2m, byte sa, byte sb)
{
    long n = 0;
    byte sc = 16-(sa+sb);
    for (byte jj = 0; jj < 16; jj += 4)
    {
        long a,b;   // Don't multiply by zero
        if ((a = (*m++ >> sa)) && (b = (m2m[jj] >> sb)))
            n += a*b >> sc;
    }
    return n;
}

void Matrix::Concatenate( Matrix& m2 )
{	
	byte i,j;
	Matrix res;
	for (i = 0; i < 3*4; i += 4)
	{
		for( j = 0; j < 3; j++ )
			res._m[i + j] = MLine(_m+i,m2._m + j,2,2);
	    res._m[i + 3] = MLine(_m+i,m2._m + 3,2,6);
	}
	for( j = 0; j < 4; j++ )
		res._m[12 + j] = MLine(_m+12,m2._m + j,6,6);
	memcpy(_m,res._m,sizeof(_m));
}