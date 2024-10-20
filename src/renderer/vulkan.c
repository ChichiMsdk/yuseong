#include "os.h"

#include "yvulkan.h"
#include "vulkan_renderpass.h"
#include "vulkan_framebuffer.h"
#include "vulkan_swapchain.h"
#include "vulkan_command.h"

#include "core/darray.h"
#include "core/logger.h"
#include "core/assert.h"
#include "core/ymemory.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vulkan/vk_enum_string_helper.h>

static VkContext gVkCtx;

YND VkResult VulkanCreateDevice(VkContext *pVulkanContext, char *pGPUName);
YND int32_t MemoryFindIndex(uint32_t typeFilter, uint32_t propertyFlags);

VKAPI_ATTR VkBool32 VKAPI_CALL 
vkDebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageTypes,
		const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData, void *pUserData);

void
vkDestroyVulkanImage(VkContext *pContext, VulkanImage *pImage)
{
	if (pImage->view)
	{
		vkDestroyImageView(pContext->device.logicalDev, pImage->view, pContext->pAllocator);
		pImage->view = 0;
	}
	if (pImage->memory)
	{
		vkFreeMemory(pContext->device.logicalDev, pImage->memory, pContext->pAllocator);
		pImage->memory = 0;
	}
	if (pImage->handle)
	{
		vkDestroyImage(pContext->device.logicalDev, pImage->handle, pContext->pAllocator);
		pImage->handle = 0;
	}
}

void
RendererShutdown(YMB OS_State *pState)
{
	VkDevice device = gVkCtx.device.logicalDev;
	VulkanDevice myDevice = gVkCtx.device;
	VkContext *pCtx = &gVkCtx;

#ifdef DEBUG
		PFN_vkDestroyDebugUtilsMessengerEXT func = (PFN_vkDestroyDebugUtilsMessengerEXT)
			vkGetInstanceProcAddr(gVkCtx.instance, "vkDestroyDebugUtilsMessengerEXT");

		KASSERT_MSG(func, "Failed to create debug destroy messenger!");
		func(gVkCtx.instance, gVkCtx.debugMessenger, gVkCtx.pAllocator);
#endif // DEBUG

	VK_ASSERT(vkCommandBufferFree(pCtx, pCtx->pGfxCommands, &myDevice.graphicsCommandPool, pCtx->swapChain.imageCount));
	VK_ASSERT(vkCommandPoolDestroy(pCtx, &myDevice.graphicsCommandPool, 1));

	vkDestroyVulkanImage(&gVkCtx, &gVkCtx.swapChain.depthAttachment);
	for (uint32_t i = 0; i < gVkCtx.swapChain.imageCount; i++)
		vkDestroyImageView(device, gVkCtx.swapChain.pViews[i], gVkCtx.pAllocator);

	vkDestroySwapchainKHR(device, gVkCtx.swapChain.handle, gVkCtx.pAllocator);
	vkDestroySurfaceKHR(gVkCtx.instance, gVkCtx.surface, gVkCtx.pAllocator);

	vkDestroyDevice(gVkCtx.device.logicalDev, gVkCtx.pAllocator);
	vkDestroyInstance(gVkCtx.instance, gVkCtx.pAllocator);
}

void
FramebuffersRegenerate(VkSwapchain *pSwapchain, VulkanRenderPass *pRenderpass)
{
	for (uint32_t i = 0; i < pSwapchain->imageCount; i++)
	{
		/*
		 * TODO: make this dynamic and based on the currently configured attachments 
		 * if using manual Renderpasses
		 */
		uint32_t attachmentCount = 2;
		VkImageView pAttachments[] = { pSwapchain->pViews[i], pSwapchain->depthAttachment.view };
		vkFrameBufferCreate(
				&gVkCtx,
				pRenderpass,
				gVkCtx.framebufferWidth,
				gVkCtx.framebufferHeight,
				attachmentCount,
				pAttachments,
				&gVkCtx.swapChain.pFramebuffers[i]);
	}
}

