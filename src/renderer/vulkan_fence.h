#ifndef VULKAN_FENCE_H
#define VULKAN_FENCE_H

#include "yvulkan.h"

YND VkResult vkFenceCreate(
		VkContext*							pCtx,
		b8									bSignaled,
		VulkanFence*						pOutFence);

void vkFenceDestroy(
		VkContext*							pCtx,
		VulkanFence*						pFence);

YND VkResult vkFenceWait(
		VkContext*							pCtx,
		VulkanFence*						pFence,
		uint64_t timeoutNs);

YND VkResult vkFenceReset(
		VkContext*							pCtx, 
		VulkanFence*						pFence);

#endif // VULKAN_FENCE_H
