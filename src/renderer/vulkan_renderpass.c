#include "vulkan_renderpass.h"

#include "core/logger.h"
#include "core/ymemory.h"

/*
 * NOTE: In vulkan 1.3 dynamic rendering allows not to deal with this boilerplate
 * needs more information and benchmarks to see if there are any differences
 */
void 
vkRenderPassCreate(VkContext* pCtx, VulkanRenderPass* pOutRenderPass, struct ColorFloat color, struct RectFloat rect, 
		f32 depth, uint32_t stencil) 
{
    pOutRenderPass->x = rect.x; pOutRenderPass->y = rect.y;
    pOutRenderPass->w = rect.w; pOutRenderPass->h = rect.h;

    pOutRenderPass->r = color.r;
    pOutRenderPass->g = color.g;
    pOutRenderPass->b = color.b;
    pOutRenderPass->a = color.a;

    pOutRenderPass->depth = depth;
    pOutRenderPass->stencil = stencil;

    // Main subpass
    VkSubpassDescription subpass = {
		.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
	};

    // Attachments TODO: make this configurable.
    uint32_t attachmentDescriptionCount = 2;
    VkAttachmentDescription pAttachmentDescriptions[attachmentDescriptionCount];

    // Color attachment
    VkAttachmentDescription colorAttachment = {
		.format = pCtx->swapchain.imageFormat.format, // TODO: configurable
		.samples = VK_SAMPLE_COUNT_1_BIT,
		.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
		.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
		.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
		.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
		.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,      // Do not expect any particular layout before render pass starts.
		.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,  // Transitioned to after the render pass
		.flags = 0,
	};

    pAttachmentDescriptions[0] = colorAttachment;

    VkAttachmentReference colorAttachmentReference;
    colorAttachmentReference.attachment = 0;  // Attachment description array index
    colorAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentReference;

    // Depth attachment, if there is one
    VkAttachmentDescription depthAttachment = {
		.format = pCtx->device.depthFormat,
		.samples = VK_SAMPLE_COUNT_1_BIT,
		.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
		.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
		.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
		.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
		.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
	};
    pAttachmentDescriptions[1] = depthAttachment;

    // Depth attachment reference
    VkAttachmentReference depthAttachmentReference;
    depthAttachmentReference.attachment = 1;
    depthAttachmentReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    // TODO: other attachment types (input, resolve, preserve)

    // Depth stencil data.
    subpass.pDepthStencilAttachment = &depthAttachmentReference;
    // Input from a shader
    subpass.inputAttachmentCount = 0;
    subpass.pInputAttachments = 0;
    // Attachments used for multisampling colour attachments
    subpass.pResolveAttachments = 0;
    // Attachments not used in this subpass, but must be preserved for the next.
    subpass.preserveAttachmentCount = 0;
    subpass.pPreserveAttachments = 0;
    // Render pass dependencies. TODO: make this configurable.
    VkSubpassDependency dependency = {
		.srcSubpass = VK_SUBPASS_EXTERNAL,
		.dstSubpass = 0,
		.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		.srcAccessMask = 0,
		.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
		.dependencyFlags = 0,
	};
    // Render pass create.
	VkRenderPassCreateInfo renderPassCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
		.attachmentCount = attachmentDescriptionCount,
		.pAttachments = pAttachmentDescriptions,
		.subpassCount = 1,
		.pSubpasses = &subpass,
		.dependencyCount = 1,
		.pDependencies = &dependency,
		.pNext = 0,
		.flags = 0,
	};
    VK_ASSERT(vkCreateRenderPass(
				pCtx->device.logicalDev,
				&renderPassCreateInfo,
				pCtx->pAllocator,
				&pOutRenderPass->handle));
}

void
vkRenderPassDestroy(VkContext* pCtx, VulkanRenderPass* pRenderPass) 
{
    if (pRenderPass && pRenderPass->handle) 
	{
        vkDestroyRenderPass(pCtx->device.logicalDev, pRenderPass->handle, pCtx->pAllocator);
        pRenderPass->handle = 0;
    }
}

void
vkRenderPassBegin(VulkanCommandBuffer* pCommandBuffer, VulkanRenderPass* pRenderPass, VkFramebuffer frameBuffer) 
{
    VkRenderPassBeginInfo beginInfo = {
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
		.renderPass = pRenderPass->handle,
		.framebuffer = frameBuffer,
		.renderArea.offset.x = pRenderPass->x,
		.renderArea.offset.y = pRenderPass->y,
		.renderArea.extent.width = pRenderPass->w,
		.renderArea.extent.height = pRenderPass->h,
	};

    VkClearValue pClearValues[2];
    yZeroMemory(pClearValues, sizeof(VkClearValue) * 2);
    pClearValues[0].color.float32[0] = pRenderPass->r;
    pClearValues[0].color.float32[1] = pRenderPass->g;
    pClearValues[0].color.float32[2] = pRenderPass->b;
    pClearValues[0].color.float32[3] = pRenderPass->a;
    pClearValues[1].depthStencil.depth = pRenderPass->depth;
    pClearValues[1].depthStencil.stencil = pRenderPass->stencil;

    beginInfo.clearValueCount = 2;
    beginInfo.pClearValues = pClearValues;

    vkCmdBeginRenderPass(pCommandBuffer->handle, &beginInfo, VK_SUBPASS_CONTENTS_INLINE);
    pCommandBuffer->state = COMMAND_BUFFER_STATE_IN_RENDER_PASS;
}

void
vkRenderPassEnd(VulkanCommandBuffer* pCommandBuffer, YMB VulkanRenderPass* pRenderPass) 
{
    vkCmdEndRenderPass(pCommandBuffer->handle);
    pCommandBuffer->state = COMMAND_BUFFER_STATE_RECORDING;
}
