#ifndef VULKAN_TIMER_H
#define VULKAN_TIMER_H

#include "yvulkan.h"

YND VkResult vkQueryPoolTimerCreate(
		VkDevice							device,
		VkAllocationCallbacks*				pAllocator,
		VkQueryPool*						pOutQueryPool);

YND VkResult vkTimerGetResults(
		VkQueue								queue,
		VkDevice							device,
		uint64_t*							pOutTimeStamp);

void vkQueryPoolTimerDestroy(
		VkDevice							device,
		VkAllocationCallbacks*				pAllocator,
		VkQueryPool							queryPool);

void vkTimerStart(
		VkCommandBuffer						command);

void vkTimerEnd(
		VkCommandBuffer						command);

VkResult vkTimerLog(
		VkPhysicalDeviceProperties			properties,
		VkQueue								queue, 
		VkDevice							device,
		SECOND_UNIT							unit);

#endif // VULKAN_TIMER_H
