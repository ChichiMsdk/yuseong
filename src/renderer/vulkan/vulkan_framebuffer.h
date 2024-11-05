#ifndef VULKAN_FRAMEBUFFER_H
#define VULKAN_FRAMEBUFFER_H

#include "yvulkan.h"

void vkFramebufferCreate(
		VkContext*							pCtx, 
		VulkanRenderPass*					pRenderpass,
		uint32_t							width,
		uint32_t							height,
		uint32_t							attachmentCount,
		VkImageView*						pAttachments,
		VulkanFramebuffer*					pOutFramebuffer);

void vkFramebufferDestroy(
		VkDevice							device,
		VkAllocationCallbacks*				pAllocator,
		VulkanFramebuffer*					pFramebuffer);

void vkFramebuffersRegenerate(
		VkContext*							pCtx,
		VkSwapchain*						pSwapchain,
		VulkanRenderPass*					pRenderpass);

#endif // VULKAN_FRAMEBUFFER_H
