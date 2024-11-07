#include "vec4.h"

vec4
Vec4Get(float x, float y, float z, float w)
{
	vec4 a;
	a.data[0] = x;
	a.data[1] = y;
	a.data[2] = z;
	a.data[3] = w;
	return a; 
}
