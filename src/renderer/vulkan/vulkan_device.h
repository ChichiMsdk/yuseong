#ifndef VULKAN_DEVICE_H
#define VULKAN_DEVICE_H

#include "yvulkan.h"
#include <vulkan/vulkan.h>
#include <vulkan/vk_enum_string_helper.h>

YND VkResult VulkanCreateDevice(
		VkContext*							pVulkanContext,
		VkDevice*							pOutDevice,
		char*								pGPUName);
#endif  // VULKAN_DEVICE_H
