#define BACKEND_VULKAN

#ifdef BACKEND_VULKAN

#ifndef YVULKAN_H
#define YVULKAN_H

#include "renderer/vulkan/vulkan_types_utils.h"
#include "renderer/vulkan/macro_utils.h"

#ifdef PLATFORM_LINUX
#	ifdef YGLFW3
#	endif
#endif

typedef struct OsState OsState;

typedef struct VulkanBuffer
{
	VkBuffer			handle;
	VkDeviceMemory		memory;
	VkDeviceSize		size;
	VkDeviceSize		offset;
	VkMemoryMapFlags	flags;
} VulkanBuffer;


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
	VulkanFence		fence;
	VulkanCommandBuffer	commandBuffer;
	VkCommandPool		commandPool;
}VulkanImmediateSubmit;

typedef struct VulkanDevice
{
	VkDevice							handle;
	VkPhysicalDevice					physicalDevice;

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

typedef struct DrawImage
{
	VulkanImage	image;
	VkImage		handle;
	VkExtent3D	extent;
	VkFormat	format;
} DrawImage;

typedef struct PoolSizeRatio
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

typedef struct GraphicsPipeline
{
	VkPipelineShaderStageCreateInfo*		pShaderStages;
	VkPipelineInputAssemblyStateCreateInfo	inputAssembly;
	VkPipelineRasterizationStateCreateInfo	rasterizer;
	VkPipelineColorBlendAttachmentState		colorBlendAttachment;
	VkPipelineMultisampleStateCreateInfo	multisampling;
	VkPipelineLayout						pipelineLayout;
	VkPipelineDepthStencilStateCreateInfo	depthStencil;
	VkPipelineRenderingCreateInfo			renderingCreateInfo;
	VkFormat								colorAttachmentFormat;
} GraphicsPipeline;

typedef struct GpuMeshBuffers
{
	VulkanBuffer	indexBuffer;
	VulkanBuffer	vertexBuffer;
	VkDeviceAddress	vertexBufferAddress;
} GpuMeshBuffers;

typedef struct GpuDrawPushConstants
{
	mat4			worldMatrix;
	VkDeviceAddress	vertexBufferAddress;
} GpuDrawPushConstants;

/* TODO: Uniform struct for vertex, frag, compute and mesh */
typedef struct GenericPipeline
{
	char*				pFragmentShaderFilePath;
	char*				pVertexShaderFilePath;
	VkPipelineLayout	pipelineLayout;
	VkPipeline			pipeline;
	GpuMeshBuffers		rectangle;
	GraphicsPipeline	graphicsPipeline;
} GenericPipeline;

typedef struct ComputeShaderFx
{
	ComputePushConstant pushConstant;
	const char*			pFilePath;
	VkPipeline			pipeline;
	VkPipelineLayout	pipelineLayout;
	GraphicsPipeline	graphicsPipeline;
} ComputeShaderFx;

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
	DrawImage						depthImage;

	VkSwapchain						swapchain;
	uint32_t						currentFrame;
	uint32_t						imageIndex;
	b8								bRecreatingSwapchain;

	YND VkResult					(*MemoryFindIndex)(
										VkPhysicalDevice	physicalDevice,
										uint32_t			typeFilter,
										uint32_t 			propertyFlags,
										int32_t* 			pOutIndex);

	uint64_t						nbFrames;

	VkDescriptorSetLayoutBinding*	pBindings;

	DescriptorAllocator				descriptorAllocator;
	VulkanDescriptorPool			descriptorPool;

	VkPipeline						gradientComputePipeline;
	VkPipelineLayout				gradientComputePipelineLayout;

	VkDescriptorSet					drawImageDescriptorSet;
	VkDescriptorSetLayout			drawImageDescriptorSetLayout;

	ComputeShaderFx					*pComputeShaders;

	GenericPipeline					triPipeline;
	GenericPipeline					meshPipeline;
	GpuMeshBuffers					gpuMeshBuffers;

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
