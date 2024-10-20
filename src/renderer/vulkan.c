#include "yvulkan.h"
#include "vulkan_swapchain.h"
#include "vulkan_command.h"
#include "core/darray.h"
#include "core/logger.h"
#include "core/assert.h"
#include "core/ymemory.h"
#include "os.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vulkan/vk_enum_string_helper.h>

static VkContext gVulkanContext;

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
	VkDevice device = gVulkanContext.device.logicalDev;
	VulkanDevice myDevice = gVulkanContext.device;
	VkContext *pCtx = &gVulkanContext;

#ifdef DEBUG
		PFN_vkDestroyDebugUtilsMessengerEXT func = (PFN_vkDestroyDebugUtilsMessengerEXT)
			vkGetInstanceProcAddr(gVulkanContext.instance, "vkDestroyDebugUtilsMessengerEXT");

		KASSERT_MSG(func, "Failed to create debug destroy messenger!");
		func(gVulkanContext.instance, gVulkanContext.debugMessenger, gVulkanContext.pAllocator);
#endif // DEBUG

	VK_ASSERT(vkCommandBufferFree(pCtx, pCtx->pGfxCommands, &myDevice.graphicsCommandPool, pCtx->swapChain.imageCount));
	VK_ASSERT(vkCommandPoolDestroy(pCtx, &myDevice.graphicsCommandPool, 1));

	vkDestroyVulkanImage(&gVulkanContext, &gVulkanContext.swapChain.depthAttachment);
	for (uint32_t i = 0; i < gVulkanContext.swapChain.imageCount; i++)
		vkDestroyImageView(device, gVulkanContext.swapChain.pViews[i], gVulkanContext.pAllocator);

	vkDestroySwapchainKHR(device, gVulkanContext.swapChain.handle, gVulkanContext.pAllocator);
	vkDestroySurfaceKHR(gVulkanContext.instance, gVulkanContext.surface, gVulkanContext.pAllocator);

	vkDestroyDevice(gVulkanContext.device.logicalDev, gVulkanContext.pAllocator);
	vkDestroyInstance(gVulkanContext.instance, gVulkanContext.pAllocator);
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
	gVulkanContext.MemoryFindIndex = MemoryFindIndex;
	FramebufferGetDimensions(pOsState, &gVulkanContext.framebufferWidth, &gVulkanContext.framebufferHeight);

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
	VK_CHECK(vkCreateInstance(&pCreateInfo, gVulkanContext.pAllocator, &gVulkanContext.instance));
	VK_CHECK(OS_CreateVkSurface(pOsState, &gVulkanContext));

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
			(PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(gVulkanContext.instance, "vkCreateDebugUtilsMessengerEXT");
		KASSERT_MSG(func, "Failed to create debug messenger!");
		VK_CHECK(func(gVulkanContext.instance, &debugCreateInfo, gVulkanContext.pAllocator, &gVulkanContext.debugMessenger));
		YDEBUG("Vulkan debugger created.");
#endif // DEBUG

	VK_CHECK(VulkanCreateDevice(&gVulkanContext, pGPUName));

	darray_destroy(ppRequiredValidationLayerNames);
	darray_destroy(ppRequired_extensions);

	int32_t width = gVulkanContext.framebufferWidth;
	int32_t height = gVulkanContext.framebufferHeight;
	VK_CHECK(vkSwapchainCreate(&gVulkanContext, width, height, &gVulkanContext.swapChain));
	VK_CHECK(vkCommandPoolCreate(&gVulkanContext));
	VK_CHECK(vkCommandBufferCreate(&gVulkanContext));

	return VK_SUCCESS;
}

YND int32_t
MemoryFindIndex(uint32_t typeFilter, uint32_t propertyFlags)
{
    VkPhysicalDeviceMemoryProperties memoryProperties;
    vkGetPhysicalDeviceMemoryProperties(gVulkanContext.device.physicalDev, &memoryProperties);

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
