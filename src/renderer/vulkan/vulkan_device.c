#include "vulkan_device.h"
#include "vulkan_swapchain.h"
#include "core/ymemory.h"
#include "core/logger.h"
#include "core/darray.h"
#include "core/darray_debug.h"

#include <string.h>
#include <stdlib.h>

static VkResult VulkanDeviceSelect(VkContext *pCtx);

YND VkResult
VulkanCreateDevice(VkContext *pCtx, VkDevice *pOutDevice, YMB char *pGPUName)
{
	uint32_t queueCreateInfoCount = 1;
	uint8_t index = 0;

	VK_CHECK(VulkanDeviceSelect(pCtx));

	YINFO("Creating logical device...");

	b8 bPresentSharesGraphicsQueue = pCtx->device.graphicsQueueIndex == pCtx->device.presentQueueIndex;
	b8 bTransferSharesGraphicsQueue = pCtx->device.graphicsQueueIndex == pCtx->device.transferQueueIndex;

	if (!bPresentSharesGraphicsQueue) 
		queueCreateInfoCount++;
	if (!bTransferSharesGraphicsQueue) 
		queueCreateInfoCount++;

	uint32_t pIndices[queueCreateInfoCount];
	pIndices[index++] = pCtx->device.graphicsQueueIndex;

	if (!bPresentSharesGraphicsQueue) 
		pIndices[index++] = pCtx->device.presentQueueIndex;
	if (!bTransferSharesGraphicsQueue) 
		pIndices[index++] = pCtx->device.transferQueueIndex;

	struct temp {int32_t count; uint32_t index;};
	struct temp tempIndices[queueCreateInfoCount];
	for (uint32_t i = 0; i < queueCreateInfoCount; i++)
	{
		tempIndices[i].index = -1;
		tempIndices[i].count = 0;
	}
	uint32_t realQueueCreateInfoCount = 0;

	YMB uint32_t i = 0;
	YMB uint32_t j = 0;
	/* Remove duplicates */
	while (i < queueCreateInfoCount)
	{
		if (tempIndices[realQueueCreateInfoCount].index != pIndices[i])
		{
			tempIndices[realQueueCreateInfoCount].index = pIndices[i];
			while ( j < queueCreateInfoCount && tempIndices[realQueueCreateInfoCount].index == pIndices[j])
			{
					tempIndices[realQueueCreateInfoCount].count++;
					j++;
			}
			i++;
			if (tempIndices[realQueueCreateInfoCount].count > 0)
				realQueueCreateInfoCount++;
			continue;
		}
		i++;
	}

	VkDeviceQueueCreateInfo pQueueCreateInfos[realQueueCreateInfoCount];
	f32 queue_priority = 1.0f;
	for (uint32_t i = 0; i < realQueueCreateInfoCount; ++i)
	{
		pQueueCreateInfos[i].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		pQueueCreateInfos[i].queueFamilyIndex = tempIndices[i].index;
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
		pQueueCreateInfos[i].pQueuePriorities = &queue_priority;
	}
	VkPhysicalDeviceBufferDeviceAddressFeatures deviceAddressFeature = {
		.sType					= VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES,
		.bufferDeviceAddress	= VK_TRUE,
	};

	VkPhysicalDeviceDynamicRenderingFeatures dynamicRendering = {
		.sType				= VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES,
		.dynamicRendering	= VK_TRUE,
		.pNext				= &deviceAddressFeature,
	};

	VkPhysicalDeviceSynchronization2Features physicalDeviceSynch2Features =  {
		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES,
		.synchronization2 = VK_TRUE,
		.pNext = &dynamicRendering,
	};

	/* TODO: shoud be config driven */
	VkPhysicalDeviceFeatures enabledFeatures = {
		.samplerAnisotropy = VK_TRUE,
	};

	VkPhysicalDeviceFeatures2 enabledFeatures2 = {
		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
		.features = enabledFeatures,
		.pNext = &physicalDeviceSynch2Features,
	};
	vkGetPhysicalDeviceFeatures2(pCtx->device.physicalDevice, &enabledFeatures2);
	YDEBUG("deviceAddressFeature.bufferDeviceAddress %d", deviceAddressFeature.bufferDeviceAddress);
	YASSERT(deviceAddressFeature.bufferDeviceAddress);
	const char **ppExtensionNames = DarrayCreate(const char *);
	DarrayPush(ppExtensionNames, &VK_KHR_SWAPCHAIN_EXTENSION_NAME);
	DarrayPush(ppExtensionNames, &VK_EXT_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME );
	/* DarrayPush(ppExtensionNames, &VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME); */
	YMB uint32_t extensionCount = DarrayLength(ppExtensionNames);

	VkDeviceCreateInfo deviceCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
		.queueCreateInfoCount = realQueueCreateInfoCount,
		.pQueueCreateInfos = pQueueCreateInfos,
		.enabledExtensionCount = extensionCount,
		.ppEnabledExtensionNames = ppExtensionNames,
		.pNext = &enabledFeatures2,
	};
	VK_CHECK(vkCreateDevice(pCtx->device.physicalDevice, &deviceCreateInfo, pCtx->pAllocator, pOutDevice));
	YINFO("Logical device created.");

	vkGetDeviceQueue(pCtx->device.handle, pCtx->device.presentQueueIndex, 0, &pCtx->device.presentQueue);
	vkGetDeviceQueue(pCtx->device.handle, pCtx->device.graphicsQueueIndex, 0, &pCtx->device.graphicsQueue);
	vkGetDeviceQueue(pCtx->device.handle, pCtx->device.transferQueueIndex, 0, &pCtx->device.transferQueue);

	DarrayDestroy(ppExtensionNames);

	YINFO("Queues obtained.");
	return VK_SUCCESS;
}

