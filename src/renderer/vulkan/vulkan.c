#include "os.h"

#include "yvulkan.h"
#include "vulkan_image.h"
#include "vulkan_renderpass.h"
#include "vulkan_framebuffer.h"
#include "vulkan_swapchain.h"
#include "vulkan_command.h"
#include "vulkan_fence.h"
#include "vulkan_descriptor.h"
#include "vulkan_pipeline.h"

#include "core/darray.h"
#include "core/logger.h"
#include "core/assert.h"
#include "core/ymemory.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vulkan/vk_enum_string_helper.h>

#include "TracyC.h"

static VkContext gVkCtx;

YND VkResult VulkanCreateDevice(VkContext *pVulkanContext, VkDevice *pOutDevice, char *pGPUName);
YND int32_t MemoryFindIndex(uint32_t typeFilter, uint32_t propertyFlags);

VKAPI_ATTR VkBool32 VKAPI_CALL 
vkDebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageTypes,
		const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData, void *pUserData);

void
vkShutdown(void)
{
	VkDevice device = gVkCtx.device.logicalDev;
	VulkanDevice myDevice = gVkCtx.device;
	VkContext *pCtx = &gVkCtx;

#ifdef DEBUG
		PFN_vkDestroyDebugUtilsMessengerEXT pfnDestroyDebug = (PFN_vkDestroyDebugUtilsMessengerEXT)
			vkGetInstanceProcAddr(gVkCtx.instance, "vkDestroyDebugUtilsMessengerEXT");

		KASSERT_MSG(pfnDestroyDebug, "Failed to create debug destroy messenger!");
		pfnDestroyDebug(gVkCtx.instance, gVkCtx.debugMessenger, gVkCtx.pAllocator);
#endif // DEBUG
	
	VK_ASSERT(vkDeviceWaitIdle(device));
	vkDestroyPipeline(device, pCtx->gradientComputePipeline, pCtx->pAllocator);
	vkDestroyPipelineLayout(device, pCtx->gradientComputePipelineLayout, pCtx->pAllocator);

	vkDestroyDescriptorSetLayout(device, pCtx->drawImageDescriptorSetLayout, pCtx->pAllocator);
	vkDestroyDescriptorPool(device, pCtx->descriptorPool.handle, pCtx->pAllocator);

	VK_ASSERT(vkCommandBufferFree(pCtx, pCtx->pGfxCommands, &myDevice.graphicsCommandPool, pCtx->swapchain.imageCount));
	VK_ASSERT(vkCommandPoolDestroy(pCtx, &myDevice.graphicsCommandPool, 1));

	VK_ASSERT(vkSwapchainDestroy(pCtx, &pCtx->swapchain));
	vkDestroyImage(device, pCtx->drawImage.image.handle, pCtx->pAllocator);
	vkDestroyImageView(device, pCtx->drawImage.image.view, pCtx->pAllocator);
	vkFreeMemory(device, pCtx->drawImage.image.memory, pCtx->pAllocator);
    /*
	 * vkDestroyVulkanImage(&gVkCtx, &gVkCtx.swapchain.depthAttachment);
	 * for (uint32_t i = 0; i < gVkCtx.swapchain.imageCount; i++)
	 * 	vkDestroyImageView(device, gVkCtx.swapchain.pViews[i], gVkCtx.pAllocator);
	 * vkDestroySwapchainKHR(device, gVkCtx.swapchain.handle, gVkCtx.pAllocator);
     */
	vkDestroySurfaceKHR(gVkCtx.instance, gVkCtx.surface, gVkCtx.pAllocator);

	for (uint32_t i = 0; i < gVkCtx.swapchain.imageCount; i++)
	{
		vkFramebufferDestroy(&gVkCtx, &gVkCtx.swapchain.pFramebuffers[i]);
	}

	DarrayDestroy(gVkCtx.swapchain.pFramebuffers);

	vkRenderPassDestroy(&gVkCtx, &gVkCtx.mainRenderpass); 
	for (uint8_t i = 0; i < gVkCtx.swapchain.maxFrameInFlight; i++)
	{
		vkDestroySemaphore(device, gVkCtx.pSemaphoresAvailableImage[i], gVkCtx.pAllocator);
		vkDestroySemaphore(device, gVkCtx.pSemaphoresQueueComplete[i], gVkCtx.pAllocator);
		vkFenceDestroy(&gVkCtx, &gVkCtx.pFencesInFlight[i]);
	}
	DarrayDestroy(gVkCtx.pSemaphoresQueueComplete);
	DarrayDestroy(gVkCtx.pSemaphoresAvailableImage);
	DarrayDestroy(gVkCtx.pFencesInFlight);
	DarrayDestroy(gVkCtx.ppImagesInFlight);
	DarrayDestroy(gVkCtx.pGfxCommands);

	/* TODO: Add check if NULL so we dont substract the size and prevent checking here */
	if (gVkCtx.device.swapchainSupport.pFormats)
	{
		yFree(gVkCtx.device.swapchainSupport.pFormats, gVkCtx.device.swapchainSupport.formatCount, 
				MEMORY_TAG_RENDERER);
	}
	if (gVkCtx.device.swapchainSupport.pPresentModes)
	{
		yFree(gVkCtx.device.swapchainSupport.pPresentModes, gVkCtx.device.swapchainSupport.presentModeCount, 
				MEMORY_TAG_RENDERER);
	}
	vkDestroyDevice(gVkCtx.device.logicalDev, gVkCtx.pAllocator);
	vkDestroyInstance(gVkCtx.instance, gVkCtx.pAllocator);
}

