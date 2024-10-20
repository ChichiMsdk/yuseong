#ifndef VULKAN_IMAGE_H
#define VULKAN_IMAGE_H

#include "yvulkan.h"

void vkImageTransition(
		VulkanCommandBuffer*				pCmd,
		VkImage								pImg,
		VkImageLayout						currentLayout,
		VkImageLayout						newLayout);

#endif // VULKAN_IMAGE_H
