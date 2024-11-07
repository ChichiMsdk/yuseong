#ifndef VEC4_H
#define VEC4_H

#include <stdalign.h>
#include <mydefines.h>

typedef struct vec4
{
	alignas(16) f32	data[4];
} vec4;

typedef struct ComputePushConstant
{
	vec4 data1;
	vec4 data2;
	vec4 data3;
	vec4 data4;
} ComputePushConstant;


vec4 Vec4Get(float x, float y, float z, float w);

#endif // VEC4_H
