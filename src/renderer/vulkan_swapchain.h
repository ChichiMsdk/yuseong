#ifndef VULKAN_SWAPCHAIN_H
#define VULKAN_SWAPCHAIN_H

#include "yvulkan.h"

YND VkResult vkSwapchainDestroy(
		VkContext*							pCtx, 
		VkSwapchain*						pSwapchain);

YND VkResult vkSwapchainCreate(
		VkContext*							pContext,
		uint32_t							width,
		uint32_t							height,
		VkSwapchain*						pSwapchain);

void vkDeviceQuerySwapchainSupport(
		VkPhysicalDevice					physDevice,
		VkSurfaceKHR						surface,
		VkSwapchainSupportInfo*				pOutSupportInfo);

YND VkResult vkSwapchainAcquireNextImageIndex(
		VkContext*							pCtx,
		VkSwapchain* 						pSwapchain,
		uint64_t 							timeoutNS,
		VkSemaphore 						semaphoreImageAvailable,
		VkFence								fence,
		uint32_t*							pOutImageIndex);

#endif // VULKAN_SWAPCHAIN_H
