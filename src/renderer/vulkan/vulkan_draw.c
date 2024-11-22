#include "vulkan_draw.h"

#include "renderer/renderer_defines.h"
#include "vulkan_fence.h"
#include "vulkan_swapchain.h"
#include "vulkan_command.h"
#include "vulkan_image.h"
#include "vulkan_pipeline.h"
#include "vulkan_timer.h"

#include "core/yvec4.h"
#include "core/logger.h"
#include "cglm/mat4.h"

#include "profiler.h"

#include <math.h>

YND VkResult
vkComputeShaderInvocation(VkContext* pCtx, VulkanCommandBuffer* pCmd, ComputeShaderFx computeShader)
{
	VkPipelineBindPoint pipelineBindPoint = VK_PIPELINE_BIND_POINT_COMPUTE;
	vkCmdBindPipeline(pCmd->handle, pipelineBindPoint, computeShader.pipeline);

	const uint32_t*	pDynamicOffsets		= VK_NULL_HANDLE;
	uint32_t		firstSet			= 0;
	uint32_t		descriptorSetCount	= 1;
	uint32_t		dynamicOffsetCount	= 0;
	vkCmdBindDescriptorSets(
			pCmd->handle,
			pipelineBindPoint,
			computeShader.pipelineLayout,
			firstSet,
			descriptorSetCount,
			&pCtx->drawImageDescriptorSet,
			dynamicOffsetCount,
			pDynamicOffsets);

	VkShaderStageFlags	flags	= VK_SHADER_STAGE_COMPUTE_BIT;
	uint32_t			offset	= 0;
	uint32_t			size	= sizeof(ComputePushConstant);
	vkCmdPushConstants(
			pCmd->handle,
			pCtx->gradientComputePipelineLayout,
			flags,
			offset,
			size,
			&computeShader.pushConstant);

	uint32_t groupCountX = ceil(pCtx->drawImage.extent.width / 16.0f);
	uint32_t groupCountY = ceil(pCtx->drawImage.extent.height / 16.0f);
	uint32_t groupCountZ = 1;
	vkCmdDispatch(pCmd->handle, groupCountX, groupCountY, groupCountZ);

	return VK_SUCCESS;
}

extern const char *gppShaderFilePath[];
extern int32_t gShaderFileIndex;
static int32_t gOldFileShaderIndex;

YMB YND static VkResult
vkPipelineCheckReset(VkContext* pCtx, VkDevice device)
{
	if (gOldFileShaderIndex != gShaderFileIndex)
	{
		gOldFileShaderIndex = gShaderFileIndex;
		VK_CHECK(vkPipelineReset(pCtx, device, gppShaderFilePath[gShaderFileIndex]));
	}
	return VK_SUCCESS;
}

YMB static void
PrintColor(RgbaFloat c)
{
	YINFO("r: %f, g: %f, b: %f, a:%f", c.r, c.g, c.b, c.a);
}

/* FIXME: Resizing -> 
 * 	- Recreation of swapchain
 * 	- Recreation of bindings
 * 	- Get the new and correct framebuffer size
 * 	- Check Images actual state (Transition etc in yDrawImpl)
 */

void vkGeometryDraw(
		VkContext*							pCtx,
		VkCommandBuffer						commandBuffer,
		VkExtent2D							drawExtent,
		DrawImage							drawImage,
		DrawImage							depthImage,
		GenericPipeline*					pMeshPipeline,
		VkPipeline							trianglePipeline);