static b8 
PhysicalDeviceMeetsRequirements( VkPhysicalDevice device, VkSurfaceKHR surface,
		const VkPhysicalDeviceProperties* pProperties,
		const VkPhysicalDeviceFeatures* pFeatures,
		const VkPhysicalDeviceRequirements* pRequirements,
		VkPhysicalDeviceQueueFamilyInfo* pOutQueueInfo,
		VkSwapchainSupportInfo* pOutSwapchainSupport)
{
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
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, 0);
    VkQueueFamilyProperties queueFamilies[queueFamilyCount];
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies);
    // Look at each queue and see what queues it supports
    uint8_t minTransferScore = 255;
    for (uint32_t i = 0; i < queueFamilyCount; ++i)
	{
        uint8_t currentTransferScore = 0;
        // Graphics queue?
        if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
		{
            pOutQueueInfo->graphicsFamilyIndex = i;
            ++currentTransferScore;
        }
        // Compute queue?
        if (queueFamilies[i].queueFlags & VK_QUEUE_COMPUTE_BIT)
		{
            pOutQueueInfo->computeFamilyIndex = i;
            ++currentTransferScore;
        }
        // Transfer queue?
        if (queueFamilies[i].queueFlags & VK_QUEUE_TRANSFER_BIT)
		{
            // Take the index if it is the current lowest. This increases the
            // liklihood that it is a dedicated transfer queue.
            if (currentTransferScore <= minTransferScore)
			{
                minTransferScore = currentTransferScore;
                pOutQueueInfo->transferFamilyIndex = i;
            }
        }
        // Present queue?
        VkBool32 supportsPresent = VK_FALSE;
        VK_CHECK(vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &supportsPresent));
        if (supportsPresent)
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
        VK_CHECK(vkDeviceQuerySwapchainSupport(device, surface, pOutSwapchainSupport));
        if (pOutSwapchainSupport->formatCount < 1 || pOutSwapchainSupport->presentModeCount < 1)
		{
			/* TODO: Add check if NULL so we dont substract the size and prevent checking here */
            if (pOutSwapchainSupport->pFormats)
				yFree(pOutSwapchainSupport->pFormats, pOutSwapchainSupport->formatCount, MEMORY_TAG_RENDERER);
            if (pOutSwapchainSupport->pPresentModes)
				yFree(pOutSwapchainSupport->pPresentModes, pOutSwapchainSupport->presentModeCount, MEMORY_TAG_RENDERER);
            YINFO("Required swapchain support not present, skipping device.");
            return FALSE;
        }
        // Device extensions.
        if (pRequirements->ppDeviceExtensionNames)
		{
            uint32_t availableExtensionCount = 0;
            VkExtensionProperties* pAvailableExtensions = 0;
            VK_CHECK(vkEnumerateDeviceExtensionProperties(device, 0, &availableExtensionCount, VK_NULL_HANDLE));

            if (availableExtensionCount > 0)
			{
                pAvailableExtensions =
					yAlloc(sizeof(VkExtensionProperties) * availableExtensionCount, MEMORY_TAG_RENDERER);
                VK_CHECK(
						vkEnumerateDeviceExtensionProperties(device, 0, &availableExtensionCount, pAvailableExtensions));

                uint32_t requiredExtensionCount = DarrayLength(pRequirements->ppDeviceExtensionNames);
                for (uint32_t i = 0; i < requiredExtensionCount; ++i)
				{
                    b8 bFound = FALSE;
                    for (uint32_t j = 0; j < availableExtensionCount; ++j)
					{
                        if (!strcmp(pRequirements->ppDeviceExtensionNames[i], pAvailableExtensions[j].extensionName))
						{
							YINFO("Extension[%d]: %s", j, pAvailableExtensions[j].extensionName);
                            bFound = TRUE;
                            break;
                        }
                    }
                    if (!bFound)
					{
                        YINFO("Required extension not found: '%s', skipping device.",
								pRequirements->ppDeviceExtensionNames[i]);
                        yFree(pAvailableExtensions, availableExtensionCount, MEMORY_TAG_RENDERER);
						return FALSE;
                    }
                }
				yFree(pAvailableExtensions, availableExtensionCount, MEMORY_TAG_RENDERER);
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

static VkResult
VulkanDeviceSelect(VkContext *pCtx)
{
	uint32_t physicalDeviceCount = 0;
	VK_CHECK(vkEnumeratePhysicalDevices(pCtx->instance, &physicalDeviceCount, VK_NULL_HANDLE));

	/* VkPhysicalDevice *pVkDevicess = malloc(sizeof(VkPhysicalDevice) * physicalDeviceCount); */
	VkPhysicalDevice pVkDevices[physicalDeviceCount];
	VK_CHECK(vkEnumeratePhysicalDevices(pCtx->instance, &physicalDeviceCount, pVkDevices));

	for (uint32_t i = 0; i < physicalDeviceCount; i++)
	{
		VkPhysicalDeviceProperties properties;
		vkGetPhysicalDeviceProperties(pVkDevices[i], &properties);

		VkPhysicalDeviceFeatures features;
		vkGetPhysicalDeviceFeatures(pVkDevices[i], &features);

		VkPhysicalDeviceMemoryProperties memory;
		vkGetPhysicalDeviceMemoryProperties(pVkDevices[i], &memory);

		VkPhysicalDeviceRequirements requirements = {
			.bGraphics = TRUE,
			.bPresent = TRUE,
			.bTransfer = TRUE,
			.bSamplerAnisotropy = TRUE,
			.bDiscreteGpu = TRUE,
		};
		requirements.ppDeviceExtensionNames = DarrayCreate(const char *);
		DarrayPush(requirements.ppDeviceExtensionNames, &VK_KHR_SWAPCHAIN_EXTENSION_NAME);
		DarrayPush(requirements.ppDeviceExtensionNames, &VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME);

		VkPhysicalDeviceQueueFamilyInfo queueInfo = {0};
		b8 bResult = PhysicalDeviceMeetsRequirements(pVkDevices[i], pCtx->surface, &properties, &features,
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
			for (uint32_t j = 0; j < memory.memoryHeapCount; ++j)
			{
				YMB f32 memorySizeGib = (((f32)memory.memoryHeaps[j].size) / 1024.0f / 1024.0f / 1024.0f);
				if (memory.memoryHeaps[j].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT)
				{ YINFO("Local GPU memory: %.2f GiB", memorySizeGib); }
				else
				{ YINFO("Shared System memory: %.2f GiB", memorySizeGib); }
			}

			pCtx->device.physicalDevice = pVkDevices[i];
			pCtx->device.graphicsQueueIndex = queueInfo.graphicsFamilyIndex;
			pCtx->device.presentQueueIndex = queueInfo.presentFamilyIndex;
			pCtx->device.transferQueueIndex = queueInfo.transferFamilyIndex;
			// NOTE: set compute index here if needed.

			// Keep a copy of properties, features and memory info for later use.
			pCtx->device.properties = properties;
			pCtx->device.features = features;
			pCtx->device.memory = memory;
			DarrayDestroy(requirements.ppDeviceExtensionNames);
			break;
		}
		DarrayDestroy(requirements.ppDeviceExtensionNames);
	}
	if (!pCtx->device.physicalDevice)
	{ 
		YERROR("No physical device found met the requirements.");
		return VK_ERROR_UNKNOWN;
	}
	return VK_SUCCESS;
}
