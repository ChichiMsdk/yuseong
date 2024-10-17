#ifndef VULKAN_COMMAND_H
#define VULKAN_COMMAND_H

#include <vulkan/vulkan.h>
#include "yvulkan.h"

[[nodiscard]] VkResult vkCommandPoolCreate(
		VkContext*							pContext);

[[nodiscard]] VkResult vkCommandBufferAllocate(
		VkContext*							pCtx,
		VkCommandBufferLevel				cmdBufferLvl,
		VkCommandPool						pool,
		VulkanCommandBuffer*				pOutCommandBuffers);

[[nodiscard]] VkResult vkCommandPoolDestroy(
		VkContext*							pCtx,
		VkCommandPool*						pCmdPools,
		uint32_t							poolCount);

[[nodiscard]] VkResult vkCommandBufferFree(
		VkContext*							pCtx,
		VulkanCommandBuffer*				pCommandBuffers,
		VkCommandPool*						pPool,
		uint32_t							poolCount);

[[nodiscard]] VkResult vkCommandBufferCreate(
		VkContext*							pCtx);
#endif  // VULKAN_COMMAND_H
