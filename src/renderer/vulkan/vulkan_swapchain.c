#include "vulkan_swapchain.h"
#include "core/ymemory.h"
#include "core/logger.h"

#include <vulkan/vk_enum_string_helper.h>

#include "profiler.h"

/* TODO: Resize !!!!!!!!!!! */
YND VkResult 
vkImageViewCreate(VkContext* pContext, VkFormat format, VulkanImage* pImage, VkImageAspectFlags aspectFlags)
{
    VkImageViewCreateInfo viewCreateInfo = {
		.sType								= VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
		.image								= pImage->handle,
		.viewType							= VK_IMAGE_VIEW_TYPE_2D,  // TODO: Make configurable.
		.format								= format,

		/* TODO: Make it configurable */
		.subresourceRange.baseMipLevel		= 0,
		.subresourceRange.levelCount		= 1,
		.subresourceRange.baseArrayLayer	= 0,
		.subresourceRange.layerCount		= 1,
		.subresourceRange.aspectMask		= aspectFlags,
	};

	VK_CHECK(vkCreateImageView(
				pContext->device.handle,
				&viewCreateInfo,
				pContext->pAllocator,
				&pImage->view));

	return VK_SUCCESS;
}
/*
 * TODO: This function sucks, enhance it
 * - Errors
 * - Params
 */
YND VkResult 
vkImageCreate(
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
		uint32_t							mipLevels)
{
    pOutImage->width = width;
    pOutImage->height = height;

    VkImageCreateInfo imageCreateInfo = {
		.sType			= VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
		.imageType		= imageType,
		.extent			= extent,
		.mipLevels		= mipLevels,				// TODO: Support mip mapping
		.arrayLayers	= 1,						// TODO: Support number of layers in the image.
		.format			= format,
		.tiling			= tiling,
		.initialLayout	= VK_IMAGE_LAYOUT_UNDEFINED,
		.usage			= usage,
		.samples		= VK_SAMPLE_COUNT_1_BIT,	// TODO: Configurable sample count.
		.sharingMode	= VK_SHARING_MODE_EXCLUSIVE	// TODO: Configurable sharing mode.
	};

    VK_CHECK(vkCreateImage(pContext->device.handle, &imageCreateInfo, pContext->pAllocator, &pOutImage->handle));

    /* NOTE: Query memory requirements. */
    VkMemoryRequirements memoryRequirements;
    vkGetImageMemoryRequirements(pContext->device.handle, pOutImage->handle, &memoryRequirements);

    int32_t memoryType;

	VK_RESULT(pContext->MemoryFindIndex(
				pContext->device.physicalDevice,
				memoryRequirements.memoryTypeBits,
				memoryFlags,
				&memoryType));

    VkMemoryAllocateInfo memAllocInfo = {
		.sType				= VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		.allocationSize		= memoryRequirements.size,
		.memoryTypeIndex	= memoryType,
	};
	VK_ASSERT(vkAllocateMemory(pContext->device.handle, &memAllocInfo, pContext->pAllocator, &pOutImage->memory));

	// TODO: configurable memory offset.
    VK_ASSERT(vkBindImageMemory(pContext->device.handle, pOutImage->handle, pOutImage->memory, 0));
    if (bCreateView)
	{
        pOutImage->view = VK_NULL_HANDLE;
		/* WARN: This needs to be freed */
        VK_CHECK(vkImageViewCreate(pContext, format, pOutImage, viewAspectFlags));
    }
	return VK_SUCCESS;
}

b8
vkDeviceDetectDepthFormat(VulkanDevice *pDevice)
{
	VkFormat pCandidates[3] = {
		VK_FORMAT_D32_SFLOAT,
		VK_FORMAT_D32_SFLOAT_S8_UINT,
		VK_FORMAT_D24_UNORM_S8_UINT
	};
	const uint64_t candidateCount = 3;
	uint32_t flags = VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT;

	for (uint64_t i = 0; i < candidateCount; i++)
	{
		VkFormatProperties properties;
		vkGetPhysicalDeviceFormatProperties(pDevice->physicalDevice, pCandidates[i], &properties);
		if ((properties.linearTilingFeatures & flags) == flags )
		{
			pDevice->depthFormat = pCandidates[i];
			return TRUE;
		}
		else if ((properties.optimalTilingFeatures & flags) == flags)
		{
			pDevice->depthFormat = pCandidates[i];
			return TRUE;
		}
	}
	return FALSE;
}

