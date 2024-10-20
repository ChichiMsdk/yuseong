#include "vulkan_device.h"
#include "vulkan_swapchain.h"
#include "core/ymemory.h"
#include "core/logger.h"
#include "core/darray.h"

#include <string.h>
#include <stdlib.h>

b8 
PhysicalDeviceMeetsRequirements( VkPhysicalDevice device, VkSurfaceKHR surface,
		const VkPhysicalDeviceProperties* pProperties,
		const VkPhysicalDeviceFeatures* pFeatures,
		const VkPhysicalDeviceRequirements* pRequirements,
		VkPhysicalDeviceQueueFamilyInfo* pOutQueueInfo,
		VkSwapchainSupportInfo* pOutSwapchainSupport)
{
	VkResult errcode = 0;
    // Evaluate device properties to determine if it meets the needs of our applcation.
    pOutQueueInfo->graphicsFamilyIndex = -1;
    pOutQueueInfo->presentFamilyIndex = -1;
    pOutQueueInfo->computeFamilyIndex = -1;
    pOutQueueInfo->transferFamilyIndex = -1;
    // Discrete GPU?
    if (pRequirements->bDiscreteGpu)
	{
        if (pProperties->deviceType != VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
		{
            YINFO("Device %s is not a discrete GPU. Skipping.", pProperties->deviceName);
            return FALSE;
        }
    }
    u32 queue_family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, 0);
    VkQueueFamilyProperties queue_families[queue_family_count];
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, queue_families);
    // Look at each queue and see what queues it supports
    u8 min_transfer_score = 255;
    for (u32 i = 0; i < queue_family_count; ++i)
	{
        u8 current_transfer_score = 0;
        // Graphics queue?
        if (queue_families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
		{
            pOutQueueInfo->graphicsFamilyIndex = i;
            ++current_transfer_score;
        }
        // Compute queue?
        if (queue_families[i].queueFlags & VK_QUEUE_COMPUTE_BIT)
		{
            pOutQueueInfo->computeFamilyIndex = i;
            ++current_transfer_score;
        }
        // Transfer queue?
        if (queue_families[i].queueFlags & VK_QUEUE_TRANSFER_BIT)
		{
            // Take the index if it is the current lowest. This increases the
            // liklihood that it is a dedicated transfer queue.
            if (current_transfer_score <= min_transfer_score)
			{
                min_transfer_score = current_transfer_score;
                pOutQueueInfo->transferFamilyIndex = i;
            }
        }
        // Present queue?
        VkBool32 supports_present = VK_FALSE;
        errcode = vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &supports_present);
		if (errcode != VK_SUCCESS) {YFATAL("%s", string_VkResult(errcode)); exit(errcode); }
        if (supports_present)
            pOutQueueInfo->presentFamilyIndex = i;
    }

    if (
        (!pRequirements->bGraphics || (pRequirements->bGraphics && (i32)pOutQueueInfo->graphicsFamilyIndex != -1)) &&
        (!pRequirements->bPresent || (pRequirements->bPresent && (i32)pOutQueueInfo->presentFamilyIndex != -1)) &&
        (!pRequirements->bCompute || (pRequirements->bCompute && (i32)pOutQueueInfo->transferFamilyIndex != -1)) &&
        (!pRequirements->bTransfer || (pRequirements->bTransfer && (i32)pOutQueueInfo->transferFamilyIndex != -1)))
	{
        YINFO("Device meets queue requirements.");
		YTRACE("Family Indices:")
        YTRACE("Graphics: [%i] | Present: [%i] | Transfer: [%i] | Compute: [%i]",
				pOutQueueInfo->graphicsFamilyIndex, pOutQueueInfo->presentFamilyIndex,
				pOutQueueInfo->transferFamilyIndex, pOutQueueInfo->computeFamilyIndex);

        // Query swapchain support.
        vkDeviceQuerySwapchainSupport(device, surface, pOutSwapchainSupport);
        if (pOutSwapchainSupport->formatCount < 1 || pOutSwapchainSupport->presentModeCount < 1)
		{
            if (pOutSwapchainSupport->pFormats)
			{
				yFree(pOutSwapchainSupport->pFormats,
						sizeof(VkSurfaceFormatKHR) * pOutSwapchainSupport->formatCount, MEMORY_TAG_RENDERER);
			}
            if (pOutSwapchainSupport->pPresentModes)
                {
					yFree(pOutSwapchainSupport->pPresentModes,
							sizeof(VkPresentModeKHR) * pOutSwapchainSupport->presentModeCount, MEMORY_TAG_RENDERER);
				}
            YINFO("Required swapchain support not present, skipping device.");
            return FALSE;
        }
        // Device extensions.
        if (pRequirements->ppDeviceExtensionNames)
		{
            u32 availableExtensionCount = 0;
            VkExtensionProperties* pAvailableExtensions = 0;
            errcode = vkEnumerateDeviceExtensionProperties(device, 0, &availableExtensionCount, 0);
			if (errcode != VK_SUCCESS) {YFATAL("%s", string_VkResult(errcode)); exit(errcode); }
            if (availableExtensionCount != 0)
			{
                pAvailableExtensions =
					yAlloc(sizeof(VkExtensionProperties) * availableExtensionCount, MEMORY_TAG_RENDERER);
                errcode =
					vkEnumerateDeviceExtensionProperties( device, 0, &availableExtensionCount, pAvailableExtensions);
				if (errcode != VK_SUCCESS) {YFATAL("%s", string_VkResult(errcode)); exit(errcode); }

                u32 required_extension_count = darray_length(pRequirements->ppDeviceExtensionNames);
                for (u32 i = 0; i < required_extension_count; ++i)
				{
                    b8 found = FALSE;
                    for (u32 j = 0; j < availableExtensionCount; ++j)
					{
                        if (!strcmp(pRequirements->ppDeviceExtensionNames[i], pAvailableExtensions[j].extensionName))
						{
                            found = TRUE;
                            break;
                        }
                    }
                    if (!found)
					{
                        YINFO("Required extension not found: '%s', skipping device.",
								pRequirements->ppDeviceExtensionNames[i]);
                        yFree(pAvailableExtensions,
								sizeof(VkExtensionProperties) * availableExtensionCount, MEMORY_TAG_RENDERER);
                        return FALSE;
                    }
                }
				yFree(pAvailableExtensions, sizeof(VkExtensionProperties) * availableExtensionCount, MEMORY_TAG_RENDERER);
            }
        }
        // Sampler anisotropy
        if (pRequirements->bSamplerAnisotropy && !pFeatures->samplerAnisotropy)
		{
            YINFO("Device does not support samplerAnisotropy, skipping.");
            return FALSE;
        }
        // Device meets all requirements.
        return TRUE;
    }
    return FALSE;
}

