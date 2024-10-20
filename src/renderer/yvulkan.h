#ifndef YVULKAN_H
#define YVULKAN_H

#	include <vulkan/vulkan.h>
#	include "mydefines.h"
# 	include "core/assert.h"
#	include <vulkan/vk_enum_string_helper.h>

typedef struct OS_State OS_State;

#define VK_ASSERT(expr) KASSERT_MSG(expr == VK_SUCCESS, string_VkResult(expr));

#define VK_CHECK(expr) \
	do { \
		VkResult errcode = (expr); \
		if (errcode != VK_SUCCESS) \
		{ \
			YERROR("%s", string_VkResult(errcode)); \
			return errcode; \
		} \
	} while (0)

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
	u8 bMaxFrameInFlight;
	VkSwapchainKHR handle;
	uint32_t imageCount;
	VkImage* pImages;
	VkImageView* pViews;

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

typedef struct VkContext
{
	VkInstance instance;
	VkAllocationCallbacks *pAllocator;
	VkSurfaceKHR surface;
	/* vulkan_device device; */
	VulkanDevice device;
	VulkanCommandBuffer *pGfxCommands;
	uint32_t framebufferWidth;
	uint32_t framebufferHeight;

	VkSwapchain swapChain;
	uint32_t currentFrame;
	int32_t (*MemoryFindIndex)(uint32_t typeFilter, uint32_t propertyFlags);
#ifdef DEBUG
	VkDebugUtilsMessengerEXT debugMessenger;
#endif
}VkContext;

YND VkResult RendererInit(
		OS_State*							pState);

void	RendererShutdown(
		OS_State*							pState);

YND VkResult vkCommandPoolCreate(
		VkContext*							pContext);
#endif // YVULKAN_H
