#ifndef VULKAN_IMAGE_H
#define VULKAN_IMAGE_H

#include "yvulkan.h"

void vkImageTransition(
		VkCommandBuffer		    cmd,
		VkImage			    pImg,
		VkImageLayout		    currentLayout,
		VkImageLayout		    newLayout);

void vkImageCopy(
		VkCommandBuffer		    cmd,
		VkImage			    srcImage,
		VkImage			    dstImage,
		VkExtent2D		    srcSize,
		VkExtent2D		    dstSize);

#endif // VULKAN_IMAGE_H
