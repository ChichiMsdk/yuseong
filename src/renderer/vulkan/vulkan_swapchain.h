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

YND VkResult vkDeviceQuerySwapchainSupport(
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

YND VkResult vkSwapchainPresent(
		VkContext*							pCtx,
		VkSwapchain*						pSwapchain,
		YMB VkQueue							gfxQueue,
		VkQueue								presentQueue,
		VkSemaphore							semaphoreRenderComplete,
		uint32_t							presentImageIndex);

YND VkResult vkImageCreate(
		VkContext*							pContext,
		VkImageType							imageType,
		uint32_t							width,
		uint32_t							height,
		VkFormat							format,
		VkImageTiling						tiling,
		VkImageUsageFlags					usage,
		VkMemoryPropertyFlags				memoryFlags,
		b32									bCreateView,
		VkImageAspectFlags					viewAspectFlags,
		VulkanImage*						pOutImage,
		YMB VkExtent3D						extent,
		uint32_t mipLevels					);

#endif // VULKAN_SWAPCHAIN_H
