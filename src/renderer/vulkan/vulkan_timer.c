#include "vulkan_timer.h"

static VkQueryPool gTimer;

YND VkResult
vkQueryPoolTimerCreate(VkDevice device, VkAllocationCallbacks* pAllocator, VkQueryPool* pOutQueryPool)
{
	VkQueryPoolCreateInfo queryPoolCreateInfo = {
		.sType		= VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO,
		.queryType	= VK_QUERY_TYPE_TIMESTAMP,
		.queryCount	= 2,
	};

	pOutQueryPool = &gTimer;
	VK_CHECK(vkCreateQueryPool(device, &queryPoolCreateInfo, pAllocator, pOutQueryPool));

	/* NOTE: This is just temporary */
	return VK_SUCCESS;
}

void
vkQueryPoolTimerDestroy(VkDevice device, VkAllocationCallbacks* pAllocator, VkQueryPool queryPool)
{
	/* NOTE: This is just temporary */
	queryPool = gTimer;
	vkDestroyQueryPool(device, queryPool, pAllocator);
}

void
vkTimerStart(VkCommandBuffer command)
{
	uint32_t firstQuery = 0;
	uint32_t queryCount = 1;

	vkCmdResetQueryPool(command, gTimer, firstQuery, queryCount);
	vkCmdWriteTimestamp(command, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, gTimer, firstQuery);
}

void
vkTimerEnd(VkCommandBuffer command)
{
	uint32_t secondQuery = 1;

	/* NOTE: This is just temporary */
	vkCmdResetQueryPool(command, gTimer, 1, 1);
	vkCmdWriteTimestamp(command, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, gTimer, secondQuery);
}

YND VkResult
vkTimerGetResults(VkDevice device, uint64_t* pOutTimeStamp)
{
	uint32_t			firstQuery	= 0;
	uint32_t			queryCount	= 2;
	size_t				dataSize	= sizeof(uint64_t) * 2;
	VkDeviceSize		stride		= sizeof(uint64_t);
	VkQueryResultFlags	flags		= VK_QUERY_RESULT_64_BIT;

	VK_CHECK(vkGetQueryPoolResults(
				device,
				gTimer,
				firstQuery,
				queryCount,
				dataSize,
				pOutTimeStamp,
				stride,
				flags));

	return VK_SUCCESS;
}

VkResult
vkTimerLog(VkDevice device, SECOND_UNIT unit)
{
	f64 unitScale = 1.0f;
	char *pStrUnit = "ms";
	switch (unit)
	{
		case NANOSECONDS:
			unitScale *= 1000.0f * 1000.0f;
			pStrUnit = "ns";
			break;
		case MICROSECONDS:
			unitScale *= 1000.0f;
			pStrUnit = "us";
			break;
		case MILLISECONDS:
			unitScale *= 1.0f;
			pStrUnit = "ms";
			break;
		case SECONDS:
			unitScale *= 0.1f;
			pStrUnit = "s";
			break;
		default:
			YERROR("Unknown unit %d defaults to milliseconds", unit);
			break;
	}
	uint64_t pTimeStamp[2];
	VK_CHECK(vkTimerGetResults(device, pTimeStamp));
	f64 deltaTime = (pTimeStamp[1] - pTimeStamp[0]);
	YINFO("Took: %.2f%s", deltaTime / unitScale, pStrUnit);
	return VK_SUCCESS;
}
