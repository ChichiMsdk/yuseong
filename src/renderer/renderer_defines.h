#ifndef RENDERER_DEFINES_H
#define RENDERER_DEFINES_H

#include "mydefines.h"

typedef struct ColorInt
{
	int32_t r; int32_t g; int32_t b; int32_t a;
}ColorInt;

typedef struct RgbaFloat
{
	f32 r; f32 g; f32 b; f32 a;
}RgbaFloat;

typedef struct RectFloat
{
	f32 x; f32 y;
	f32 w; f32 h;
}RectFloat;

typedef struct RectInt
{
	int32_t x; int32_t y;
	int32_t w; int32_t h;
}RectInt;

#define YRED (RgbaFloat){.r = 255.0f, .g = 0.0f, .b = 0.0f, .a = 1.0f}
#define YRED_DARK (RgbaFloat){.r = 139.0f, .g = 0.0f, .b = 0.0f, .a = 1.0f}

#define YGREEN (RgbaFloat){.r = 0.0f, .g = 255.0f, .b = 0.0f, .a = 1.0f}
#define YGREEN_DARK (RgbaFloat){.r = 0.0f, .g = 139.0f, .b = 0.0f, .a = 1.0f}

#define YBLUE (RgbaFloat){.r = 0.0f, .g = 0.0f, .b = 255.0f, .a = 1.0f}
#define YBLUE_DARK (RgbaFloat){.r = 0.0f, .g = 0.0f, .b = 139.0f, .a = 1.0f}

#define SCALE_RGBA(c) \
	do { \
		c.r = c.r / 255; \
		c.g = c.g / 255; \
		c.b = c.b / 255; \
	} \
	while (0);

#endif // RENDERER_DEFINES_H