/* WARN: Leaking currently, needs to free at the beginning or at the end */
/* TODO: Profile ImageCopy&Co's */
YND VkResult
vkDrawImpl(VkContext* pCtx)
{
	TracyCZoneN(drawCtx, "yDraw", 1);

	YMB VkDevice	device				= pCtx->device.handle;
	uint64_t		fenceWaitTimeoutNs	= 1000 * 1000 * 1000; // 1 sec || 1 billion nanoseconds
	VK_CHECK(vkFenceWait(pCtx, &pCtx->pFencesInFlight[pCtx->currentFrame], fenceWaitTimeoutNs));
	/*
	 * NOTE: Fences have to be reset between uses, you can’t use the same
	 * fence on multiple GPU commands without resetting it in the middle.
	 */
	VK_CHECK(vkFenceReset(pCtx, &pCtx->pFencesInFlight[pCtx->currentFrame]));

	/* NOTE: Get next swapchain image */
	VK_CHECK(vkSwapchainAcquireNextImageIndex(
				pCtx,
				&pCtx->swapchain,
				fenceWaitTimeoutNs,
				pCtx->pSemaphoresAvailableImage[pCtx->currentFrame],
				VK_NULL_HANDLE, 
				&pCtx->imageIndex));

	/* NOTE: Getting the current frame's cmd buffer and reset it */
	VulkanCommandBuffer*		pCmd	= &pCtx->pGfxCommands[pCtx->imageIndex];
	VkCommandBufferResetFlags	flags	= 0;
	VK_CHECK(vkCommandBufferReset(pCmd, flags));

	DrawImage		drawImage			= pCtx->drawImage;
	/* YDEBUG("Format is: %s", string_VkFormat(depthImage.format)); */
	/* exit(1); */
	DrawImage		depthImage			= pCtx->depthImage;
	VkImage			swapchainImage		= pCtx->swapchain.pImages[pCtx->imageIndex];

	/* NOTE: Start command recording */
	b8				bSingleUse			= TRUE;
	b8				bSimultaneousUse	= FALSE;
	b8				bRenderPassContinue	= FALSE;
	vkCommandBufferBegin(pCmd, bSingleUse, bRenderPassContinue, bSimultaneousUse);

	/* NOTE: Start counter */
	vkTimerStart(pCmd->handle);

	/* NOTE: Make the swapchain image into general mode */
	VkImageLayout	currentLayout		= VK_IMAGE_LAYOUT_UNDEFINED;
	VkImageLayout	newLayout			= VK_IMAGE_LAYOUT_GENERAL;
	vkImageTransition(pCmd, drawImage.image.handle, currentLayout, newLayout);

	/* NOTE: Compute shader invocation */
	VK_CHECK(vkComputeShaderInvocation(pCtx, pCmd, pCtx->pComputeShaders[gShaderFileIndex]));

	/* NOTE: make the drawable image and the depth image into proper layout */
	vkImageTransition(pCmd, drawImage.image.handle, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
	vkImageTransition(pCmd, depthImage.image.handle, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);

	/* NOTE: Draw the geometry pipeline */
	YMB VkExtent2D extent = { .width = pCtx->swapchain.extent.width, .height = pCtx->swapchain.extent.height, };
	vkGeometryDraw(
			pCtx,
			pCmd->handle,
			extent,
			pCtx->drawImage,
			depthImage,
			&pCtx->meshPipeline,
			pCtx->triPipeline.pipeline);

	/* NOTE: make the drawable image and the swapchain image into layout for copies */
	currentLayout	= VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	newLayout		= VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
	vkImageTransition(pCmd, drawImage.image.handle, currentLayout, newLayout);
	vkImageTransition(pCmd, swapchainImage, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

	/* NOTE: Copy the image to the swapchain*/
	VkExtent2D drawExtent		= {.width = drawImage.extent.width, .height = drawImage.extent.height};
	VkExtent2D swapchainExtent	= {.width = pCtx->swapchain.extent.width, .height = drawImage.extent.height};
	vkImageCopy(pCmd->handle, drawImage.image.handle, swapchainImage, drawExtent, swapchainExtent);

	/* NOTE: Make the swapchain image into presentable mode */
	vkImageTransition(pCmd, swapchainImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

	/* NOTE: Stop the timer and log */
	vkTimerEnd(pCmd->handle);
	/* vkTimerLog(pCtx->device.properties, pCtx->device.graphicsQueue, device, MICROSECONDS); */

	/* NOTE: End command recording */
	VK_CHECK(vkCommandBufferEnd(pCmd));

	/* NOTE: submit command buffer to the queue and execute it. */
	VK_CHECK(vkQueueSubmitAndSwapchainPresent(pCtx, pCmd));

	TracyCZoneEnd(drawCtx);
	return VK_SUCCESS;
}

void
vkClearBackground(VkContext* pCtx, VulkanCommandBuffer* pCmd, VkImage pImage)
{
	f32					flashSpeed	= 960.0f;
	YMB f32				flash		= fabs(sin(pCtx->nbFrames / flashSpeed));
	VkClearColorValue	clearValue	= {.float32 = {0.0f, 0.0f, flash, 1.0f}};

	VkImageSubresourceRange clearRange = {
		.aspectMask		= VK_IMAGE_ASPECT_COLOR_BIT,
		.baseMipLevel	= 0,
		.levelCount		= VK_REMAINING_MIP_LEVELS,
		.baseArrayLayer	= 0,
		.layerCount		= VK_REMAINING_ARRAY_LAYERS,
	};

	uint32_t 		rangeCount				= 1;
	VkImageLayout	clearColorImageLayout	= VK_IMAGE_LAYOUT_GENERAL;
	vkCmdClearColorImage(pCmd->handle, pImage, clearColorImageLayout, &clearValue, rangeCount, &clearRange);
}

YND VkResult
vkQueueSubmitAndSwapchainPresent(VkContext* pCtx, VulkanCommandBuffer* pCmd)
{
	VkSemaphoreSubmitInfo semaphoreWaitSubmitInfo = {
		.sType						= VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
		.semaphore					= pCtx->pSemaphoresAvailableImage[pCtx->currentFrame],
		.stageMask					= VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
		.value						= 1,
	};
	VkSemaphoreSubmitInfo semaphoreSignalSubmitInfo = {
		.sType 						= VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
		.semaphore 					= pCtx->pSemaphoresQueueComplete[pCtx->currentFrame],
		.stageMask 					= VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
		.value 						= 1,
	};
	VkCommandBufferSubmitInfo cmdBufferSubmitInfo = {
		.sType						= VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
		.commandBuffer				= pCmd->handle,
	};
	VkSubmitInfo2 submitInfo2 = {
		.sType						= VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
		.commandBufferInfoCount		= 1,
		.pCommandBufferInfos		= &cmdBufferSubmitInfo,
		.pWaitSemaphoreInfos		= &semaphoreWaitSubmitInfo,
		.waitSemaphoreInfoCount		= 1,
		.pSignalSemaphoreInfos		= &semaphoreSignalSubmitInfo,
		.signalSemaphoreInfoCount	= 1,
	};

	uint32_t submitCount = 1;
	VK_CHECK(vkQueueSubmit2(
				pCtx->device.graphicsQueue,
				submitCount,
				&submitInfo2,
				pCtx->pFencesInFlight[pCtx->currentFrame].handle));

	pCmd->state = COMMAND_BUFFER_STATE_SUBMITTED;

	VK_CHECK(vkSwapchainPresent(
				pCtx,
				&pCtx->swapchain,
				pCtx->device.graphicsQueue,
				pCtx->device.presentQueue,
				pCtx->pSemaphoresQueueComplete[pCtx->currentFrame],
				pCtx->imageIndex));

	return VK_SUCCESS;
}

#include <stdio.h>

void
vkGeometryDraw(
		VkContext*							pCtx,
		VkCommandBuffer						commandBuffer,
		VkExtent2D							drawExtent,
		DrawImage							drawImage,
		DrawImage							depthImage,
		GenericPipeline*					pMeshPipeline,
		VkPipeline							trianglePipeline)
{
	/* NOTE: begin a render pass  connected to our draw image */
	VkRenderingAttachmentInfo	colorAttachment	= {
		.sType			= VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
		.imageView		= drawImage.image.view,
		.imageLayout	= VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		.loadOp			= VK_ATTACHMENT_LOAD_OP_LOAD,
		.storeOp		= VK_ATTACHMENT_STORE_OP_STORE,
	};

	VkRenderingAttachmentInfo	depthAttachment	= {
		.sType							= VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
		.imageView						= depthImage.image.view,
		.imageLayout					= VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
		.loadOp							= VK_ATTACHMENT_LOAD_OP_CLEAR,
		.storeOp						= VK_ATTACHMENT_STORE_OP_STORE,
		.clearValue.depthStencil.depth	= 0.f,

	};

	VkRenderingInfo	renderingInfo	= {
		.sType					= VK_STRUCTURE_TYPE_RENDERING_INFO,
		.colorAttachmentCount	= 1,
		.pStencilAttachment		= VK_NULL_HANDLE,
		.pColorAttachments		= &colorAttachment,
		.pDepthAttachment		= &depthAttachment,
		.renderArea = {
			.offset				= (VkOffset2D) {0}, 
			.extent 			= drawExtent,
		},
		.layerCount				= 1,
	}; 

	vkCmdBeginRendering(commandBuffer, &renderingInfo);

	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, trianglePipeline);

	/* NOTE: set dynamic viewport and scissor */
	VkViewport	viewport	=	{
		.x			=	0,
		.y			=	0,
		.width		=	drawExtent.width,
		.height		=	drawExtent.height,
		.minDepth	=	0.f,
		.maxDepth	=	1.f,
	};

	uint32_t	firstViewport	=	0;
	uint32_t	viewportCount	=	1;
	vkCmdSetViewport(commandBuffer, firstViewport, viewportCount, &viewport);

	VkRect2D	scissor	=	{
		.offset.x		=	0,
		.offset.y		=	0,
		.extent.width	=	viewport.width,
		.extent.height	=	viewport.height,
	};

	uint32_t	firstScissor	=	0;
	uint32_t	scissorCount	=	1;
	vkCmdSetScissor(commandBuffer, firstScissor, scissorCount, &scissor);

	/* NOTE: launch a draw command to draw 3 vertices */
	YMB uint32_t	vertexCount		=	3;
	uint32_t	instanceCount	=	1;
	YMB uint32_t	firstVertex		=	0;
	uint32_t	firstInstance	=	0;
	vkCmdDraw(commandBuffer, vertexCount, instanceCount, firstVertex, firstInstance);

	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pMeshPipeline->pipeline);

	GpuDrawPushConstants	pushConstants	= {0};
	mat4					matrice			= GLM_MAT4_IDENTITY_INIT;

	glm_mat4_copy(matrice, pushConstants.worldMatrix);
	pushConstants.vertexBufferAddress = pCtx->gpuMeshBuffers.vertexBufferAddress;

	uint32_t			offset		= 0;
	uint32_t			size		= sizeof(GpuDrawPushConstants);
	VkShaderStageFlags	stageFlags	= VK_SHADER_STAGE_VERTEX_BIT;
	vkCmdPushConstants(
			commandBuffer,
			pMeshPipeline->pipelineLayout,
			stageFlags,
			offset,
			size,
			&pushConstants);

	VkIndexType	indexType	= VK_INDEX_TYPE_UINT32;
	vkCmdBindIndexBuffer(commandBuffer, pCtx->gpuMeshBuffers.indexBuffer.handle, offset, indexType);

	uint32_t	indexCount		= 6;
	uint32_t	vertexOffset	= 0;
	uint32_t	firstIndex		= 0;

	instanceCount	= 1;
	firstInstance	= 0;
	vkCmdDrawIndexed(commandBuffer, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);

	vkCmdEndRendering(commandBuffer);
}

VkResult
vkImmediateSubmit(VkContext* pCtx, VulkanDevice device, void (*function)(VkCommandBuffer cmd))
{
	VK_CHECK(vkFenceReset(pCtx, &device.immediateSubmit.fence));

	b8 bSingleUse			= TRUE;
	b8 bRenderPassContinue	= FALSE;
	b8 bSimultaneousUse		= FALSE;
	vkCommandBufferBegin(&device.immediateSubmit.commandBuffer, bSingleUse, bRenderPassContinue, bSimultaneousUse);

	function(device.immediateSubmit.commandBuffer.handle);

	VK_CHECK(vkEndCommandBuffer(device.immediateSubmit.commandBuffer.handle));

	VkCommandBufferSubmitInfo cmdBufferSubmitInfo = {
		.sType						= VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
		.commandBuffer				= device.immediateSubmit.commandBuffer.handle,
	};
	VkSubmitInfo2 submitInfo2 = {
		.sType						= VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
		.commandBufferInfoCount		= 1,
		.pCommandBufferInfos		= &cmdBufferSubmitInfo,
		.pWaitSemaphoreInfos		= VK_NULL_HANDLE,
		.waitSemaphoreInfoCount		= 0,
		.pSignalSemaphoreInfos		= VK_NULL_HANDLE,
		.signalSemaphoreInfoCount	= 0,
	};
	uint32_t	submitCount	= 1;
	uint32_t	fenceCount	= 1;
	VK_CHECK(vkQueueSubmit2(device.graphicsQueue, submitCount, &submitInfo2, device.immediateSubmit.fence.handle));
	VK_CHECK(vkWaitForFences(device.handle, fenceCount, &device.immediateSubmit.fence.handle, TRUE, 9999999999));

	return VK_SUCCESS;
}

/*
 * VkPipelineShaderStageCreateInfo pipelineShaderStageInfo = {
 * 	.sType	= VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
 * 	.pNext	= VK_NULL_HANDLE,
 * 	.stage	= VK_SHADER_STAGE_COMPUTE_BIT,
 * 	.pName	= "main",
 * };
 */

