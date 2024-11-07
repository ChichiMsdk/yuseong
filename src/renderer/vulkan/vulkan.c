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
#include "core/darray_debug.h"
#include "core/logger.h"
#include "core/myassert.h"
#include "core/ymemory.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vulkan/vk_enum_string_helper.h>

#include "TracyC.h"

YMB static VkContext gVkCtx;
static GlobalContext gContext;

static inline void SyncInit(
		VkContext*										pCtx,
		VulkanDevice									device);

YND static int32_t MemoryFindIndex(
		VkPhysicalDevice								physicalDevice,
		uint32_t										typeFilter,
		uint32_t										propertyFlags);

YMB static inline VkResult DebugRequiredExtensionValidationLayers(
		const char***									pppRequiredExtensions,
		const char***									pppRequiredValidationLayerNames,
		uint32_t*										pRequiredValidationLayerCount);

YMB static inline VkResult DebugCallbackSetup(
		VkContext*										pCtx);

/* TODO: Make it a function that looks config file for the folder ?*/
extern int32_t gShaderFileIndex;
extern const char *gppShaderFilePath[];

YND VkResult
vkInit(OsState *pOsState, void** ppOutCtx)
{
	char*			pGPUName						= "NVIDIA GeForce RTX 3080";
	const char**	ppRequiredValidationLayerNames	= NULL;
	uint32_t		requiredValidationLayerCount	= 0;
	const char**	ppRequiredExtensions			= NULL;
	uint32_t		requiredExtensionCount			= 0;
	VkContext*		pCurrentCtx						= NULL;

	/* 
	 * NOTE: This is just for fun, multiple context is complex
	 * MAX_CONTEXT = 100; VkContext = 2168 bytes.
	 * So gContext.ppCtx = at least 216,8 kilobytes.
	 */  
	gContext.ppCtx[gContext.contextCount] = yAlloc(sizeof(VkContext), MEMORY_TAG_RENDERER_CONTEXT);
	gContext.currentContext = gContext.contextCount;
	gContext.contextCount++;
	pCurrentCtx = gContext.ppCtx[gContext.currentContext];

	ppRequiredExtensions = DarrayCreate(const char *);
	DarrayPush(ppRequiredExtensions, &VK_KHR_SURFACE_EXTENSION_NAME);
	DarrayPush(ppRequiredExtensions, &VK_KHR_SURFACE_OS);

	pCurrentCtx->MemoryFindIndex = MemoryFindIndex;
	OsFramebufferGetDimensions(pOsState, &pCurrentCtx->framebufferWidth, &pCurrentCtx->framebufferHeight);

#ifdef DEBUG
	DebugRequiredExtensionValidationLayers(
			&ppRequiredExtensions,
			&ppRequiredValidationLayerNames,
			&requiredValidationLayerCount);
#endif // DEBUG

	requiredExtensionCount = DarrayLength(ppRequiredExtensions);
	VkApplicationInfo pAppInfo = {
		.sType						= VK_STRUCTURE_TYPE_APPLICATION_INFO,
		.pApplicationName			= "yuseong",
		.pEngineName				= "yuseongEngine",
		.applicationVersion			= VK_MAKE_VERSION(1, 0, 0),
		.engineVersion				= VK_MAKE_VERSION(1, 0, 0),
		.apiVersion					= VK_API_VERSION_1_3
	};
	VkInstanceCreateInfo pCreateInfo = {
		.sType						= VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
		.pNext						= VK_NULL_HANDLE,
		.flags						= 0,
		.pApplicationInfo			= &pAppInfo,
		.enabledLayerCount			= requiredValidationLayerCount,
		.ppEnabledLayerNames		= ppRequiredValidationLayerNames,
		.ppEnabledExtensionNames	= ppRequiredExtensions,
		.enabledExtensionCount		= requiredExtensionCount,
		/* .enabledExtensionCount		= darray_length(ppRequired_extensions), */
	};

	/* NOTE: Create vkInstance */
	VK_ASSERT(vkCreateInstance(&pCreateInfo, pCurrentCtx->pAllocator, &pCurrentCtx->instance));

	/* NOTE: Create vkSurface  */
	VK_CHECK(OsCreateVkSurface(pOsState, pCurrentCtx));
	YDEBUG("Vulkan surface created");

#ifdef DEBUG
	DebugCallbackSetup(pCurrentCtx);
#endif // DEBUG

	/* NOTE: Creating the device */
	VK_CHECK(VulkanCreateDevice(pCurrentCtx, &pCurrentCtx->device.logicalDev, pGPUName));

	/* NOTE: Create Swapchain */
	int32_t width = pCurrentCtx->framebufferWidth;
	int32_t height = pCurrentCtx->framebufferHeight;
	VK_CHECK(vkSwapchainCreate(pCurrentCtx, width, height, &pCurrentCtx->swapchain));

	/* NOTE: Create commandPool */
	VK_CHECK(vkCommandPoolCreate(pCurrentCtx->device, pCurrentCtx->pAllocator, &pCurrentCtx->device.graphicsCommandPool));

	/* NOTE: Create commandBuffer */
	VK_CHECK(vkCommandBufferCreate(pCurrentCtx));

	/* NOTE: Create RenderPass */
	YMB RgbaFloat	color	= { .r = 30.0f, .g = 30.0f, .b = 200.0f, .a = 1.0f, };
	YMB RectFloat	rect	= { .x = 0.0f, .y = 0.0f, .w = width, .h = height, };
	YMB f32			depth	= 1.0f;
	YMB f32 		stencil	= 0.0f;
	VK_CHECK(vkRenderPassCreate(pCurrentCtx, &pCurrentCtx->mainRenderpass, color, rect, depth, stencil));

	pCurrentCtx->swapchain.pFramebuffers = DarrayReserve(VulkanFramebuffer, pCurrentCtx->swapchain.imageCount);
	vkFramebuffersRegenerate(pCurrentCtx, &pCurrentCtx->swapchain, &pCurrentCtx->mainRenderpass);

	pCurrentCtx->pSemaphoresAvailableImage	= DarrayReserve(VkSemaphore, pCurrentCtx->swapchain.maxFrameInFlight);
	pCurrentCtx->pSemaphoresQueueComplete	= DarrayReserve(VkSemaphore, pCurrentCtx->swapchain.maxFrameInFlight);
	pCurrentCtx->pFencesInFlight			= DarrayReserve(VulkanFence, pCurrentCtx->swapchain.maxFrameInFlight);

	/* NOTE: Init semaphore and fences */
	SyncInit(pCurrentCtx, pCurrentCtx->device);

	/*
	 * NOTE: In flight fences should not yet exist at this point, so clear the list. These are stored in pointers
	 * because the initial state should be 0, and will be 0 when not in use. Acutal fences are not owned
	 * by this list.
	 */
    pCurrentCtx->ppImagesInFlight = DarrayReserve(VulkanFence, pCurrentCtx->swapchain.imageCount);
    for (uint32_t i = 0; i < pCurrentCtx->swapchain.imageCount; ++i) 
	{
        pCurrentCtx->ppImagesInFlight[i] = 0;
    }

	/* NOTE: DescriptorSets allocation */
	VK_CHECK(vkDescriptorsInit(pCurrentCtx, pCurrentCtx->device.logicalDev));
	/* VK_CHECK(vkPipelineInit(pCurrentCtx, pCurrentCtx->device.logicalDev, gpShaderFilePath[gShaderFileIndex])); */
	/* NOTE: ComputePipeline for shaders */
	VK_CHECK(vkComputePipelineInit(pCurrentCtx, pCurrentCtx->device.logicalDev, gppShaderFilePath));

	/* NOTE: Cleanup */
	DarrayDestroy(ppRequiredExtensions);
	DarrayDestroy(ppRequiredValidationLayerNames);

	(*ppOutCtx) = pCurrentCtx;

	YINFO("Vulkan renderer initialized.");
	return VK_SUCCESS;
}

