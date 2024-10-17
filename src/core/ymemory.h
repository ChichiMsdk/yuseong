#ifndef YMEMORY_H
#define YMEMORY_H

#	include "mydefines.h"

typedef enum memory_tag
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
}memory_tag;

void *yalloc(u64 size, memory_tag tag);
void yfree(void *block, u64 size, memory_tag tag);
char *get_memory_usage_str(void);

void *yzero_memory(void *block, u64 size);
void *ycopy_memory(void *dest, const void *source, u64 size);
void *yset_memory(void *dest, i32 value, u64 size);

#endif // YMEMORY_H
