#ifndef VULKAN_RENDERPASS_H
#define VULKAN_RENDERPASS_H

#include "yvulkan.h"

YND VkResult vkRenderPassCreate(
		VkContext*							pCtx, 
		VulkanRenderPass*					pOutRenderPass,
		RgbaFloat							color,
		RectFloat							rect,
		f32									depth,
		uint32_t							stencil);

void vkRenderPassDestroy(
		VkContext*							pCtx,
		VulkanRenderPass*					pRenderPass);

void vkRenderPassBegin(
		VulkanCommandBuffer*				pCommandBuffer,
		VulkanRenderPass*					pRenderPass,
		VkFramebuffer						frameBuffer);

void vkRenderPassEnd(
		VulkanCommandBuffer*				pCommandBuffer,
		VulkanRenderPass*					pRenderPass); 

#endif // VULKAN_RENDERPASS_H
