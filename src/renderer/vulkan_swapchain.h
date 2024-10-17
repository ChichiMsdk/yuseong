#ifndef VULKAN_SWAPCHAIN_H
#define VULKAN_SWAPCHAIN_H

#include "yvulkan.h"

[[nodiscard]] VkResult vkSwapchainCreate(
		VkContext*							pContext,
		uint32_t							width,
		uint32_t							height,
		VkSwapchain*						pSwapchain);

void vkDeviceQuerySwapchainSupport(
		VkPhysicalDevice					physDevice,
		VkSurfaceKHR						surface,
		VkSwapchainSupportInfo*				pOutSupportInfo);

#endif // VULKAN_SWAPCHAIN_H