void
vkShutdown(void *pContext)
{
	VkContext *pCtx						= (VkContext*)pContext;
	VkDevice device						= pCtx->device.logicalDev;
	VulkanDevice myDevice				= pCtx->device;
	VkAllocationCallbacks* pAllocator	= pCtx->pAllocator;

#ifdef DEBUG
		PFN_vkDestroyDebugUtilsMessengerEXT pfnDestroyDebug = (PFN_vkDestroyDebugUtilsMessengerEXT)
			vkGetInstanceProcAddr(pCtx->instance, "vkDestroyDebugUtilsMessengerEXT");

		YASSERT_MSG(pfnDestroyDebug, "Failed to create debug destroy messenger!");
		pfnDestroyDebug(pCtx->instance, pCtx->debugMessenger, pAllocator);
#endif // DEBUG

    /*
	 * VK_ASSERT(vkDeviceWaitIdle(device));
	 * vkDestroyPipeline(device, pCtx->gradientComputePipeline, pAllocator);
	 * vkDestroyPipelineLayout(device, pCtx->gradientComputePipelineLayout, pAllocator);
     */
	vkPipelinesCleanUp(pCtx, device);
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
		yFree(
				pCtx->device.swapchainSupport.pFormats,
				pCtx->device.swapchainSupport.formatCount,
				MEMORY_TAG_RENDERER);
	}
	if (pCtx->device.swapchainSupport.pPresentModes)
	{
		yFree(
				pCtx->device.swapchainSupport.pPresentModes,
				pCtx->device.swapchainSupport.presentModeCount,
				MEMORY_TAG_RENDERER);
	}
	vkDestroyDevice(pCtx->device.logicalDev, pAllocator);
	vkDestroyInstance(pCtx->instance, pAllocator);

	yFree(pCtx, 1, MEMORY_TAG_RENDERER_CONTEXT);
}

