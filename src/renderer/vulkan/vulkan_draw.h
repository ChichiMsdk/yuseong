#ifndef VULKAN_DRAW_H
#define VULKAN_DRAW_H

#include "yvulkan.h"

YND	VkResult vkQueueSubmitAndSwapchainPresent(
		VkContext*							pCtx,
		VulkanCommandBuffer*				pCmd);

YND VkResult vkDrawImpl(
		VkContext*							pCtx);

void		 vkClearBackground(
		VkContext*							pCtx,
		VulkanCommandBuffer*				pCmd,
		VkImage								pImage);

#endif // VULKAN_DRAW_H
