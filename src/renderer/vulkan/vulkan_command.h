#ifndef VULKAN_COMMAND_H
#define VULKAN_COMMAND_H

#include <vulkan/vulkan.h>
#include "yvulkan.h"

typedef void (*pfnSubmit)(void* pCtx, VulkanCommandBuffer command);

YND VkResult vkCommandSubmitImmediate(
		pfnSubmit							pfnSubmit,
		VulkanDevice						device,
		VulkanImmediateSubmit				submitCommand,
		void*								pCtx);

YND VkResult vkImmediateSubmitCommandCreate(
		VulkanDevice						device,
		VulkanImmediateSubmit*				pSubmit,
		VkAllocationCallbacks*				pAllocator);

YND VkResult vkCommandPoolCreate(
		VulkanDevice						device,
		VkAllocationCallbacks*				pAllocator,
		VkCommandPool*						pOutCommandPool);

YND VkResult vkCommandBufferAllocate(
		VkDevice							device,
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

void vkCommandBufferBegin(
		VulkanCommandBuffer*				pCmd,
		b8									bSingleUse,
		b8									bRenderPassContinue,
		b8									bSimultaneousUse);

YND VkResult vkCommandBufferEnd(
		VulkanCommandBuffer*				pCmd);

YND VkResult vkCommandBufferReset(
		VulkanCommandBuffer*				pCmd,
		VkCommandBufferResetFlags			flags);

#endif  // VULKAN_COMMAND_H
