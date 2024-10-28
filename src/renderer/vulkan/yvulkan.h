#ifndef YVULKAN_H
#define YVULKAN_H

#	include <vulkan/vulkan.h>
#	include <vulkan/vk_enum_string_helper.h>

#	include "renderer/renderer_defines.h"
# 	include "core/assert.h"

#ifdef PLATFORM_LINUX
#	ifdef YGLFW3
#	endif
#endif

typedef struct OsState OsState;

#define VK_ASSERT(expr) KASSERT_MSG(expr == VK_SUCCESS, string_VkResult(expr));

#define VK_RESULT(expr) \
	do { \
		VkResult errcode = (expr); \
		if (errcode != VK_SUCCESS) \
		{ \
			return errcode; \
		} \
	} while (0);

#define VK_CHECK(expr) \
	do { \
		VkResult errcode = (expr); \
		if (errcode != VK_SUCCESS) \
		{ \
			YERROR("%s", string_VkResult(errcode)); \
			return errcode; \
		} \
	} while (0);


typedef struct VkPhysicalDeviceRequirements
{
	b8 bGraphics;
	b8 bPresent;
	b8 bCompute;
	b8 bTransfer;
	const char **ppDeviceExtensionNames;
	b8 bSamplerAnisotropy;
	b8 bDiscreteGpu;
}VkPhysicalDeviceRequirements;

typedef struct VkPhysicalDeviceQueueFamilyInfo
{
    uint32_t graphicsFamilyIndex;
    uint32_t presentFamilyIndex;
    uint32_t computeFamilyIndex;
    uint32_t transferFamilyIndex;
}VkPhysicalDeviceQueueFamilyInfo;

typedef struct VkSwapchainSupportInfo
{
    VkSurfaceCapabilitiesKHR capabilities;
    uint32_t formatCount;
    VkSurfaceFormatKHR* pFormats;
    uint32_t presentModeCount;
    VkPresentModeKHR* pPresentModes;
}VkSwapchainSupportInfo;

typedef struct VulkanFence 
{
    VkFence handle;
    b8 bSignaled;
}VulkanFence;

typedef struct VulkanDevice
{
	VkDevice logicalDev;
	VkPhysicalDevice physicalDev;

	VkSwapchainSupportInfo swapchainSupport;
	VkQueue graphicsQueue;
	VkQueue presentQueue;
	VkQueue transferQueue;

    int32_t graphicsQueueIndex;
    int32_t presentQueueIndex;
    int32_t transferQueueIndex;

	VkPhysicalDeviceProperties properties;
    VkPhysicalDeviceFeatures features;
    VkPhysicalDeviceMemoryProperties memory;

	VkCommandPool graphicsCommandPool;
	VkFormat depthFormat;
}VulkanDevice;

typedef struct VulkanImage
{
	VkImage handle;
	VkDeviceMemory memory;
	VkImageView view;
	uint32_t width;
	uint32_t height;
}VulkanImage;


typedef enum VulkanRenderPassState 
{
	READY,
	RECORDING,
	IN_RENDER_PASS,
	RECORDING_ENDED,
	SUBMITTED,
	NOT_ALLOCATED
}VkRenderPassState;

typedef struct VulkanRenderPass 
{
	VkRenderPass handle;
	f32 x, y, w, h;
	f32 r, g, b, a;

	f32 depth;
	uint32_t stencil;

	VkRenderPassState state;
}VulkanRenderPass;

typedef struct VulkanFramebuffer
{
	VkFramebuffer handle;
	uint32_t attachmentCount;
	VkImageView *pAttachments;
	VulkanRenderPass *pRenderpass;
}VulkanFramebuffer;

typedef struct VkSwapchain
{
	VkSurfaceFormatKHR imageFormat;
	u8 maxFrameInFlight;
	VkSwapchainKHR handle;
	uint32_t imageCount;
	VkImage* pImages;
	VkImageView* pViews;
	VkExtent3D extent;

	// framebuffers used for on-screen rendering.
	VulkanFramebuffer* pFramebuffers;

	// TODO: This is an ambiguous name -> try having "VulkanImage" in the name
	VulkanImage depthAttachment;
}VkSwapchain;

typedef enum VulkanCommandBufferState 
{
	COMMAND_BUFFER_STATE_READY,
	COMMAND_BUFFER_STATE_RECORDING,
	COMMAND_BUFFER_STATE_IN_RENDER_PASS,
	COMMAND_BUFFER_STATE_RECORDING_ENDED,
	COMMAND_BUFFER_STATE_SUBMITTED,
	COMMAND_BUFFER_STATE_NOT_ALLOCATED
} VkCommandBufferState;

typedef struct VulkanCommandBuffer 
{
	VkCommandBuffer handle;
	VkCommandBufferState state;
} VulkanCommandBuffer;

typedef struct DrawImage
{
	VulkanImage image;
	VkImage handle;
	VkExtent3D extent;
	VkFormat format;
}DrawImage;

typedef struct VkContext
{
	VkInstance instance;
	VkAllocationCallbacks *pAllocator;
	VkSurfaceKHR surface;
	VulkanDevice device;

	/* NOTE: Darray */
	VulkanCommandBuffer *pGfxCommands;
	uint32_t framebufferWidth;
	uint32_t framebufferHeight;
	/*
	 * NOTE: Current framebuffer size gen. If it does not match framebufferSizeLastGeneration
	 * new one should be generated.
	 */
    uint64_t framebufferSizeGeneration;
	/*
	 * NOTE: Framebuffer gen when it was last created.
	 * Set to framebufferSizeGeneration when updated.
	 */
    uint64_t framebufferSizeLastGeneration;
	VulkanRenderPass mainRenderpass;

	// NOTE: Darrays
	VkSemaphore *pSemaphoresAvailableImage;
	VkSemaphore *pSemaphoresQueueComplete;
	VulkanFence *pFencesInFlight;

    /* NOTE: Holds pointers to fences which exist and are owned elsewhere. */
	VulkanFence **ppImagesInFlight;

	DrawImage drawImage;

	VkSwapchain swapchain;
	uint32_t currentFrame;
	uint32_t imageIndex;
	b8 bRecreatingSwapchain;

	int32_t (*MemoryFindIndex)(uint32_t typeFilter, uint32_t propertyFlags);
	uint64_t nbFrames;

#ifdef DEBUG
	VkDebugUtilsMessengerEXT debugMessenger;
#endif

}VkContext;

YND VkResult vkInit(
		OsState*							pState);

void vkShutdown(void);

YND VkResult vkCommandPoolCreate(
		VkContext*							pContext);

void vkClearBackground(
		VulkanCommandBuffer*				pCmd,
		VkImage								pImage);

YND VkResult vkQueueSubmitAndSwapchainPresent(
		VulkanCommandBuffer*				pCmd);

YND VkResult vkDrawImpl(void);
#endif // YVULKAN_H
