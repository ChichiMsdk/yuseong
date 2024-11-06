#include "darray_debug.h"

#ifdef DARRAY_LOG

#include "ymemory.h"
#include "logger.h"
#include "ystring.h"

#include <string.h>
#include <stdio.h>

#define START_SIZE 1000
#define POINTER_MAX 100

typedef struct PointerArray
{
	void*			ptr;
	char*			pLocation;
	size_t			size;
	int				line;
}PointerArray;

typedef struct Array
{
	PointerArray*	pAllocs;
	size_t			arraySize;
	size_t			elemCount;
}Array;

Array gTracker = {0};

static inline void
ReallocCheck(void)
{
	if (gTracker.arraySize == 0)
	{
		gTracker.pAllocs = yAlloc(START_SIZE * sizeof(PointerArray), MEMORY_TAG_TRACKER);
		gTracker.arraySize = START_SIZE;
		gTracker.elemCount = 0;
	}
	if (gTracker.elemCount >= gTracker.arraySize)
	{
		PointerArray* pTmp = yAlloc(gTracker.arraySize * 2 * sizeof(PointerArray), MEMORY_TAG_TRACKER);
		memcpy(pTmp, gTracker.pAllocs, gTracker.arraySize);
		yFree(gTracker.pAllocs, gTracker.arraySize, MEMORY_TAG_TRACKER);
		gTracker.pAllocs = pTmp;
		gTracker.arraySize *= 2;
	}
}

void
AddToTracker(void* pPtr, const char *pFile, int line, size_t size)
{
	ReallocCheck();
	size_t i = 0;
	while (i < gTracker.elemCount)
	{
		if (gTracker.pAllocs[i].ptr == pPtr)
		{
			YFATAL("%p ALREADY HERE ?!", pPtr);
			return ;
		}
		i++;
	}
	gTracker.pAllocs[gTracker.elemCount].pLocation = StrDup(pFile);
	gTracker.pAllocs[gTracker.elemCount].line = line;
	gTracker.pAllocs[gTracker.elemCount].ptr = pPtr;
	gTracker.pAllocs[gTracker.elemCount].size = size;
	gTracker.elemCount++;
}

void
PrintFullTrackerList(void)
{
	if (gTracker.elemCount == 0)
		return ;
	YTRACE("Current tracker list:");
	size_t i = 0;
	while (i < gTracker.elemCount)
	{
		YTRACE("[%llu]%p at %s:%d", i, gTracker.pAllocs[i].ptr, gTracker.pAllocs[i].pLocation, gTracker.pAllocs[i].line);
		i++;
	}
	YTRACE("End of tracker list");
}

#include <stdlib.h>

void
RemoveFromTracker(void* pPtr, YMB const char* pFile, YMB int line)
{
	size_t i = 0;
	b8 bFound = FALSE;
	while (i < gTracker.elemCount)
	{
		if (gTracker.pAllocs[i].ptr == pPtr)
		{
			bFound = TRUE;
			// copy to pTmp without pArray[j];
			size_t k = 0;
			size_t l = 0;
			while (l < gTracker.elemCount)
			{
				if (l != i)
				{
					gTracker.pAllocs[k].ptr = gTracker.pAllocs[l].ptr;
					gTracker.pAllocs[k].pLocation = gTracker.pAllocs[l].pLocation;
					gTracker.pAllocs[k].line = gTracker.pAllocs[l].line;
					k++;
				}
				else
				{
					gTracker.pAllocs[l].ptr = NULL;
					free(gTracker.pAllocs[l].pLocation);
					gTracker.pAllocs[l].line = -1;
				}
				l++;
			}
			while (k < gTracker.elemCount)
				gTracker.pAllocs[k++].ptr = NULL; 
			gTracker.elemCount--;
			break;
		}
		if (bFound == TRUE)
			break;
		i++;
	}
	if (bFound == FALSE)
	{
		YERROR("God damn: couldn't find %p. Did you free it before calling this ?", pPtr);
		return ;
	}
}

char *pBytes[] = {"bytes", "byte"};
char *pPrefix[] = {"giga", "mega", "kilo", ""};

void
GetLeaks(void)
{
	const uint64_t gib = 1024 * 1024 * 1024;
	const uint64_t mib = 1024 * 1024;
	const uint64_t kib = 1024;
	uint32_t prefix = 3;
	uint32_t plural = 1;

	if (gTracker.elemCount == 0)
	{
		YTRACE("No leaks tracked. (There might be some)");
		return ;
	}
	PointerArray* pTmp = gTracker.pAllocs;
	size_t totalLeaks = 0;

	for (size_t i = 0; i < gTracker.elemCount; i++)
		totalLeaks += pTmp[i].size;

	if (totalLeaks >= gib)
		prefix = 0;
	else if (totalLeaks >= mib)
		prefix = 1;
	else if (totalLeaks >= kib)
		prefix = 2;

	if (totalLeaks > 1)
		plural = 0;

	printf("=============================================================================\n");
	YLEAKS("%zu pointers and total of %zu %s%s leaked", gTracker.elemCount, totalLeaks, pPrefix[prefix], pBytes[plural]);

	for (size_t i = 0; i < gTracker.elemCount; i++)
	{
		YOUT("\t%zu bytes at adress %p in file: %s:%d", pTmp[i].size, pTmp[i].ptr, pTmp[i].pLocation, pTmp[i].line);
	}
}
#include <stdlib.h>