YND VkResult
RendererInit(OS_State *pOsState)
{
	char*			pGPUName = "NVIDIA GeForce RTX 3080";
	const char**	ppRequiredValidationLayerNames = 0;
	const char**	ppRequired_extensions = darray_create(const char *);
	uint32_t		requiredValidationLayerCount = 0;

	darray_push(ppRequired_extensions, &VK_KHR_SURFACE_EXTENSION_NAME);
	darray_push(ppRequired_extensions, &VK_KHR_SURFACE_OS);
	gVkCtx.MemoryFindIndex = MemoryFindIndex;
	FramebufferGetDimensions(pOsState, &gVkCtx.framebufferWidth, &gVkCtx.framebufferHeight);

#ifdef DEBUG
		darray_push(ppRequired_extensions, &VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
		YDEBUG("Required extensions:");
		uint32_t length = darray_length(ppRequired_extensions);
		for (uint32_t i = 0; i < length; ++i)
			YDEBUG(ppRequired_extensions[i]);

		// The list of validation layers required.
		YINFO("Validation layers enabled. Enumerating...");
		ppRequiredValidationLayerNames = darray_create(const char*);
		darray_push(ppRequiredValidationLayerNames, &"VK_LAYER_KHRONOS_validation");
		requiredValidationLayerCount = darray_length(ppRequiredValidationLayerNames);
		
		// Obtain list of available validation layers.
		uint32_t availableLayerCount = 0;
		VK_CHECK(vkEnumerateInstanceLayerProperties(&availableLayerCount, 0));

		VkLayerProperties* pAvailableLayers = darray_reserve(VkLayerProperties, availableLayerCount);
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
		YINFO("All required layers are present.");
#endif // DEBUG

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
		.enabledExtensionCount = darray_length(ppRequired_extensions),
		.ppEnabledExtensionNames = ppRequired_extensions
	};
	VK_CHECK(vkCreateInstance(&pCreateInfo, gVkCtx.pAllocator, &gVkCtx.instance));
	VK_CHECK(OS_CreateVkSurface(pOsState, &gVkCtx));

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

	VK_CHECK(VulkanCreateDevice(&gVkCtx, pGPUName));

	darray_destroy(ppRequiredValidationLayerNames);
	darray_destroy(ppRequired_extensions);

	int32_t width = gVkCtx.framebufferWidth;
	int32_t height = gVkCtx.framebufferHeight;
	VK_CHECK(vkSwapchainCreate(&gVkCtx, width, height, &gVkCtx.swapChain));
	VK_CHECK(vkCommandPoolCreate(&gVkCtx));
	VK_CHECK(vkCommandBufferCreate(&gVkCtx));

	ColorFloat color = {
		.r = 30.0f,
		.g = 30.0f,
		.b = 200.0f,
		.a = 1.0f,
	};
	RectFloat rect = {
		.x = 0.0f, .y = 0.0f,
		.w = gVkCtx.framebufferWidth,
		.h = gVkCtx.framebufferHeight,
	};
	f32 depth = 0; f32 stencil = 0;

	vkRenderPassCreate(&gVkCtx, &gVkCtx.mainRenderpass, color, rect, depth, stencil);
	gVkCtx.swapChain.pFramebuffers = DarrayReserve(VulkanFramebuffer, gVkCtx.swapChain.imageCount);
	FramebuffersRegenerate(&gVkCtx.swapChain, &gVkCtx.mainRenderpass);

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
        if (typeFilter & (1 << i) 
				&& (memoryProperties.memoryTypes[i].propertyFlags & propertyFlags) == propertyFlags)
		{
            return i;
        }
    }
    YWARN("Unable to find suitable memory type!");
    return -1;
}

VKAPI_ATTR VkBool32 VKAPI_CALL 
vkDebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageTypes,
		const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData, void *pUserData)
{
	(void) messageTypes;
	(void) pUserData;
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