static inline void
SyncInit(VkContext* pCtx, VulkanDevice device)
{
	b8 bSignaled = TRUE;
    /*
	 * NOTE: one fence to control when the gpu has finished rendering the frame,
	 * and 2 semaphores to synchronize rendering with swapchain
	 * we want the fence to start signalled so we can wait on it on the first frame
	 * This will prevent the application from waiting indefinitely for the first frame to render since it
	 * cannot be rendered until a frame is "rendered" before it.
     */
	for (uint8_t i = 0; i < pCtx->swapchain.maxFrameInFlight; i++)
	{
		VkSemaphoreCreateInfo semaphoreCreateInfo = {
			.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO
		};
		vkCreateSemaphore(device.logicalDev, &semaphoreCreateInfo, pCtx->pAllocator, &pCtx->pSemaphoresAvailableImage[i]);
		vkCreateSemaphore(device.logicalDev, &semaphoreCreateInfo, pCtx->pAllocator, &pCtx->pSemaphoresQueueComplete[i]);

		/* NOTE: Assuming that the app CANNOT run without fences therefore we abort() in case of failure */
		VK_ASSERT(vkFenceCreate(device.logicalDev, bSignaled, pCtx->pAllocator, &pCtx->pFencesInFlight[i]));
	}
	/* NOTE: Fence for the immediate submit commands */
	/* VK_ASSERT(vkFenceCreate(device.logicalDev, bSignaled, pCtx->pAllocator, &device.immediateSubmit.fence)) */
}

YND int32_t
MemoryFindIndex(VkPhysicalDevice physicalDevice, uint32_t typeFilter, uint32_t propertyFlags)
{
    VkPhysicalDeviceMemoryProperties memoryProperties;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties);

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

VKAPI_ATTR VkBool32 VKAPI_CALL vkDebugCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT			messageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT					messageTypes,
		const VkDebugUtilsMessengerCallbackDataEXT*		pCallbackData,
		void*											pUserData);

#ifdef DEBUG
static inline VkResult
DebugCallbackSetup(VkContext* pCtx)
{
	YDEBUG("Creating Vulkan debugger...");

	uint32_t logSeverity = 
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT 
		| VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT;
	// | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT;
	//VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT;

	VkDebugUtilsMessageTypeFlagsEXT messageType = 
		VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
		| VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT 
		| VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT 
		| VK_DEBUG_UTILS_MESSAGE_TYPE_DEVICE_ADDRESS_BINDING_BIT_EXT;

	VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo = {
		.sType				= VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
		.messageSeverity	= logSeverity,
		.pfnUserCallback	= vkDebugCallback,
		.messageType		= messageType,
	};

	PFN_vkCreateDebugUtilsMessengerEXT pfnDebug =
		(PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(pCtx->instance, "vkCreateDebugUtilsMessengerEXT");
	YASSERT_MSG(pfnDebug, "Failed to create debug messenger!");
	VK_CHECK(pfnDebug(pCtx->instance, &debugCreateInfo, pCtx->pAllocator, &pCtx->debugMessenger));
	YDEBUG("Vulkan debugger created.");

	return VK_SUCCESS;
}

static inline VkResult
DebugRequiredExtensionValidationLayers(
		const char***						pppRequiredExtensions,
		const char***						pppRequiredValidationLayerNames,
		uint32_t*							pRequiredValidationLayerCount)
{
	DarrayPush(*pppRequiredExtensions, &VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

	YDEBUG("Required extensions:");
	uint32_t length = DarrayLength(*pppRequiredExtensions);
	for (uint32_t i = 0; i < length; ++i)
		YDEBUG((*pppRequiredExtensions)[i]);

	YINFO("Validation layers enabled. Enumerating...");

	/* NOTE: The list of validation layers required. */
	*pppRequiredValidationLayerNames = DarrayCreate(const char*);
	DarrayPush(*pppRequiredValidationLayerNames, &"VK_LAYER_KHRONOS_validation");
	*pRequiredValidationLayerCount = DarrayLength(*pppRequiredValidationLayerNames);

	/* NOTE: Obtain list of available validation layers. */
	uint32_t availableLayerCount = 0;
	VK_CHECK(vkEnumerateInstanceLayerProperties(&availableLayerCount, 0));

	VkLayerProperties* pAvailableLayers = DarrayReserve(VkLayerProperties, availableLayerCount);
	VK_CHECK(vkEnumerateInstanceLayerProperties(&availableLayerCount, pAvailableLayers));

	/* NOTE:Verify all required layers are available. */
	for (uint32_t i = 0; i < *pRequiredValidationLayerCount; ++i)
	{
		YINFO("Searching for layer: %s...", (*pppRequiredValidationLayerNames)[i]);
		b8 bFound = FALSE;
		for (uint32_t j = 0; j < availableLayerCount; ++j)
		{
			if (!strcmp((*pppRequiredValidationLayerNames)[i], pAvailableLayers[j].layerName))
			{ 
				bFound = TRUE; 
				YINFO("Found."); 
				break; 
			}
		}
		if (!bFound) 
		{ 
			YFATAL("Required validation layer is missing: %s", (*pppRequiredValidationLayerNames)[i]); 
			exit(1); 
		}
	}
	DarrayDestroy(pAvailableLayers);
	YINFO("All required validation layers are present.");
	return VK_SUCCESS;
}
#endif

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