YND VkResult 
vkSwapchainCreate(VkContext* pContext, uint32_t width, uint32_t height, VkSwapchain* pSwapchain)
{
    VkExtent2D swapchainExtent = {
		.width	= width, 
		.height	= height,
	};
    pSwapchain->maxFrameInFlight = 2;

    /* NOTE: Choose a swap surface format. */
    b8 bFound = FALSE;
    for (uint32_t i = 0; i < pContext->device.swapchainSupport.formatCount; ++i)
	{
        VkSurfaceFormatKHR format = pContext->device.swapchainSupport.pFormats[i];

        /* NOTE: Preferred formats */
        if (format.format == VK_FORMAT_B8G8R8A8_UNORM 
				&& format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
		{
            pSwapchain->imageFormat = format;
            bFound = TRUE;
            break;
        }
    }
    if (!bFound)
		pSwapchain->imageFormat = pContext->device.swapchainSupport.pFormats[0];

    /* VkPresentModeKHR present_mode = VK_PRESENT_MODE_FIFO_KHR; */
	/* VkPresentModeKHR desiredMode = VK_PRESENT_MODE_MAILBOX_KHR; */
	VkPresentModeKHR desiredMode = VK_PRESENT_MODE_FIFO_KHR;
	VkPresentModeKHR presentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
    for (uint32_t i = 0; i < pContext->device.swapchainSupport.presentModeCount; ++i)
	{
        VkPresentModeKHR mode = pContext->device.swapchainSupport.pPresentModes[i];
        if (mode == desiredMode)
		{
            presentMode = mode;
            break;
        }
    }
    /* NOTE: Requery swapchain support. */
    VK_CHECK(vkDeviceQuerySwapchainSupport(
				pContext->device.physicalDevice,
				pContext->surface,
				&pContext->device.swapchainSupport));

    /* NOTE: Swapchain extent */
    if (pContext->device.swapchainSupport.capabilities.currentExtent.width != UINT32_MAX)
	{
		swapchainExtent = pContext->device.swapchainSupport.capabilities.currentExtent;
	}

    /* NOTE: Clamp to the value allowed by the GPU. */
    VkExtent2D min = pContext->device.swapchainSupport.capabilities.minImageExtent;
    VkExtent2D max = pContext->device.swapchainSupport.capabilities.maxImageExtent;

    swapchainExtent.width = YCLAMP(swapchainExtent.width, min.width, max.width);
    swapchainExtent.height = YCLAMP(swapchainExtent.height, min.height, max.height);

    uint32_t imageCount = pContext->device.swapchainSupport.capabilities.minImageCount + 1;

    if (pContext->device.swapchainSupport.capabilities.maxImageCount > 0 
			&& imageCount > pContext->device.swapchainSupport.capabilities.maxImageCount)
	{
        imageCount = pContext->device.swapchainSupport.capabilities.maxImageCount;
    }

    /* NOTE: Swapchain create info */
    VkSwapchainCreateInfoKHR swapchainCreateInfo = {
		.sType				= VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
    	.surface			= pContext->surface,
    	.minImageCount		= imageCount,
    	.imageFormat		= pSwapchain->imageFormat.format,
    	.imageColorSpace	= pSwapchain->imageFormat.colorSpace,
    	.imageExtent		= swapchainExtent,
		.imageArrayLayers	= 1,
		.imageUsage			= VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
		/* .imageUsage			= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT */
	};

	/* NOTE: Setup the queue family indices */
	uint32_t queueFamilyIndices[] = {
		(uint32_t)pContext->device.graphicsQueueIndex,
		(uint32_t)pContext->device.presentQueueIndex
	};
	if (pContext->device.graphicsQueueIndex != pContext->device.presentQueueIndex)
	{
		swapchainCreateInfo.imageSharingMode		= VK_SHARING_MODE_CONCURRENT;
		swapchainCreateInfo.queueFamilyIndexCount	= 2;
		swapchainCreateInfo.pQueueFamilyIndices		= queueFamilyIndices;
	}
	else
	{
		swapchainCreateInfo.imageSharingMode		= VK_SHARING_MODE_EXCLUSIVE;
		swapchainCreateInfo.queueFamilyIndexCount	= 0;
		swapchainCreateInfo.pQueueFamilyIndices 	= 0;
	}

    swapchainCreateInfo.preTransform	= pContext->device.swapchainSupport.capabilities.currentTransform;
    swapchainCreateInfo.compositeAlpha	= VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchainCreateInfo.presentMode		= presentMode;
    swapchainCreateInfo.clipped			= VK_TRUE;
    swapchainCreateInfo.oldSwapchain	= 0;

    VK_CHECK(vkCreateSwapchainKHR(
				pContext->device.handle,
				&swapchainCreateInfo,
				pContext->pAllocator,
				&pSwapchain->handle));

    /* NOTE: Start with a zero frame index. */
    pContext->currentFrame = 0;

    /* NOTE: Images */
    pSwapchain->imageCount = 0;
    VK_CHECK(vkGetSwapchainImagesKHR(pContext->device.handle, pSwapchain->handle, &pSwapchain->imageCount, 0));

	pSwapchain->pImages = yAlloc(sizeof(VkImage) * pSwapchain->imageCount, MEMORY_TAG_RENDERER);
	pSwapchain->pViews = yAlloc(sizeof(VkImageView) * pSwapchain->imageCount, MEMORY_TAG_RENDERER);

	VK_CHECK(vkGetSwapchainImagesKHR(
				pContext->device.handle,
				pSwapchain->handle,
				&pSwapchain->imageCount,
				pSwapchain->pImages));

	VkImageViewCreateInfo viewInfo = {0};
    /* NOTE: Views */
    for (uint32_t i = 0; i < pSwapchain->imageCount; ++i)
	{
		/* YINFO("i: %u/%u", i + 1, pSwapchain->imageCount); */
        viewInfo.sType								= VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image								= pSwapchain->pImages[i];
        viewInfo.viewType							= VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format								= pSwapchain->imageFormat.format;
        viewInfo.subresourceRange.aspectMask		= VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.baseMipLevel		= 0;
        viewInfo.subresourceRange.levelCount		= 1;
        viewInfo.subresourceRange.baseArrayLayer	= 0;
        viewInfo.subresourceRange.layerCount		= 1;

        VK_CHECK(vkCreateImageView(
					pContext->device.handle,
					&viewInfo,
					pContext->pAllocator,
					&pSwapchain->pViews[i]));
    }

    /* NOTE: Depth resources */
    if (!vkDeviceDetectDepthFormat(&pContext->device))
	{
        pContext->device.depthFormat = VK_FORMAT_UNDEFINED;
        YFATAL("Failed to find a supported format!");
		return VK_ERROR_UNKNOWN;
    }
    /* NOTE: Create depth image and its view. */
	VkExtent3D extent = {
		.width	= swapchainExtent.width,
		.height	= swapchainExtent.height,
		.depth	= 1,
	};
	pSwapchain->extent = extent;
	VkImageUsageFlags swapchainImageUsageFlag = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
	b8 bCreateView = TRUE;
	uint32_t mipLevels = 4;
    VK_CHECK(vkImageCreate(
		/*(VkContext*)*/				pContext, 
		/*(VkImageType)*/				VK_IMAGE_TYPE_2D, 
		/*(uint32_t)*/					swapchainExtent.width, 
		/*(uint32_t)*/					swapchainExtent.height,
        /*(VkFormat)*/					pContext->device.depthFormat,
        /*(VkImageTiling)*/				VK_IMAGE_TILING_OPTIMAL,
		/*(VkImageUsageFlags)*/			swapchainImageUsageFlag,
        /*(VkMemoryPropertyFlagBits)*/	VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        /*(b8)*/						bCreateView,
        /*(VkImageAspectFlagBits)*/		VK_IMAGE_ASPECT_DEPTH_BIT,
        /*(VulkanImage*)*/				&pSwapchain->depthAttachment,
		/*(VkExtent3D)*/				extent,
		/*(uint32_t)*/					mipLevels));

	/* NOTE: Create the drawable image */
	DrawImage drawImage = {
		.format	= VK_FORMAT_R16G16B16A16_SFLOAT,

		.extent	= {
			.width	= width,
			.height	= height,
			.depth	= 1,
		},

	};
	VkImageUsageFlags drawImageUsageFlag =
		VK_IMAGE_USAGE_TRANSFER_SRC_BIT | 
		VK_IMAGE_USAGE_TRANSFER_DST_BIT |
		VK_IMAGE_USAGE_STORAGE_BIT		|
		VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	bCreateView = TRUE;
	mipLevels = 1;

	VK_CHECK(vkImageCreate(
				pContext,
				VK_IMAGE_TYPE_2D,
				drawImage.extent.width,
				drawImage.extent.height,
				drawImage.format,
				VK_IMAGE_TILING_OPTIMAL,
				drawImageUsageFlag,
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
				bCreateView,
				VK_IMAGE_ASPECT_COLOR_BIT,
				&drawImage.image,
				drawImage.extent,
				mipLevels));

	pContext->drawImage = drawImage;

//////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////

	DrawImage depthImage = {
		.format	= VK_FORMAT_D32_SFLOAT,
    
		.extent	= {
			.width	= width,
			.height	= height,
			.depth	= 1,
		},
    
	};
	VkImageUsageFlags depthImageUsageFlag = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
	bCreateView = TRUE;
	mipLevels = 1;
    
	VK_CHECK(vkImageCreate(
				pContext,
				VK_IMAGE_TYPE_2D,
				depthImage.extent.width,
				depthImage.extent.height,
				depthImage.format,
				VK_IMAGE_TILING_OPTIMAL,
				depthImageUsageFlag,
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
				bCreateView,
				VK_IMAGE_ASPECT_DEPTH_BIT,
				&depthImage.image,
				depthImage.extent,
				mipLevels));
    
	pContext->depthImage = depthImage;

    /*
	 * pContext->depthImage.format = pContext->device.depthFormat;
	 * pContext->depthImage.image = pContext->swapchain.depthAttachment;
	 * pContext->depthImage.handle = pContext->swapchain.depthAttachment.handle;
     */

    YINFO("Swapchain created successfully.");
	return VK_SUCCESS;
}

YND VkResult
vkSwapchainRecreate(VkContext *pCtx, uint32_t width, uint32_t height, VkSwapchain *pSwapchain)
{
	YMB VkDevice device = pCtx->device.handle;

	VK_RESULT(vkSwapchainDestroy(pCtx, pSwapchain));
	VK_RESULT(vkSwapchainCreate(pCtx, width, height, pSwapchain));
	return VK_SUCCESS;
}

YND VkResult 
vkSwapchainAcquireNextImageIndex(
		VkContext*							pCtx,
		VkSwapchain*						pSwapchain,
		uint64_t							timeoutNS,
		VkSemaphore							semaphoreImageAvailable,
		VkFence								fence,
		uint32_t*							pOutImageIndex)
{
    VkResult result = vkAcquireNextImageKHR(
			pCtx->device.handle,
			pSwapchain->handle,
			timeoutNS,
			semaphoreImageAvailable,
			fence,
			pOutImageIndex);
	/*
	 * NOTE: on AMD you can resize window and like you say “just wait” it never throw VK_ERROR_OUT_OF_DATE_KHR, 
	 * but on Nvidia its always VK_ERROR_OUT_OF_DATE_KHR no matter what
	 * (and crash if you dont process this destroying/freeing/recreating your allocated data). 
	 * (VK_SUBOPTIMAL_KHR happens only on X11/XCB or/and xWayland)
	 * https://community.khronos.org/t/vk-suboptimal-khr-is-it-safe-to-use-it-as-window-resize-detection/107848/5
	 */
	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
	{
		/* NOTE: Trigger pSwapchain recreation, then boot out of the render loop. */
		VK_RESULT(vkSwapchainRecreate(pCtx, pCtx->framebufferWidth, pCtx->framebufferHeight, pSwapchain));
	}
	/* WARN: Is this fatal ?! */
	else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
	{
		YFATAL("Failed to acquire pSwapchain image!");
		return result;
	}
    return VK_SUCCESS;
}

YND VkResult
vkSwapchainPresent(VkContext* pCtx, VkSwapchain* pSwapchain, YMB VkQueue gfxQueue, VkQueue presentQueue,
		VkSemaphore semaphoreRenderComplete, uint32_t presentImageIndex)
{
	// Return the image to the swapchain for presentation.
	VkPresentInfoKHR presentInfo = {
		.sType				= VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
		.waitSemaphoreCount	= 1,
		.pWaitSemaphores	= &semaphoreRenderComplete,
		.swapchainCount		= 1,
		.pSwapchains		= &pSwapchain->handle,
		.pImageIndices		= &presentImageIndex,
		.pResults			= VK_NULL_HANDLE,
	};

	/* TracyCFrameMark; */
	VkResult result = vkQueuePresentKHR(presentQueue, &presentInfo);
	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
	{
		YDEBUG("%s", string_VkResult(result));
		/* 
		 * NOTE: Swapchain is out of date, suboptimal or a framebuffer resize has occurred.
		 * Trigger swapchain recreation.
		 */
		VK_ASSERT(vkSwapchainRecreate(pCtx, pCtx->framebufferWidth, pCtx->framebufferHeight, pSwapchain));
	}
	else if (result != VK_SUCCESS)
	{
		YFATAL("Failed to present swap chain image!");
		return result;
	}
	/* NOTE: Increment (and loop) the index. */
	pCtx->currentFrame = (pCtx->currentFrame + 1) % pSwapchain->maxFrameInFlight;
	pCtx->nbFrames++;
	return VK_SUCCESS;
}

void
vkDestroyVulkanImage(VkContext *pContext, VulkanImage *pImage)
{
	if (pImage->view)
	{
		vkDestroyImageView(pContext->device.handle, pImage->view, pContext->pAllocator);
		pImage->view = 0;
	}
	if (pImage->memory)
	{
		vkFreeMemory(pContext->device.handle, pImage->memory, pContext->pAllocator);
		pImage->memory = 0;
	}
	if (pImage->handle)
	{
		vkDestroyImage(pContext->device.handle, pImage->handle, pContext->pAllocator);
		pImage->handle = 0;
	}
}

YND VkResult
vkSwapchainDestroy(VkContext* pCtx, VkSwapchain* pSwapchain)
{
	VkDevice device = pCtx->device.handle;
	VK_CHECK(vkDeviceWaitIdle(device));
		vkDestroyVulkanImage(pCtx, &pSwapchain->depthAttachment);
	for (uint32_t i = 0; i < pCtx->swapchain.imageCount; i++)
	{
		vkDestroyImageView(device, pCtx->swapchain.pViews[i], pCtx->pAllocator);
	}
	vkDestroySwapchainKHR(device, pCtx->swapchain.handle, pCtx->pAllocator);
	yFree(pCtx->swapchain.pImages, pCtx->swapchain.imageCount, MEMORY_TAG_RENDERER);
	yFree(pCtx->swapchain.pViews, pCtx->swapchain.imageCount, MEMORY_TAG_RENDERER);
	return VK_SUCCESS;
}

YND VkResult
vkDeviceQuerySwapchainSupport(VkPhysicalDevice physDevice, VkSurfaceKHR surface,VkSwapchainSupportInfo* pOutSupportInfo)
{
	/* NOTE: Surface capabilities */
    VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physDevice, surface, &pOutSupportInfo->capabilities));

    /* NOTE: Surface formats */
    VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(physDevice, surface, &pOutSupportInfo->formatCount, VK_NULL_HANDLE));

    if (pOutSupportInfo->formatCount != 0)
	{
        if (!pOutSupportInfo->pFormats)
		{
            pOutSupportInfo->pFormats =
				yAlloc(sizeof(VkSurfaceFormatKHR) * pOutSupportInfo->formatCount, MEMORY_TAG_RENDERER);
        }
		VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(
					physDevice,
					surface,
					&pOutSupportInfo->formatCount,
					pOutSupportInfo->pFormats));
    }
    /* NOTE: Present modes */
    VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(
				physDevice,
				surface,
				&pOutSupportInfo->presentModeCount,
				VK_NULL_HANDLE));

    if (pOutSupportInfo->presentModeCount != 0)
	{
        if (!pOutSupportInfo->pPresentModes)
		{
            pOutSupportInfo->pPresentModes = 
				yAlloc(sizeof(VkPresentModeKHR) * pOutSupportInfo->presentModeCount, MEMORY_TAG_RENDERER);
        }

        VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(
					physDevice,
					surface,
					&pOutSupportInfo->presentModeCount,
					pOutSupportInfo->pPresentModes));
    }
	return VK_SUCCESS;
}
