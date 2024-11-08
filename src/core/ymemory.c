#include "os.h"

#include "ystring.h"
#include "ymemory.h"
#include "logger.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

struct MemoryStats
{
	uint64_t totalAllocated;
	uint64_t pTaggedAllocations[MEMORY_TAG_MAX_TAGS];
};

static const char *MemoryTagStrings[MEMORY_TAG_MAX_TAGS] = {
	"UNKNOWN    ",
	"TRACKER    ",
	"ARRAY      ",
	"DARRAY     ",
	"DICT       ",
	"RING_QUEUE ",
	"BST        ",
	"STRING     ",
	"APPLICATION",
	"JOB        ",
	"TEXTURE    ",
	"MAT_INST   ",
	"RENDERER   ",
	"CONTEXT    ",
	"GAME       ",
	"TRANSFORM  ",
	"ENTITY     ",
	"ENTITY_NODE",
	"SCENE      "
};

static struct MemoryStats gStats;

void*
yZeroMemory(void *pBlock, uint64_t size)
{
	return memset(pBlock, 0, size);
}

/**
  * Returns zero'ed allocated buffer
  */
YND void *
_yAlloc(uint64_t size, MemoryTags tag)
{
	if (tag == MEMORY_TAG_UNKNOWN)
		YDEBUG("yAlloc called using MEMORY_TAG_UNKNOWN, re-class this allocation.");
	gStats.totalAllocated += size;
	gStats.pTaggedAllocations[tag] += size;
	void *pBlock = malloc(size);
	YASSERT_MSG(pBlock, "OUT OF MEMORY");
	memset(pBlock, 0, size);
	return pBlock;
}

void 
_yFree(void *pBlock, uint64_t size, MemoryTags tag)
{
	if (tag == MEMORY_TAG_UNKNOWN)
		YDEBUG("yFree called using MEMORY_TAG_UNKNOWN, re-class this allocation.");
	gStats.totalAllocated -= size;
	gStats.pTaggedAllocations[tag] -= size;

	// TODO: Memory alignment
	free(pBlock);
	// WARN: This might cost a lot ?
	pBlock = NULL;
}

/**
  * Returns allocated buffer to print
  */
YND char *
StrGetMemoryUsage(void)
{
	if (gStats.totalAllocated <= 0)
		return StrDup("No memory allocated.");
	const uint64_t gib = 1024 * 1024 * 1024;
	const uint64_t mib = 1024 * 1024;
	const uint64_t kib = 1024;
	
	char pBuffer[8000] = "System memory use (tagged):\n";
	uint64_t offset = strlen(pBuffer);
	for (uint32_t i = 0; i < MEMORY_TAG_MAX_TAGS; ++i)
	{
		if (gStats.pTaggedAllocations[i] <= 0)
			continue;
		char pUnit[4] = "XiB";
		float fAmount = 1.0f;
		if (gStats.pTaggedAllocations[i] >= gib) 
		{
			pUnit[0] = 'G';
			fAmount = gStats.pTaggedAllocations[i] / (float)gib;
		} 
		else if (gStats.pTaggedAllocations[i] >= mib) 
		{
			pUnit[0] = 'M';
			fAmount = gStats.pTaggedAllocations[i] / (float)mib;
		}
		else if (gStats.pTaggedAllocations[i] >= kib) 
		{
			pUnit[0] = 'K';
			fAmount = gStats.pTaggedAllocations[i] / (float)kib;
		} 
		else 
		{
			pUnit[0] = 'B';
			pUnit[1] = 0;
			fAmount = (float)gStats.pTaggedAllocations[i];
		}
		int32_t length = snprintf(pBuffer + offset, 8000, "  %s: %.2f%s\n", MemoryTagStrings[i], fAmount, pUnit);
		offset += length;
	}
	char* pOutString = StrDup(pBuffer);
	return pOutString;
}
