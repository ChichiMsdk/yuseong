#ifndef VULKAN_TYPES_UTILS_H
#define VULKAN_TYPES_UTILS_H

#include "core/yvec4.h"
#include "core/myassert.h"
#include "renderer/renderer_defines.h"

#include <vulkan/vulkan.h>
#include <vulkan/vk_enum_string_helper.h>

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

typedef struct VulkanImage
{
	VkImage			handle;
	VkImageView		view;
	VkDeviceMemory	memory;
	uint32_t		width;
	uint32_t		height;
} VulkanImage;

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

#endif // VULKAN_TYPES_UTILS_H
