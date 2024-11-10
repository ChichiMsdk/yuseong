#ifndef MYDEFINES_H
#define MYDEFINES_H

#include "mystd.h"
#include <stdint.h>

typedef enum SECOND_UNIT
{
	NANOSECONDS,
	MICROSECONDS,
	MILLISECONDS,
	SECONDS
}SECOND_UNIT;

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long u64;

typedef signed char i8;
typedef signed short i16;
typedef signed int i32;
typedef signed long long i64;

typedef float f32;
typedef double f64;

// Boolean types
typedef int b32;
typedef char b8;

#if defined(__clang__) || defined(__gcc__)
#define STATIC_ASSERT _Static_assert
#else
#define STATIC_ASSERT static_assert
#endif

#define YU_NULL ((void *)0);

STATIC_ASSERT(sizeof(u8) == 1, "Expected u8 to be 1 byte.");
STATIC_ASSERT(sizeof(u16) == 2, "Expected u16 to be 2 bytes.");
STATIC_ASSERT(sizeof(u32) == 4, "Expected u32 to be 4 bytes.");
STATIC_ASSERT(sizeof(u64) == 8, "Expected u64 to be 8 bytes.");

STATIC_ASSERT(sizeof(i8) == 1, "Expected i8 to be 1 byte.");
STATIC_ASSERT(sizeof(i16) == 2, "Expected i16 to be 2 bytes.");
STATIC_ASSERT(sizeof(i32) == 4, "Expected i32 to be 4 bytes.");
STATIC_ASSERT(sizeof(i64) == 8, "Expected i64 to be 8 bytes.");

STATIC_ASSERT(sizeof(f32) == 4, "Expected f32 to be 4 bytes.");
STATIC_ASSERT(sizeof(f64) == 8, "Expected f64 to be 8 bytes.");

#define TRUE 1
#define FALSE 0

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) 
#define YPLATFORM_WINDOWS 1
#ifndef _WIN64
#error "64-bit is required on Windows!"
#endif
#elif defined(__linux__) || defined(__gnu_linux__)
// Linux OS
#define YPLATFORM_LINUX 1
#if defined(__ANDROID__)
#define YPLATFORM_ANDROID 1
#endif
#elif defined(__unix__)
// Catch anything not caught by the above.
#define YPLATFORM_UNIX 1
#elif defined(_POSIX_VERSION)
// Posix
#define YPLATFORM_POSIX 1
#elif __APPLE__
// Apple platforms
#define YPLATFORM_APPLE 1
#include <TargetConditionals.h>
#if TARGET_IPHONE_SIMULATOR
// iOS Simulator
#define YPLATFORM_IOS 1
#define YPLATFORM_IOS_SIMULATOR 1
#elif TARGET_OS_IPHONE
#define YPLATFORM_IOS 1
// iOS device
#elif TARGET_OS_MAC
// Other kinds of Mac OS
#else
#error "Unknown Apple platform"
#endif
#else
#error "Unknown platform!"
#endif

#ifdef YEXPORT
// Exports
#ifdef _MSC_VER
#define YAPI __declspec(dllexport)
#else
#define YAPI __attribute__((visibility("default")))
#endif
#else
// Imports
#ifdef _MSC_VER
#define YAPI __declspec(dllimport)
#else
#define YAPI
#endif
#endif


#define COUNT_OF(x) ((sizeof(x)/sizeof(0[x])) / ((size_t)(!(sizeof(x) % sizeof(0[x])))))
#define YCLAMP(value, min, max) (value <= min) ? min : (value >= max) ? max : value;

#endif // MYDEFINES_H
