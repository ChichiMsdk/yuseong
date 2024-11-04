#include "vulkan_framebuffer.h"
#include "core/ymemory.h"

#include "core/logger.h"

void
vkFramebufferCreate(VkContext* pCtx, VulkanRenderPass* pRenderpass, uint32_t width, uint32_t height, 
		uint32_t attachmentCount, VkImageView* pAttachments, VulkanFramebuffer* pOutFramebuffer) 
{
    // Take a copy of the attachments, renderpass and attachment count
    pOutFramebuffer->pAttachments = yAlloc(sizeof(VkImageView) * attachmentCount, MEMORY_TAG_RENDERER);
    for (uint32_t i = 0; i < attachmentCount; ++i) 
	{
        pOutFramebuffer->pAttachments[i] = pAttachments[i];
    }
    pOutFramebuffer->pRenderpass = pRenderpass;
    pOutFramebuffer->attachmentCount = attachmentCount;

	VkFramebufferCreateInfo framebufferCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
		.renderPass = pRenderpass->handle,
		.attachmentCount = attachmentCount,
		.pAttachments = pOutFramebuffer->pAttachments,
		.width = width,
		.height = height,
		.layers = 1,
	};

    VK_ASSERT(vkCreateFramebuffer(
				pCtx->device.logicalDev,
				&framebufferCreateInfo,
				pCtx->pAllocator,
				&pOutFramebuffer->handle));
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
    pFramebuffer->handle = 0;
    pFramebuffer->attachmentCount = 0;
    pFramebuffer->pRenderpass = 0;
}
