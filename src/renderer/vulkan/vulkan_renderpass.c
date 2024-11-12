#include "vulkan_renderpass.h"

#include "core/logger.h"
#include "core/ymemory.h"

/*
 * NOTE: In vulkan 1.3 dynamic rendering allows not to deal with this boilerplate
 * needs more information and benchmarks to see if there are any differences
 */
YND VkResult 
vkRenderPassCreate(
		VkContext*							pCtx,
		VulkanRenderPass*					pOutRenderPass,
		RgbaFloat							color,
		RectFloat							rect,
		f32									depth,
		uint32_t							stencil)
{
    pOutRenderPass->x = rect.x; 
	pOutRenderPass->y = rect.y;
    pOutRenderPass->w = rect.w; 
	pOutRenderPass->h = rect.h;
    pOutRenderPass->r = color.r;
    pOutRenderPass->g = color.g;
    pOutRenderPass->b = color.b;
    pOutRenderPass->a = color.a;
    pOutRenderPass->depth = depth;
    pOutRenderPass->stencil = stencil;

    /* NOTE: Attachments */
	/* TODO: make this configurable. */
    uint32_t attachmentDescriptionCount = 2;
    VkAttachmentDescription pAttachmentDescriptions[attachmentDescriptionCount];

    /* NOTE: Color attachment */
    VkAttachmentDescription colorAttachment = {
		.format			= pCtx->swapchain.imageFormat.format,	// TODO: configurable
		.samples		= VK_SAMPLE_COUNT_1_BIT,
		.loadOp			= VK_ATTACHMENT_LOAD_OP_CLEAR,
		.storeOp		= VK_ATTACHMENT_STORE_OP_STORE,
		.stencilLoadOp	= VK_ATTACHMENT_LOAD_OP_DONT_CARE,
		.stencilStoreOp	= VK_ATTACHMENT_STORE_OP_DONT_CARE,
		.initialLayout	= VK_IMAGE_LAYOUT_UNDEFINED,			// NOTE: Do not expect any particular layout before render pass starts.
		.finalLayout	= VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,		// NOTE: Transitioned to after the render pass
		.flags			= 0,
	};
    pAttachmentDescriptions[0] = colorAttachment;

    /* NOTE: Depth attachment, if there is one */
    VkAttachmentDescription depthAttachment = {
		.format			= pCtx->device.depthFormat,
		.samples		= VK_SAMPLE_COUNT_1_BIT,
		.loadOp			= VK_ATTACHMENT_LOAD_OP_CLEAR,
		.storeOp		= VK_ATTACHMENT_STORE_OP_DONT_CARE,
		.stencilLoadOp	= VK_ATTACHMENT_LOAD_OP_DONT_CARE,
		.stencilStoreOp	= VK_ATTACHMENT_STORE_OP_DONT_CARE,
		.initialLayout	= VK_IMAGE_LAYOUT_UNDEFINED,
		.finalLayout	= VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
	};
    pAttachmentDescriptions[1] = depthAttachment;

    /* NOTE: Depth attachment reference */
    VkAttachmentReference depthAttachmentReference = {
		.attachment	= 1,
		.layout		= VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
	};

    /* NOTE: Color attachment reference */
    VkAttachmentReference colorAttachmentReference = {

		/* NOTE:Attachment description array index */
		.attachment	= 0,
		.layout		= VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
	};

    /* TODO: other attachment types (input, resolve, preserve) */

    /* NOTE: Main subpass */
    VkSubpassDescription subpassDescription = {
		.pipelineBindPoint			= VK_PIPELINE_BIND_POINT_GRAPHICS,
		.colorAttachmentCount		= 1,
		.pColorAttachments			= &colorAttachmentReference,

		/* NOTE: Depth stencil data. */
		.pDepthStencilAttachment	= &depthAttachmentReference,

		/* NOTE: Input from a shader */
		.inputAttachmentCount		= 0,
		.pInputAttachments			= 0,

		/* NOTE: Attachments used for multisampling colour attachments */
		.pResolveAttachments		= 0,

		/* NOTE: Attachments not used in this subpass, but must be preserved for the next. */
		.preserveAttachmentCount	= 0,
		.pPreserveAttachments		= 0,
	};

    /* NOTE: Render pass dependencies. */
	/* TODO: make this configurable. */
    VkSubpassDependency subpassDependency = {
		.srcSubpass			= VK_SUBPASS_EXTERNAL,
		.dstSubpass			= 0,
		.srcStageMask		= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		.srcAccessMask		= 0,
		.dstStageMask		= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		.dstAccessMask		= VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
		.dependencyFlags	= 0,
	};
    /* NOTE: Render pass create. */
	VkRenderPassCreateInfo renderPassCreateInfo = {
		.sType				= VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
		.attachmentCount	= attachmentDescriptionCount,
		.pAttachments		= pAttachmentDescriptions,
		.subpassCount		= 1,
		.pSubpasses			= &subpassDescription,
		.dependencyCount	= 1,
		.pDependencies		= &subpassDependency,
		.pNext				= 0,
		.flags				= 0,
	};
    VK_CHECK(vkCreateRenderPass(
				pCtx->device.handle,
				&renderPassCreateInfo,
				pCtx->pAllocator,
				&pOutRenderPass->handle));

    YINFO("RenderPass created.");
	return VK_SUCCESS;
}

void
vkRenderPassDestroy(VkContext* pCtx, VulkanRenderPass* pRenderPass) 
{
    if (pRenderPass && pRenderPass->handle) 
	{
        vkDestroyRenderPass(pCtx->device.handle, pRenderPass->handle, pCtx->pAllocator);
        pRenderPass->handle = 0;
    }
}

void
vkRenderPassBegin(VulkanCommandBuffer* pCommandBuffer, VulkanRenderPass* pRenderPass, VkFramebuffer frameBuffer) 
{
    VkRenderPassBeginInfo renderPassBeginInfo = {
		.sType						= VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
		.renderPass					= pRenderPass->handle,
		.framebuffer				= frameBuffer,
		.renderArea.offset.x		= pRenderPass->x,
		.renderArea.offset.y		= pRenderPass->y,
		.renderArea.extent.width	= pRenderPass->w,
		.renderArea.extent.height	= pRenderPass->h,
	};
	uint32_t clearValueCount = 2;
	/* NOTE: '2' has to be a literal to avoid VLA */
    VkClearValue pClearValues[2];
    yZeroMemory(pClearValues, sizeof(VkClearValue) * clearValueCount);
    pClearValues[0].color.float32[0] = pRenderPass->r;
    pClearValues[0].color.float32[1] = pRenderPass->g;
    pClearValues[0].color.float32[2] = pRenderPass->b;
    pClearValues[0].color.float32[3] = pRenderPass->a;
    pClearValues[1].depthStencil.depth = pRenderPass->depth;
    pClearValues[1].depthStencil.stencil = pRenderPass->stencil;

    renderPassBeginInfo.clearValueCount = clearValueCount;
    renderPassBeginInfo.pClearValues = pClearValues;

    vkCmdBeginRenderPass(pCommandBuffer->handle, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
    pCommandBuffer->state = COMMAND_BUFFER_STATE_IN_RENDER_PASS;
}

void
vkRenderPassEnd(VulkanCommandBuffer* pCommandBuffer, YMB VulkanRenderPass* pRenderPass) 
{
    vkCmdEndRenderPass(pCommandBuffer->handle);
    pCommandBuffer->state = COMMAND_BUFFER_STATE_RECORDING;
}