YND void*
_DarrayCreate(uint64_t length, uint64_t stride, YMB const char* pFile, YMB int line)
{
    uint64_t headerSize = DARRAY_FIELD_LENGTH * sizeof(uint64_t);
    uint64_t arraySize = length * stride;
    uint64_t* pNewArray = yAlloc(headerSize + arraySize, MEMORY_TAG_DARRAY);
    memset(pNewArray, 0, headerSize + arraySize);
    pNewArray[DARRAY_CAPACITY] = length;
    pNewArray[DARRAY_LENGTH] = 0;
    pNewArray[DARRAY_STRIDE] = stride;

	AddToTracker(pNewArray, pFile, line, headerSize + arraySize);
    return pNewArray + DARRAY_FIELD_LENGTH;
}

void
_DarrayDestroy(void* pArray, YMB const char* pFile, YMB int line)
{
    uint64_t* pHeader = (uint64_t*)pArray - DARRAY_FIELD_LENGTH;
    uint64_t headerSize = DARRAY_FIELD_LENGTH * sizeof(uint64_t);
    uint64_t totalSize = headerSize + pHeader[DARRAY_CAPACITY] * pHeader[DARRAY_STRIDE];
	RemoveFromTracker(pHeader, pFile, line);
    _yFree(pHeader, totalSize, MEMORY_TAG_DARRAY);
}

YND uint64_t
_DarrayFieldGet(void* pArray, uint64_t field)
{
    uint64_t* pHeader = (uint64_t*)pArray - DARRAY_FIELD_LENGTH;
    return pHeader[field];
}

void
_DarrayFieldSet(void* pArray, uint64_t field, uint64_t value)
{
    uint64_t* pHeader = (uint64_t*)pArray - DARRAY_FIELD_LENGTH;
    pHeader[field] = value;
}

YND void*
_DarrayResize(void* pArray, const char* pFile, int line)
{
	/*
	 * WARN: Leak ?
	 */
    uint64_t length = darray_length(pArray);
    uint64_t stride = darray_stride(pArray);
    void* pTemp = _DarrayCreate((DARRAY_RESIZE_FACTOR * darray_capacity(pArray)), stride, pFile, line);
    memcpy(pTemp, pArray, length * stride);

    _DarrayFieldSet(pTemp, DARRAY_LENGTH, length);
    _DarrayDestroy(pArray, pFile, line);
    return pTemp;
}

YND void*
_DarrayPush(void* pArray, const void* pValue, const char* pFile, int line)
{
    uint64_t length = darray_length(pArray);
    uint64_t stride = darray_stride(pArray);
    if (length >= darray_capacity(pArray))
	{
        pArray = _DarrayResize(pArray, pFile, line);
    }

    uint64_t addr = (uint64_t)pArray;
    addr += (length * stride);
    memcpy((void*)addr, pValue, stride);
    _DarrayFieldSet(pArray, DARRAY_LENGTH, length + 1);
    return pArray;
}

void
_DarrayPop(void* pArray, void* pDest)
{
    uint64_t length = darray_length(pArray);
    uint64_t stride = darray_stride(pArray);
    uint64_t addr = (uint64_t)pArray;
    addr += ((length - 1) * stride);
    memcpy(pDest, (void*)addr, stride);
    _DarrayFieldSet(pArray, DARRAY_LENGTH, length - 1);
}

void
_DarrayPopAt(void* pArray, uint64_t index, void* pDest)
{
    uint64_t length = darray_length(pArray);
    uint64_t stride = darray_stride(pArray);
    if (index >= length)
	{
        YERROR("Index outside the bounds of this array! Length: %llu, index: %llu", length, index);
        return ;
    }
    uint64_t addr = (uint64_t)pArray;
    memcpy(pDest, (void*)(addr + (index * stride)), stride);

    // If not on the last element, snip out the entry and copy the rest inward.
    if (index != length - 1)
	{
        memcpy((void*)(addr + (index * stride)), (void*)(addr + ((index + 1) * stride)),
            stride * (length - index));
    }

    _DarrayFieldSet(pArray, DARRAY_LENGTH, length - 1);
    return ;
}

YND void*
_DarrayInsertAt(void* pArray, uint64_t index, void* pValue, const char* pFile, int line)
{
    uint64_t length = darray_length(pArray);
    uint64_t stride = darray_stride(pArray);
    if (index >= length)
	{
        YERROR("Index outside the bounds of this pArray! Length: %llu, index: %llu", length, index);
        return pArray;
    }
    if (length >= darray_capacity(pArray))
	{
    	pArray = _DarrayResize(pArray, pFile, line);
    }
    uint64_t addr = (uint64_t)pArray;
    // If not on the last element, copy the rest outward.
    if (index != length - 1)
	{
        memcpy( (void*)(addr + ((index + 1) * stride)), (void*)(addr + (index * stride)), stride * (length - index));
    }
    // Set the value at the index
    memcpy((void*)(addr + (index * stride)), pValue, stride);

    _DarrayFieldSet(pArray, DARRAY_LENGTH, length + 1);
    return pArray;
}
#else

void
GetLeaks(void)
{
}

#endif // DARRAY_LOG
