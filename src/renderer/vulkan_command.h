#ifndef VULKAN_COMMAND_H
#define VULKAN_COMMAND_H

#include <vulkan/vulkan.h>
#include "yvulkan.h"

YND VkResult vkCommandPoolCreate(
		VkContext*							pContext);

YND VkResult vkCommandBufferAllocate(
		VkContext*							pCtx,
		VkCommandBufferLevel				cmdBufferLvl,
		VkCommandPool						pool,
		VulkanCommandBuffer*				pOutCommandBuffers);

YND VkResult vkCommandPoolDestroy(
		VkContext*							pCtx,
		VkCommandPool*						pCmdPools,
		uint32_t							poolCount);

YND VkResult vkCommandBufferFree(
		VkContext*							pCtx,
		VulkanCommandBuffer*				pCommandBuffers,
		VkCommandPool*						pPool,
		uint32_t							poolCount);

YND VkResult vkCommandBufferCreate(
		VkContext*							pCtx);
#endif  // VULKAN_COMMAND_H