YND VkResult
VulkanDeviceSelect(VkContext *pCtx)
{
	uint32_t physicalDeviceCount = 0;
	VK_CHECK(vkEnumeratePhysicalDevices(pCtx->instance, &physicalDeviceCount, VK_NULL_HANDLE));

	/* VkPhysicalDevice *pVkDevicess = malloc(sizeof(VkPhysicalDevice) * physicalDeviceCount); */
	VkPhysicalDevice pVkDevicess[physicalDeviceCount];
	VK_CHECK(vkEnumeratePhysicalDevices(pCtx->instance, &physicalDeviceCount, pVkDevicess));

	/* VulkanDevice *pVkDevices = malloc(sizeof(VulkanDevice) * physicalDeviceCount); */
	for (uint32_t i = 0; i < physicalDeviceCount; i++)
	{
		VkPhysicalDeviceProperties properties;
		vkGetPhysicalDeviceProperties(pVkDevicess[i], &properties);

		VkPhysicalDeviceFeatures features;
		vkGetPhysicalDeviceFeatures(pVkDevicess[i], &features);

		VkPhysicalDeviceMemoryProperties memory;
		vkGetPhysicalDeviceMemoryProperties(pVkDevicess[i], &memory);

		VkPhysicalDeviceRequirements requirements = {
			.bGraphics = TRUE,
			.bPresent = TRUE,
			.bTransfer = TRUE,
			.bSamplerAnisotropy = TRUE,
			.bDiscreteGpu = TRUE,
			.ppDeviceExtensionNames = darray_create(const char *)
		};
		darray_push(requirements.ppDeviceExtensionNames, &VK_KHR_SWAPCHAIN_EXTENSION_NAME);

		VkPhysicalDeviceQueueFamilyInfo queueInfo = {0};
		b8 bResult = PhysicalDeviceMeetsRequirements(pVkDevicess[i], pCtx->surface, &properties, &features,
				&requirements, &queueInfo, &pCtx->device.swapchainSupport);

		if (bResult)
		{
			YINFO("Selected device: '%s'.", properties.deviceName);
			// GPU type, etc.
			switch (properties.deviceType)
			{
				default:
				case VK_PHYSICAL_DEVICE_TYPE_OTHER: YINFO("GPU type is Unknown."); break;
				case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU: YINFO("GPU type is Integrated."); break;
				case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU: YINFO("GPU type is Discrete."); break;
				case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU: YINFO("GPU type is Virtual."); break;
				case VK_PHYSICAL_DEVICE_TYPE_CPU: YINFO("GPU type is CPU."); break;
			}
			YINFO("GPU Driver version: %d.%d.%d || Vulkan API version: %d.%d.%d",
					VK_VERSION_MAJOR(properties.driverVersion),
					VK_VERSION_MINOR(properties.driverVersion),
					VK_VERSION_PATCH(properties.driverVersion),
					VK_VERSION_MAJOR(properties.apiVersion),
					VK_VERSION_MINOR(properties.apiVersion),
					VK_VERSION_PATCH(properties.apiVersion));
			// Memory information
			for (u32 j = 0; j < memory.memoryHeapCount; ++j)
			{
				f32 memorySizeGib = (((f32)memory.memoryHeaps[j].size) / 1024.0f / 1024.0f / 1024.0f);
				if (memory.memoryHeaps[j].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT)
				{ YINFO("Local GPU memory: %.2f GiB", memorySizeGib); }
				else
				{ YINFO("Shared System memory: %.2f GiB", memorySizeGib); }
			}

			pCtx->device.physicalDev = pVkDevicess[i];
			pCtx->device.graphicsQueueIndex = queueInfo.graphicsFamilyIndex;
			pCtx->device.presentQueueIndex = queueInfo.presentFamilyIndex;
			pCtx->device.transferQueueIndex = queueInfo.transferFamilyIndex;
			// NOTE: set compute index here if needed.

			// Keep a copy of properties, features and memory info for later use.
			pCtx->device.properties = properties;
			pCtx->device.features = features;
			pCtx->device.memory = memory;
			break;
		}
	}
	if (!pCtx->device.physicalDev)
	{ 
		YERROR("No physical device found met the requirements.");
		return VK_ERROR_UNKNOWN;
	}
	return VK_SUCCESS;
}

