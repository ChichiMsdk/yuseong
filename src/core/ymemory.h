#ifndef YMEMORY_H
#define YMEMORY_H

#	include "mydefines.h"
#	include "logger.h"

typedef enum MemoryTags
{
	MEMORY_TAG_UNKNOWN,
	MEMORY_TAG_ARRAY,
	MEMORY_TAG_DARRAY,
	MEMORY_TAG_DICT,
	MEMORY_TAG_RING_QUEUE,
	MEMORY_TAG_BST,
	MEMORY_TAG_STRING,
	MEMORY_TAG_APPLICATION,
	MEMORY_TAG_JOB,
	MEMORY_TAG_TEXTURE,
	MEMORY_TAG_MATERIAL_INSTANCE,
	MEMORY_TAG_RENDERER,
	MEMORY_TAG_GAME,
	MEMORY_TAG_TRANSFORM,
	MEMORY_TAG_ENTITY,
	MEMORY_TAG_ENTITY_NODE,
	MEMORY_TAG_SCENE,

	MEMORY_TAG_MAX_TAGS,
}MemoryTags;

#define SystemMemoryUsagePrint() \
	do { \
		char *pMemReport = StrGetMemoryUsage(); \
		YDEBUG("%s", pMemReport); \
		free(pMemReport); \
	} while (0)

#define yFree2(pBlock, size, tag) \
	_yFree(pBlock, size, tag)

#define yFree(pBlock, size, tag) \
	_yFree(pBlock, sizeof(*(pBlock)) * size, tag)

#define yAlloc(size, tag) \
	_yAlloc(size, tag);

/*
 * #define yAlloc(size, tag) \
 * 	_yAlloc(size, tag)
 */

YND void* _yAlloc(
		uint64_t							size,
		MemoryTags							tag);

void _yFree(
		void*								pBlock,
		uint64_t							size,
		MemoryTags							tag);

YND char* StrGetMemoryUsage(void);

void *yZeroMemory(
		void*								pBlock,
		uint64_t							size);

void* yCopyMemory(
		void*								pDest,
		const void*							pSrc,
		uint64_t							size);

void* ySetMemory(
		void*								pDest,
		int32_t								value,
		uint64_t							size);

#endif // YMEMORY_H