void
vkFramebuffersRegenerate(VkSwapchain *pSwapchain, VulkanRenderPass *pRenderpass)
{
	for (uint32_t i = 0; i < pSwapchain->imageCount; i++)
	{
		/*
		 * TODO: make this dynamic and based on the currently configured attachments 
		 * if using manual Renderpasses
		 */
		uint32_t attachmentCount = 2;
		VkImageView pAttachments[] = { pSwapchain->pViews[i], pSwapchain->depthAttachment.view };
		vkFramebufferCreate(
				&gVkCtx,
				pRenderpass,
				gVkCtx.framebufferWidth,
				gVkCtx.framebufferHeight,
				attachmentCount,
				pAttachments,
				&gVkCtx.swapchain.pFramebuffers[i]);
	}
}

static inline void
SyncInit(void)
{
    /*
	 * NOTE: one fence to control when the gpu has finished rendering the frame,
	 * and 2 semaphores to synchronize rendering with swapchain
	 * we want the fence to start signalled so we can wait on it on the first frame
	 * This will prevent the application from waiting indefinitely for the first frame to render since it
	 * cannot be rendered until a frame is "rendered" before it.
     */
	VkDevice device = gVkCtx.device.logicalDev;
	for (uint8_t i = 0; i < gVkCtx.swapchain.maxFrameInFlight; i++)
	{
		VkSemaphoreCreateInfo semaphoreCreateInfo = {VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
		vkCreateSemaphore(device, &semaphoreCreateInfo, gVkCtx.pAllocator, &gVkCtx.pSemaphoresAvailableImage[i]);
		vkCreateSemaphore(device, &semaphoreCreateInfo, gVkCtx.pAllocator, &gVkCtx.pSemaphoresQueueComplete[i]);

		/* NOTE: Assuming that the app CANNOT run without fences therefore we abort() in case of failure */
		VK_ASSERT(vkFenceCreate(&gVkCtx, TRUE, &gVkCtx.pFencesInFlight[i]));
	}
}

YND VkResult
vkInit(OsState *pOsState)
{
	char*			pGPUName = "NVIDIA GeForce RTX 3080";
	const char**	ppRequiredValidationLayerNames = 0;
	const char**	ppRequiredExtensions = DarrayCreate(const char *);
	uint32_t		requiredValidationLayerCount = 0;
	uint32_t		requiredExtensionCount = 0;

	DarrayPush(ppRequiredExtensions, &VK_KHR_SURFACE_EXTENSION_NAME);
	DarrayPush(ppRequiredExtensions, &VK_KHR_SURFACE_OS);

	gVkCtx.MemoryFindIndex = MemoryFindIndex;
	OsFramebufferGetDimensions(pOsState, &gVkCtx.framebufferWidth, &gVkCtx.framebufferHeight);
#ifdef DEBUG
		DarrayPush(ppRequiredExtensions, &VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
		YDEBUG("Required extensions:");
		uint32_t length = DarrayLength(ppRequiredExtensions);
		for (uint32_t i = 0; i < length; ++i)
			YDEBUG(ppRequiredExtensions[i]);

		// The list of validation layers required.
		YINFO("Validation layers enabled. Enumerating...");
		ppRequiredValidationLayerNames = DarrayCreate(const char*);
		DarrayPush(ppRequiredValidationLayerNames, &"VK_LAYER_KHRONOS_validation");
		requiredValidationLayerCount = DarrayLength(ppRequiredValidationLayerNames);
		
		// Obtain list of available validation layers.
		uint32_t availableLayerCount = 0;
		VK_CHECK(vkEnumerateInstanceLayerProperties(&availableLayerCount, 0));

		VkLayerProperties* pAvailableLayers = DarrayReserve(VkLayerProperties, availableLayerCount);
		VK_CHECK(vkEnumerateInstanceLayerProperties(&availableLayerCount, pAvailableLayers));

		//  Verify all required layers are available.
		for (uint32_t i = 0; i < requiredValidationLayerCount; ++i)
		{
			YINFO("Searching for layer: %s...", ppRequiredValidationLayerNames[i]);
			b8 bFound = FALSE;
			for (uint32_t j = 0; j < availableLayerCount; ++j)
			{
				if (!strcmp(ppRequiredValidationLayerNames[i], pAvailableLayers[j].layerName))
				{ bFound = TRUE; YINFO("Found."); break; }
			}
			if (!bFound) { YFATAL("Required validation layer is missing: %s", ppRequiredValidationLayerNames[i]); exit(1); }
		}
		DarrayDestroy(pAvailableLayers);
		YINFO("All required validation layers are present.");
#endif // DEBUG
	requiredExtensionCount = DarrayLength(ppRequiredExtensions);
	VkApplicationInfo pAppInfo = {
		.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
		.pApplicationName = "yuseong",
		.pEngineName = "yuseongEngine",
		.applicationVersion = VK_MAKE_VERSION(1, 0, 0),
		.engineVersion = VK_MAKE_VERSION(1, 0, 0),
		.apiVersion = VK_API_VERSION_1_3
	};
	VkInstanceCreateInfo pCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
		.pNext = VK_NULL_HANDLE,
		.flags = 0,
		.pApplicationInfo = &pAppInfo,
		.enabledLayerCount = requiredValidationLayerCount,
		.ppEnabledLayerNames = ppRequiredValidationLayerNames,
		/* .enabledExtensionCount = darray_length(ppRequired_extensions), */
		.enabledExtensionCount = requiredExtensionCount,
		.ppEnabledExtensionNames = ppRequiredExtensions
	};
	/* NOTE: CreateInstance and vkSurface */
	VK_ASSERT(vkCreateInstance(&pCreateInfo, gVkCtx.pAllocator, &gVkCtx.instance));
	VK_CHECK(OsCreateVkSurface(pOsState, &gVkCtx));
	YDEBUG("Vulkan surface created");
#ifdef DEBUG
		YDEBUG("Creating Vulkan debugger...");
		uint32_t logSeverity = 
			VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT;
			// | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT;
			//VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT;

		VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo = {VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT};
		debugCreateInfo.messageSeverity = logSeverity;
		debugCreateInfo.messageType = 
			VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | 
			VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT | 
			VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_TYPE_DEVICE_ADDRESS_BINDING_BIT_EXT;
		debugCreateInfo.pfnUserCallback = vkDebugCallback;

		PFN_vkCreateDebugUtilsMessengerEXT func =
			(PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(gVkCtx.instance, "vkCreateDebugUtilsMessengerEXT");
		KASSERT_MSG(func, "Failed to create debug messenger!");
		VK_CHECK(func(gVkCtx.instance, &debugCreateInfo, gVkCtx.pAllocator, &gVkCtx.debugMessenger));
		YDEBUG("Vulkan debugger created.");
#endif // DEBUG
	VK_CHECK(VulkanCreateDevice(&gVkCtx, &gVkCtx.device.logicalDev, pGPUName));

	/* NOTE: Create Swapchain and commandpool/commandbuffer */
	int32_t width = gVkCtx.framebufferWidth;
	int32_t height = gVkCtx.framebufferHeight;
	VK_CHECK(vkSwapchainCreate(&gVkCtx, width, height, &gVkCtx.swapchain));
	VK_CHECK(vkCommandPoolCreate(&gVkCtx));
	VK_CHECK(vkCommandBufferCreate(&gVkCtx));

	YMB RgbaFloat color = { .r = 30.0f, .g = 30.0f, .b = 200.0f, .a = 1.0f, };
	YMB RectFloat rect = { .x = 0.0f, .y = 0.0f, .w = gVkCtx.framebufferWidth, .h = gVkCtx.framebufferHeight, };
	YMB f32 depth = 1.0f;
	YMB f32 stencil = 0.0f;

	vkRenderPassCreate(&gVkCtx, &gVkCtx.mainRenderpass, color, rect, depth, stencil);
	gVkCtx.swapchain.pFramebuffers = DarrayReserve(VulkanFramebuffer, gVkCtx.swapchain.imageCount);
	vkFramebuffersRegenerate(&gVkCtx.swapchain, &gVkCtx.mainRenderpass);

	gVkCtx.pSemaphoresAvailableImage = DarrayReserve(VkSemaphore, gVkCtx.swapchain.maxFrameInFlight);
	gVkCtx.pSemaphoresQueueComplete = DarrayReserve(VkSemaphore, gVkCtx.swapchain.maxFrameInFlight);
	gVkCtx.pFencesInFlight = DarrayReserve(VulkanFence, gVkCtx.swapchain.maxFrameInFlight);

	/* NOTE: Init semaphore and fences */
	SyncInit();

	/*
	 * NOTE: In flight fences should not yet exist at this point, so clear the list. These are stored in pointers
	 * because the initial state should be 0, and will be 0 when not in use. Acutal fences are not owned
	 * by this list.
	 */
    gVkCtx.ppImagesInFlight = DarrayReserve(VulkanFence, gVkCtx.swapchain.imageCount);
    for (uint32_t i = 0; i < gVkCtx.swapchain.imageCount; ++i) 
	{
        gVkCtx.ppImagesInFlight[i] = 0;
    }

	/* TODO: Make it a function that looks config file for the folder ?*/
	const char *pShaderFilePath = "./build/obj/shaders/gradient.comp.spv";

	/* NOTE: DescriptorSets allocation and Pipeline for shaders creation */
	VK_CHECK(vkDescriptorsInit(&gVkCtx, gVkCtx.device.logicalDev));
	VK_CHECK(vkPipelineInit(&gVkCtx, gVkCtx.device.logicalDev, pShaderFilePath));

	YINFO("Vulkan renderer initialized.");

	/* NOTE: Cleanup */
	DarrayDestroy(ppRequiredValidationLayerNames);
	DarrayDestroy(ppRequiredExtensions);
	return VK_SUCCESS;
}

VkResult
vkComputeShaderInvocation(VkContext* pCtx, VulkanCommandBuffer* pCmd)
{
	VkPipelineBindPoint pipelineBindPoint = VK_PIPELINE_BIND_POINT_COMPUTE;
	vkCmdBindPipeline(pCmd->handle, pipelineBindPoint, pCtx->gradientComputePipeline);

	uint32_t firstSet = 0;
	uint32_t descriptorSetCount = 1;
	uint32_t dynamicOffsetCount = 0;
	const uint32_t* pDynamicOffsets = VK_NULL_HANDLE;

	vkCmdBindDescriptorSets(
			pCmd->handle,
			pipelineBindPoint,
			pCtx->gradientComputePipelineLayout,
			firstSet,
			descriptorSetCount,
			&pCtx->drawImageDescriptorSet,
			dynamicOffsetCount,
			pDynamicOffsets);

	uint32_t groupCountX = ceil(pCtx->drawImage.extent.width / 16.0f);
	uint32_t groupCountY = ceil(pCtx->drawImage.extent.height / 16.0f);
	uint32_t groupCountZ = 1;
	vkCmdDispatch(pCmd->handle, groupCountX, groupCountY, groupCountZ);
	return VK_SUCCESS;
}

void
PrintColor(RgbaFloat c)
{
	YINFO("r: %f, g: %f, b: %f, a:%f", c.r, c.g, c.b, c.a);
}

/* WARN: Leaking currently, needs to free at the beginning or at the end */
/* 
 * TODO: Profile ImageCopy&Co's
 */
YND VkResult
vkDrawImpl(void)
{
	TracyCZoneN(drawCtx, "yDraw", 1);

	YMB VkDevice device = gVkCtx.device.logicalDev;
	uint64_t fenceWaitTimeoutNs = 1000 * 1000 * 1000; // 1 sec || 1 billion nanoseconds
	VK_CHECK(vkFenceWait(&gVkCtx, &gVkCtx.pFencesInFlight[gVkCtx.currentFrame], fenceWaitTimeoutNs));

	/*
	 * NOTE: Fences have to be reset between uses, you can’t use the same
	 * fence on multiple GPU commands without resetting it in the middle.
	 */
	VK_CHECK(vkFenceReset(&gVkCtx, &gVkCtx.pFencesInFlight[gVkCtx.currentFrame]));

	/* NOTE: Get next swapchain image */
	VK_CHECK(vkSwapchainAcquireNextImageIndex(
				&gVkCtx,
				&gVkCtx.swapchain,
				fenceWaitTimeoutNs,
				gVkCtx.pSemaphoresAvailableImage[gVkCtx.currentFrame],
				VK_NULL_HANDLE, 
				&gVkCtx.imageIndex));

	/* NOTE: Getting the current frame's cmd buffer and reset it */
	VulkanCommandBuffer *pCmd = &gVkCtx.pGfxCommands[gVkCtx.imageIndex];
	VkCommandBufferResetFlags flags = 0;
	VK_CHECK(vkCommandBufferReset(pCmd, flags));

	/* NOTE: Start command recording */
	vkCommandBufferBegin(pCmd, TRUE, FALSE, FALSE);
	VkImage swapchainImage = gVkCtx.swapchain.pImages[gVkCtx.imageIndex];
	DrawImage drawImage = gVkCtx.drawImage;
	VkImageLayout currentLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	VkImageLayout newLayout = VK_IMAGE_LAYOUT_GENERAL;
	vkImageTransition(pCmd, drawImage.image.handle, currentLayout, newLayout);

	/* NOTE: Clear the background */
	/* vkClearBackground(pCmd, drawImage.image.handle); */

	/* NOTE: Compute shader invocation */
	/* vkComputeShaderInvocation(&gVkCtx, pCmd); */

	/* NOTE: Make the swapchain image into presentable mode */
	currentLayout = VK_IMAGE_LAYOUT_GENERAL;
	newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
	vkImageTransition(pCmd, drawImage.image.handle, currentLayout, newLayout);
	vkImageTransition(pCmd, swapchainImage, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

	/* NOTE: Copy the image */
	VkExtent2D swapchainExtent = {.width = gVkCtx.swapchain.extent.width, .height = drawImage.extent.height};
	VkExtent2D drawExtent = {.width = drawImage.extent.width, .height = drawImage.extent.height};
	vkImageCopy(pCmd->handle, drawImage.image.handle, swapchainImage, drawExtent, swapchainExtent);

	vkImageTransition(pCmd, swapchainImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

	/* NOTE: End command recording */
	VK_CHECK(vkCommandBufferEnd(pCmd));
	/*
	 * NOTE: submit command buffer to the queue and execute it.
	 * 	_renderFence will now block until the graphic commands finish execution
	 */
	VK_CHECK(vkQueueSubmitAndSwapchainPresent(pCmd));

	TracyCZoneEnd(drawCtx);
	return VK_SUCCESS;
}

void
vkClearBackground(VulkanCommandBuffer *pCmd, VkImage pImage)
{
	f32 flashSpeed = 960.0f;
	YMB f32 flash = fabs(sin(gVkCtx.nbFrames / flashSpeed));
	VkClearColorValue clearValue = {.float32 = {0.0f, 0.0f, flash, 1.0f}};
	VkImageSubresourceRange clearRange = {
		.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
		.baseMipLevel = 0,
		.levelCount = VK_REMAINING_MIP_LEVELS,
		.baseArrayLayer = 0,
		.layerCount = VK_REMAINING_ARRAY_LAYERS,
	};
	VkImageLayout clearColorImageLayout = VK_IMAGE_LAYOUT_GENERAL;
	uint32_t rangeCount = 1;
	vkCmdClearColorImage(pCmd->handle, pImage, clearColorImageLayout, &clearValue, rangeCount, &clearRange);
}

YND VkResult
vkQueueSubmitAndSwapchainPresent(VulkanCommandBuffer *pCmd)
{
	VkSemaphoreSubmitInfo semaphoreWaitSubmitInfo = {
		.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
		.semaphore = gVkCtx.pSemaphoresAvailableImage[gVkCtx.currentFrame],
		.stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
		.value = 1,
	};
	VkSemaphoreSubmitInfo semaphoreSignalSubmitInfo = {
		.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
		.semaphore = gVkCtx.pSemaphoresQueueComplete[gVkCtx.currentFrame],
		.stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
		.value = 1,
	};
	VkCommandBufferSubmitInfo cmdBufferSubmitInfo = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
		.commandBuffer = pCmd->handle,
	};
	VkSubmitInfo2 submitInfo2 = {
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
		.commandBufferInfoCount = 1,
		.pCommandBufferInfos = &cmdBufferSubmitInfo,
		.pWaitSemaphoreInfos = &semaphoreWaitSubmitInfo,
		.waitSemaphoreInfoCount = 1,
		.pSignalSemaphoreInfos = &semaphoreSignalSubmitInfo,
		.signalSemaphoreInfoCount = 1,
	};
	uint32_t submitCount = 1;
	VK_CHECK(vkQueueSubmit2(
				gVkCtx.device.graphicsQueue,
				submitCount,
				&submitInfo2,
				gVkCtx.pFencesInFlight[gVkCtx.currentFrame].handle));

	pCmd->state = COMMAND_BUFFER_STATE_SUBMITTED;

	VK_CHECK(vkSwapchainPresent(
				&gVkCtx,
				&gVkCtx.swapchain,
				gVkCtx.device.graphicsQueue,
				gVkCtx.device.presentQueue,
				gVkCtx.pSemaphoresQueueComplete[gVkCtx.currentFrame],
				gVkCtx.imageIndex));

	return VK_SUCCESS;
}

YND int32_t
MemoryFindIndex(uint32_t typeFilter, uint32_t propertyFlags)
{
    VkPhysicalDeviceMemoryProperties memoryProperties;
    vkGetPhysicalDeviceMemoryProperties(gVkCtx.device.physicalDev, &memoryProperties);

    for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; ++i)
	{
        // Check each memory type to see if its bit is set to 1.
        if (typeFilter & (1 << i) && (memoryProperties.memoryTypes[i].propertyFlags & propertyFlags) == propertyFlags)
		{
            return i;
        }
    }
    YWARN("Unable to find suitable memory type!");
    return -1;
}

VKAPI_ATTR VkBool32 VKAPI_CALL 
vkDebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,YMB VkDebugUtilsMessageTypeFlagsEXT messageTypes,
		const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData, YMB void *pUserData)
{
	switch (messageSeverity)
	{
		default:
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
			YERROR(pCallbackData->pMessage);
			break;
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
			YWARN(pCallbackData->pMessage);
			break;
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
			YINFO(pCallbackData->pMessage);
			break;
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
			YTRACE(pCallbackData->pMessage);
			break;
	}
	return VK_FALSE;
}
