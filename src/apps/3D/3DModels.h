
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

//===============================================================================
//===============================================================================

//  Icosohedron
//  Triangles
Triangle _icosTriangle[20] =
{
    {0,8,4,0},
    {0,5,10,1},
    {2,4,9,2},
    {2,11,5,3},
    {1,6,8,4},
    {1,10,7,5},
    {3,9,6,6},
    {3,7,11,7},
    {0,10,8,8},
    {1,8,10,9},
    {2,9,11,10},
    {3,11,9,11},
    {4,2,0,12},
    {5,0,2,13},
    {6,1,3,14},
    {7,3,1,15},
    {8,6,4,16},
    {9,4,6,17},
    {10,5,7,18},
    {11,7,5,19}
};

#define fU 8
#define fV (int)(0.618*fU + 0.5)
Vec3Char _icosVertex[12] =
{
    {fU,fV,0},
    {-fU,fV,0},
    {fU,-fV,0},
    {-fU,-fV,0},
    {fV,0,fU},
    {fV,0,-fU},
    {-fV,0,fU},
    {-fV,0,-fU},
    {0,fU,fV},
    {0,-fU,fV},
    {0,fU,-fV},
    {0,-fU,-fV}
};
#undef fU
#undef fV

//	Tetrahedron
#undef fU
#define fU 5

Vec3Char _tetrahedronVertex[4] = {
	{ fU, fU, fU},
	{-fU,-fU,fU},
	{-fU,fU,-fU},
	{fU,-fU,-fU}
};

Triangle _tetrahedronTriangle[4] = {
	{2,1,0, 1	},
	{1,3,0, 2	},
	{3,1,2, 3	},
	{2,0,3, 4	}
};

#undef fU
#define fU 8

Vec3Char _octahedronVertex[6] = 
{
	{fU,0,0},
	{-fU,0,0},
	{0,fU,0},
	{0,-fU,0},
	{0,0,fU},
	{0,0,-fU}
};

Triangle _octahedronTriangle[8] = 
{
  {2,4,0, 1},
  {1,4,2, 2},
  {3,4,1, 3},
  {0,4,3, 4},

  {5,2,0, 5},
  {5,1,2, 6},
  {5,3,1, 7},
  {5,0,3, 8}
};

#undef fU
#define fU 5

Vec3Char _cubeVertex[8] = {
	{-fU,-fU,-fU},
	{ fU,-fU,-fU},
	{-fU, fU,-fU},
	{ fU, fU,-fU},
	
	{-fU,-fU, fU},
	{ fU,-fU, fU},
	{-fU, fU, fU},
	{ fU, fU, fU}
};
    
//     45
//  01 67
//  23

Triangle _cubeTriangle[12] = {
	{1, 0, 2, 1},   // Front
	{2, 3, 1, 1},
	{4, 5, 7, 2},   // Back
	{7, 6, 4, 2},
	
	{7, 3, 2, 3},
	{2, 6, 7, 3},
	{1, 5, 4, 4},
	{4, 0, 1, 4},
	
	{4, 6, 2, 5},
	{2, 0, 4, 5},
	{1, 3, 7, 6},
	{7, 5, 1, 6},
};

typedef struct
{
	const Vec3Char*	Vertexes;
	const Triangle*	Triangles;
	int VertexCount;
	int	TriangleCount;
} Model;

const Model _tetrahedron =
{
	_tetrahedronVertex,
	_tetrahedronTriangle,
	4,
	4
};

const Model _octahedron =
{
	_octahedronVertex,
	_octahedronTriangle,
	6,
	8
};

const Model _cube =
{
	_cubeVertex,
	_cubeTriangle,
	8,
	12
};

const Model _icosohedron =
{
	_icosVertex,
	_icosTriangle,
	12,
	20
};