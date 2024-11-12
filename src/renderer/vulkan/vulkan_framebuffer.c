#include "vulkan_framebuffer.h"

#include "core/ymemory.h"
#include "core/logger.h"

void
vkFramebufferCreate(
		VkContext*							pCtx,
		VulkanRenderPass*					pRenderpass,
		uint32_t							width,
		uint32_t							height,
		uint32_t							attachmentCount,
		VkImageView*						pAttachments,
		VulkanFramebuffer*					pOutFramebuffer)

{
    /* NOTE: Take a copy of the attachments, renderpass and attachment count */
    pOutFramebuffer->pAttachments = yAlloc(sizeof(VkImageView) * attachmentCount, MEMORY_TAG_RENDERER);
    for (uint32_t i = 0; i < attachmentCount; ++i) 
	{
        pOutFramebuffer->pAttachments[i] = pAttachments[i];
    }
    pOutFramebuffer->pRenderpass = pRenderpass;
    pOutFramebuffer->attachmentCount = attachmentCount;

	VkFramebufferCreateInfo framebufferCreateInfo = {
		.sType				= VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
		.renderPass			= pRenderpass->handle,
		.attachmentCount	= attachmentCount,
		.pAttachments		= pOutFramebuffer->pAttachments,
		.width				= width,
		.height				= height,
		.layers				= 1,
	};

    VK_ASSERT(vkCreateFramebuffer(
				pCtx->device.handle,
				&framebufferCreateInfo,
				pCtx->pAllocator,
				&pOutFramebuffer->handle));
}

void
vkFramebuffersRegenerate(VkContext* pCtx, VkSwapchain* pSwapchain, VulkanRenderPass* pRenderpass)
{
	for (uint32_t i = 0; i < pSwapchain->imageCount; i++)
	{
		/*
		 * TODO: make this dynamic and based on the currently configured attachments 
		 * if using manual Renderpasses
		 */
		VkImageView pAttachments[] = {
			pSwapchain->pViews[i], 
			pSwapchain->depthAttachment.view 
		};
		uint32_t attachmentCount = 2;
		vkFramebufferCreate(
				pCtx,
				pRenderpass,
				pCtx->framebufferWidth,
				pCtx->framebufferHeight,
				attachmentCount,
				pAttachments,
				&pSwapchain->pFramebuffers[i]);
	}
}

void
vkFramebufferDestroy(VkDevice device, VkAllocationCallbacks* pAllocator, VulkanFramebuffer* pFramebuffer) 
{
    vkDestroyFramebuffer(device, pFramebuffer->handle, pAllocator);
    if (pFramebuffer->pAttachments) 
	{
        yFree(pFramebuffer->pAttachments, pFramebuffer->attachmentCount, MEMORY_TAG_RENDERER);
        pFramebuffer->pAttachments = 0;
    }
	/* NOTE: This shouldn't be useful */
    pFramebuffer->handle = 0;
    pFramebuffer->attachmentCount = 0;
    pFramebuffer->pRenderpass = 0;
}
