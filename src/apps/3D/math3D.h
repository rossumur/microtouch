#ifndef MATH3D_H
#define MATH3D_H

#include <math.h>
#include <stdio.h>

short SIN(byte angle);
short COS(byte angle);

//  word version
typedef struct
{
    short x;
    short y;
    short z;
} Vec3;

//  Compact version for storing models
typedef struct
{
    signed char x;
    signed char y;
    signed char z;
} Vec3Char;

#define FPONE		65536
#define FPP			16
#define FPI(x)		((x)<<FPP)

class Matrix
{
public:
	Matrix () { Identity (); }
	void Identity();
	void Transform(Vec3& v);
	void RotateX(int angle);
	void RotateY(int angle);
	void RotateZ(int angle);
	void Translate(long x, long y, long z);
	void Concatenate(Matrix& m2);
private:
	long _m[16];
};

#endif
