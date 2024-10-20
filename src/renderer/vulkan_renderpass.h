#ifndef VULKAN_RENDERPASS_H
#define VULKAN_RENDERPASS_H

#include "yvulkan.h"

struct fColor
{
	f32 r; f32 g; f32 b; f32 a;
}fColor;

struct fRect
{
	f32 x; f32 y;
	f32 w; f32 h;
}fRect;

void vkRenderPassCreate(
		VkContext*							pCtx, 
		VulkanRenderPass*					pOutRenderPass,
		struct fColor						color,
		struct fRect						rect,
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
