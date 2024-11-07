#define BACKEND_VULKAN

#ifdef BACKEND_VULKAN

#ifndef YVULKAN_H
#define YVULKAN_H

#include <vulkan/vulkan.h>
#include <vulkan/vk_enum_string_helper.h>

#include "core/vec4.h"
#include "core/myassert.h"
#include "renderer/renderer_defines.h"
#include "renderer/vulkan/macro_utils.h"

#ifdef PLATFORM_LINUX
#	ifdef YGLFW3
#	endif
#endif

typedef struct OsState OsState;

typedef struct ComputeShaderFx
{
	const char*			pFilePath;
	VkPipeline			pipeline;
	VkPipelineLayout	pipelineLayout;
	ComputePushConstant pushConstant;
} ComputeShaderFx;

typedef struct VkPhysicalDeviceRequirements
{
	const char**	ppDeviceExtensionNames;
	b8				bGraphics;
	b8				bPresent;
	b8				bCompute;
	b8				bTransfer;
	b8				bSamplerAnisotropy;
	b8				bDiscreteGpu;
} VkPhysicalDeviceRequirements;

typedef struct VkPhysicalDeviceQueueFamilyInfo
{
	uint32_t	graphicsFamilyIndex;
	uint32_t 	presentFamilyIndex;
	uint32_t 	computeFamilyIndex;
	uint32_t 	transferFamilyIndex;
} VkPhysicalDeviceQueueFamilyInfo;

typedef struct VkSwapchainSupportInfo
{
	VkSurfaceCapabilitiesKHR	capabilities;
	uint32_t					formatCount;
	VkSurfaceFormatKHR*			pFormats;
	uint32_t					presentModeCount;
	VkPresentModeKHR*			pPresentModes;
} VkSwapchainSupportInfo;

typedef struct VulkanFence 
{
	VkFence	handle;
	b8		bSignaled;
} VulkanFence;

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
	VkCommandBuffer			handle;
	VkCommandBufferState	state;
} VulkanCommandBuffer;

typedef struct VulkanImmediateSubmit
{
	VulkanFence			fence;
	VulkanCommandBuffer	commandBuffer;
	VkCommandPool		commandPool;
}VulkanImmediateSubmit;


typedef struct VulkanDevice
{
	VkDevice							logicalDev;
	VkPhysicalDevice					physicalDev;

	VkSwapchainSupportInfo				swapchainSupport;
	VkQueue								graphicsQueue;
	VkQueue 							presentQueue;
	VkQueue 							transferQueue;

	int32_t								graphicsQueueIndex;
	int32_t								presentQueueIndex;
	int32_t								transferQueueIndex;

	VkPhysicalDeviceProperties			properties;
	VkPhysicalDeviceFeatures			features;
	VkPhysicalDeviceMemoryProperties	memory;

	VkCommandPool						graphicsCommandPool;
	VkFormat							depthFormat;

	VulkanImmediateSubmit				immediateSubmit;
} VulkanDevice;

typedef struct VulkanImage
{
	VkImage			handle;
	VkImageView		view;
	VkDeviceMemory	memory;
	uint32_t		width;
	uint32_t		height;
} VulkanImage;

typedef enum VulkanRenderPassState 
{
	READY,
	RECORDING,
	IN_RENDER_PASS,
	RECORDING_ENDED,
	SUBMITTED,
	NOT_ALLOCATED
} VkRenderPassState;

typedef struct VulkanRenderPass 
{
	VkRenderPass		handle;
	f32					x, y, w, h;
	f32					r, g, b, a;
	f32					depth;
	uint32_t			stencil;
	VkRenderPassState	state;
} VulkanRenderPass;

typedef struct VulkanFramebuffer
{
	VkFramebuffer		handle;
	uint32_t			attachmentCount;
	VkImageView*		pAttachments;
	VulkanRenderPass*	pRenderpass;
} VulkanFramebuffer;

typedef struct VkSwapchain
{
	VkSwapchainKHR		handle;
	uint32_t			imageCount;
	VkSurfaceFormatKHR	imageFormat;
	u8					maxFrameInFlight;
	VkImage*			pImages;
	VkImageView*		pViews;
	VkExtent3D			extent;

	// NOTE: framebuffers used for on-screen rendering.
	VulkanFramebuffer*	pFramebuffers;

	// TODO: This is an ambiguous name -> try having "VulkanImage" in the name
	VulkanImage			depthAttachment;
} VkSwapchain;


typedef struct DrawImage
{
	VulkanImage	image;
	VkImage		handle;
	VkExtent3D	extent;
	VkFormat	format;
} DrawImage;

typedef struct PoolSizeRation
{
	VkDescriptorType	type;
	float				ratio;
} PoolSizeRatio;

typedef struct VulkanDescriptorPool
{
	VkDescriptorPool	handle;
	PoolSizeRatio		poolSizeRatio;
} VulkanDescriptorPool;

typedef struct DescriptorAllocator
{
	VkDescriptorPool	pool;
	PoolSizeRatio		poolSizeRatio;
} DescriptorAllocator;

typedef struct VkContext
{
	VkInstance						instance;
	VkAllocationCallbacks*			pAllocator;
	VkSurfaceKHR					surface;
	VulkanDevice					device;

	// NOTE: Darray
	VulkanCommandBuffer*			pGfxCommands;
	uint32_t						framebufferWidth;
	uint32_t						framebufferHeight;

	/*
	 * NOTE: Current framebuffer size gen. If it does not match 
	 * framebufferSizeLastGeneration new one should be generated.
	 */
	uint64_t						framebufferSizeGeneration;

	/*
	 * NOTE: Framebuffer gen when it was last created.
	 * Set to framebufferSizeGeneration when updated.
	 */
	uint64_t						framebufferSizeLastGeneration;
	VulkanRenderPass				mainRenderpass;

	// NOTE: Darrays
	VkSemaphore*					pSemaphoresAvailableImage;
	VkSemaphore*					pSemaphoresQueueComplete;
	VulkanFence*					pFencesInFlight;

	// NOTE: Holds pointers to fences which exist and are owned elsewhere.
	VulkanFence**					ppImagesInFlight;

	DrawImage						drawImage;

	VkSwapchain						swapchain;
	uint32_t						currentFrame;
	uint32_t						imageIndex;
	b8								bRecreatingSwapchain;

	int32_t							(*MemoryFindIndex)(VkPhysicalDevice physicalDevice, uint32_t typeFilter, uint32_t propertyFlags);
	uint64_t						nbFrames;

	VkDescriptorSetLayoutBinding*	pBindings;

	DescriptorAllocator				descriptorAllocator;
	VulkanDescriptorPool			descriptorPool;

	VkPipeline						gradientComputePipeline;
	VkPipelineLayout				gradientComputePipelineLayout;

	VkDescriptorSet					drawImageDescriptorSet;
	VkDescriptorSetLayout			drawImageDescriptorSetLayout;

	ComputeShaderFx					*pComputeShaders;

#ifdef DEBUG
	VkDebugUtilsMessengerEXT		debugMessenger;
#endif

} VkContext;

#define MAX_CONTEXT 100

typedef struct GlobalContext
{
	uint32_t	contextCount;
	uint32_t	currentContext;
	VkContext*	ppCtx[MAX_CONTEXT];
} GlobalContext;

YND VkResult vkInit(
		OsState*							pState,
		void**								ppOutContext);

void vkShutdown(
		void*								pCtx);

#endif // YVULKAN_H

#endif // BACKEND_VULKAN
