#ifdef DDEBUG
#define DARRAY_LOG
#endif

#ifdef DARRAY_LOG

#ifndef DARRAY_DEBUG_H
#define DARRAY_DEBUG_H

#	include "mydefines.h"

/*
Memory layout
uint64_t capacity = number elements that can be held
uint64_t length = number of elements currently contained
uint64_t stride = size of each element in bytes
void* pElements
*/
enum
{
    DARRAY_CAPACITY,
    DARRAY_LENGTH,
    DARRAY_STRIDE,
    DARRAY_FIELD_LENGTH
};

void GetLeaks(void);

YND void* _DarrayCreate(
		uint64_t							length,
		uint64_t							stride,
		const char*							pFile,
		int									line);

void _DarrayDestroy(
		void*								pArray,
		const char*							pFile,
		int									line);

YND uint64_t _DarrayFieldGet(
		void*								pArray,
		uint64_t							field);

void _DarrayFieldSet(
		void*								pArray,
		uint64_t							field,
		uint64_t							value);

YND void* _DarrayResize(
		void*								pArray,
		const char*							pFile,
		int									line);

YND void* _DarrayPush(
		void*								pArray,
		const void*							pValue,
		const char*							pFile,
		int									line);

void _DarrayPop(
		void*								pArray,
		void*								pDest);

void _DarrayPopAt(
		void*								pArray,
		uint64_t							index,
		void*								pDest);

YND void* _DarrayInsertAt(
		void*								pArray,
		uint64_t							index,
		void*								pValue,
		const char*							pFile,
		int									line);

#define DARRAY_DEFAULT_CAPACITY 1
#define DARRAY_RESIZE_FACTOR 2

#define DarrayCreate(var) \
    _DarrayCreate(DARRAY_DEFAULT_CAPACITY, sizeof(typeof(var)), __FILE__, __LINE__)

#define DarrayReserve(type, capacity) \
    _DarrayCreate(capacity, sizeof(type), __FILE__, __LINE__)

#define DarrayDestroy(array) _DarrayDestroy(array, __FILE__, __LINE__);

#define DarrayPush(array, value)           \
    {                                       \
        typeof(value) temp = value;         \
        array = _DarrayPush(array, &temp, __FILE__, __LINE__); \
    }

#define DarrayPop(array, value_ptr) \
    _DarrayPop(array, value_ptr)

#define darray_insert_at(array, index, value)           \
    {                                                   \
        typeof(value) temp = value;                     \
        array = _DarrayInsertAt(array, index, &temp, __FILE__, __LINE__); \
    }

#define DarrayPopAt(array, index, value_ptr) \
    _DarrayPopAt(array, index, value_ptr)

#define DarrayClear(array) \
    _DarrayFieldSet(array, DARRAY_LENGTH, 0)

#define DarrayCapacity(array) \
    _DarrayFieldGet(array, DARRAY_CAPACITY)

#define DarrayLength(array) \
    _DarrayFieldGet(array, DARRAY_LENGTH)

#define DarrayStride(array) \
    _DarrayFieldGet(array, DARRAY_STRIDE)

#define DarrayLengthSet(array, value) \
    _DarrayFieldSet(array, DARRAY_LENGTH, value)

///////////////////////////////////////////////////////////////////////

#define darray_capacity(array) \
    _DarrayFieldGet(array, DARRAY_CAPACITY)

#define darray_length(array) \
    _DarrayFieldGet(array, DARRAY_LENGTH)

#define darray_stride(array) \
    _DarrayFieldGet(array, DARRAY_STRIDE)

#define darray_length_set(array, value) \
    _DarrayFieldSet(array, DARRAY_LENGTH, value)

#endif // DARRAY_DEBUG_H
#else
void GetLeaks(void);
#endif // DARRAY_LOG
