#include "os.h"

#include "yvulkan.h"
#include "vulkan_image.h"
#include "vulkan_device.h"
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
static GlobalContext gContext;

YND static int32_t MemoryFindIndex(uint32_t typeFilter, uint32_t propertyFlags);

VKAPI_ATTR VkBool32 VKAPI_CALL 
vkDebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageTypes,
		const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData, void *pUserData);

static void
vkFramebuffersRegenerate(VkContext* pCtx, VkSwapchain* pSwapchain, VulkanRenderPass* pRenderpass)
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
				pCtx,
				pRenderpass,
				pCtx->framebufferWidth,
				pCtx->framebufferHeight,
				attachmentCount,
				pAttachments,
				&pSwapchain->pFramebuffers[i]);
	}
}

static inline void
SyncInit(VkContext* pCtx, VkDevice device)
{
    /*
	 * NOTE: one fence to control when the gpu has finished rendering the frame,
	 * and 2 semaphores to synchronize rendering with swapchain
	 * we want the fence to start signalled so we can wait on it on the first frame
	 * This will prevent the application from waiting indefinitely for the first frame to render since it
	 * cannot be rendered until a frame is "rendered" before it.
     */
	for (uint8_t i = 0; i < pCtx->swapchain.maxFrameInFlight; i++)
	{
		VkSemaphoreCreateInfo semaphoreCreateInfo = {VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
		vkCreateSemaphore(device, &semaphoreCreateInfo, pCtx->pAllocator, &pCtx->pSemaphoresAvailableImage[i]);
		vkCreateSemaphore(device, &semaphoreCreateInfo, pCtx->pAllocator, &pCtx->pSemaphoresQueueComplete[i]);

		/* NOTE: Assuming that the app CANNOT run without fences therefore we abort() in case of failure */
		b8 bSignaled = TRUE;
		VK_ASSERT(vkFenceCreate(device, bSignaled, pCtx->pAllocator, &pCtx->pFencesInFlight[i]));
	}
}

/* TODO: Add some sort of config file */
YND VkResult
vkInit(OsState *pOsState, void** ppOutCtx)
{
	char*			pGPUName = "NVIDIA GeForce RTX 3080";
	const char**	ppRequiredValidationLayerNames = 0;
	const char**	ppRequiredExtensions = DarrayCreate(const char *);
	uint32_t		requiredValidationLayerCount = 0;
	uint32_t		requiredExtensionCount = 0;

	/* 
	 * NOTE: This is just for fun, multiple context is complex
	 * MAX_CONTEXT = 100; VkContext = 2168 bytes.
	 * So gContext.ppCtx = at least 216,8 kilobytes.
	 */  
	gContext.ppCtx[gContext.contextCount] = malloc(sizeof(VkContext));
	gContext.contextCount++;

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
	vkFramebuffersRegenerate(&gVkCtx, &gVkCtx.swapchain, &gVkCtx.mainRenderpass);

	gVkCtx.pSemaphoresAvailableImage = DarrayReserve(VkSemaphore, gVkCtx.swapchain.maxFrameInFlight);
	gVkCtx.pSemaphoresQueueComplete = DarrayReserve(VkSemaphore, gVkCtx.swapchain.maxFrameInFlight);
	gVkCtx.pFencesInFlight = DarrayReserve(VulkanFence, gVkCtx.swapchain.maxFrameInFlight);

	/* NOTE: Init semaphore and fences */
	SyncInit(&gVkCtx, gVkCtx.device.logicalDev);

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
	(*ppOutCtx) = &gVkCtx;
	return VK_SUCCESS;
}

void
vkShutdown(void *pContext)
{
	VkContext *pCtx = (VkContext*)pContext;
	VkDevice device = pCtx->device.logicalDev;
	VulkanDevice myDevice = pCtx->device;
	VkAllocationCallbacks* pAllocator = pCtx->pAllocator;

#ifdef DEBUG
		PFN_vkDestroyDebugUtilsMessengerEXT pfnDestroyDebug = (PFN_vkDestroyDebugUtilsMessengerEXT)
			vkGetInstanceProcAddr(gVkCtx.instance, "vkDestroyDebugUtilsMessengerEXT");

		KASSERT_MSG(pfnDestroyDebug, "Failed to create debug destroy messenger!");
		pfnDestroyDebug(pCtx->instance, pCtx->debugMessenger, pAllocator);
#endif // DEBUG
	
	VK_ASSERT(vkDeviceWaitIdle(device));
	vkDestroyPipeline(device, pCtx->gradientComputePipeline, pAllocator);
	vkDestroyPipelineLayout(device, pCtx->gradientComputePipelineLayout, pAllocator);

	vkDestroyDescriptorSetLayout(device, pCtx->drawImageDescriptorSetLayout, pAllocator);
	vkDestroyDescriptorPool(device, pCtx->descriptorPool.handle, pAllocator);

	uint32_t poolCount = 1;
	VK_ASSERT(vkCommandBufferFree(pCtx, pCtx->pGfxCommands, &myDevice.graphicsCommandPool, pCtx->swapchain.imageCount));
	VK_ASSERT(vkCommandPoolDestroy(pCtx, &myDevice.graphicsCommandPool, poolCount));

	VK_ASSERT(vkSwapchainDestroy(pCtx, &pCtx->swapchain));
	vkDestroyImage(device, pCtx->drawImage.image.handle, pAllocator);
	vkDestroyImageView(device, pCtx->drawImage.image.view, pAllocator);
	vkFreeMemory(device, pCtx->drawImage.image.memory, pAllocator);
    /*
	 * vkDestroyVulkanImage(&gVkCtx, &gVkCtx.swapchain.depthAttachment);
	 * for (uint32_t i = 0; i < gVkCtx.swapchain.imageCount; i++)
	 * 	vkDestroyImageView(device, gVkCtx.swapchain.pViews[i], gVkCtx.pAllocator);
	 * vkDestroySwapchainKHR(device, gVkCtx.swapchain.handle, gVkCtx.pAllocator);
     */
	vkDestroySurfaceKHR(pCtx->instance, pCtx->surface, pAllocator);

	for (uint32_t i = 0; i < pCtx->swapchain.imageCount; i++)
	{
		vkFramebufferDestroy(device, pAllocator, &pCtx->swapchain.pFramebuffers[i]);
	}

	DarrayDestroy(pCtx->swapchain.pFramebuffers);

	vkRenderPassDestroy(pCtx, &pCtx->mainRenderpass); 
	for (uint8_t i = 0; i < pCtx->swapchain.maxFrameInFlight; i++)
	{
		vkDestroySemaphore(device, pCtx->pSemaphoresAvailableImage[i], pAllocator);
		vkDestroySemaphore(device, pCtx->pSemaphoresQueueComplete[i], pAllocator);
		vkFenceDestroy(pCtx, &pCtx->pFencesInFlight[i]);
	}
	DarrayDestroy(pCtx->pSemaphoresQueueComplete);
	DarrayDestroy(pCtx->pSemaphoresAvailableImage);
	DarrayDestroy(pCtx->pFencesInFlight);
	DarrayDestroy(pCtx->ppImagesInFlight);
	DarrayDestroy(pCtx->pGfxCommands);

	/* TODO: Add check if NULL so we dont substract the size and prevent checking here */
	if (pCtx->device.swapchainSupport.pFormats)
	{
		yFree(pCtx->device.swapchainSupport.pFormats, pCtx->device.swapchainSupport.formatCount, 
				MEMORY_TAG_RENDERER);
	}
	if (pCtx->device.swapchainSupport.pPresentModes)
	{
		yFree(pCtx->device.swapchainSupport.pPresentModes, pCtx->device.swapchainSupport.presentModeCount, 
				MEMORY_TAG_RENDERER);
	}
	vkDestroyDevice(pCtx->device.logicalDev, pAllocator);
	vkDestroyInstance(pCtx->instance, pAllocator);
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