YND VkResult
VulkanCreateDevice(VkContext *pCtx, YMB char *pGPUName)
{
	VkResult errcode = 0;

	VK_CHECK(VulkanDeviceSelect(pCtx));

	YINFO("Creating logical device...");
	uint32_t queueCreateInfoCount = 1;
	uint8_t index = 0;
	b8 bPresentSharesGraphicsQueue =
		pCtx->device.graphicsQueueIndex == pCtx->device.presentQueueIndex;
	b8 bTransferSharesGraphicsQueue =
		pCtx->device.graphicsQueueIndex == pCtx->device.transferQueueIndex;

	if (!bPresentSharesGraphicsQueue) queueCreateInfoCount++;
	if (!bTransferSharesGraphicsQueue) queueCreateInfoCount++;

	uint32_t pIndices[queueCreateInfoCount];
	pIndices[index++] = pCtx->device.graphicsQueueIndex;

	if (!bPresentSharesGraphicsQueue) pIndices[index++] = pCtx->device.presentQueueIndex;
	if (!bTransferSharesGraphicsQueue) pIndices[index++] = pCtx->device.transferQueueIndex;

	VkDeviceQueueCreateInfo pQueueCreateInfos[queueCreateInfoCount];
	for (uint32_t i = 0; i < queueCreateInfoCount; ++i)
	{
		pQueueCreateInfos[i].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		pQueueCreateInfos[i].queueFamilyIndex = pIndices[i];
		pQueueCreateInfos[i].queueCount = 1;
		/*
		 * TODO enable this for future enhancements.
		 * 	if (indices[i] == context->device.graphics_queue_index)
		 * 	{
		 * 		pQueueCreateInfos[i].queueCount = 2;
		 * 	}
		 */
		pQueueCreateInfos[i].flags = 0;
		pQueueCreateInfos[i].pNext = 0;
		f32 queue_priority = 1.0f;
		pQueueCreateInfos[i].pQueuePriorities = &queue_priority;
	}
	/* TODO: shoud be config driven */
	VkPhysicalDeviceFeatures enabledFeatures = {
		.samplerAnisotropy = VK_TRUE,
	};
	const char *pExtensionNames = VK_KHR_SWAPCHAIN_EXTENSION_NAME;
	VkDeviceCreateInfo deviceCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
		.queueCreateInfoCount = queueCreateInfoCount,
		.pQueueCreateInfos = pQueueCreateInfos,
		.pEnabledFeatures = &enabledFeatures,
		.enabledExtensionCount = 1,
		.ppEnabledExtensionNames = &pExtensionNames,
	};
	errcode = vkCreateDevice(pCtx->device.physicalDev, &deviceCreateInfo,
			pCtx->pAllocator, &pCtx->device.logicalDev);
	if (errcode != VK_SUCCESS) { YERROR("%s", string_VkResult(errcode)); return errcode; }
	YINFO("Logical device created.");
	vkGetDeviceQueue(pCtx->device.logicalDev, 
			pCtx->device.presentQueueIndex, 0, &pCtx->device.presentQueue);

	vkGetDeviceQueue(pCtx->device.logicalDev, 
			pCtx->device.graphicsQueueIndex, 0, &pCtx->device.graphicsQueue);

	vkGetDeviceQueue(pCtx->device.logicalDev, 
			pCtx->device.transferQueueIndex, 0, &pCtx->device.transferQueue);

	YINFO("Queues obtained.");
	return errcode;
}
