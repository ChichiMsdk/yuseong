#include "vulkan_draw.h"

#include "renderer/renderer_defines.h"
#include "vulkan_fence.h"
#include "vulkan_swapchain.h"
#include "vulkan_command.h"
#include "vulkan_image.h"
#include "vulkan_pipeline.h"

#include "core/yvec4.h"
#include "core/logger.h"

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

/* WARN: Leaking currently, needs to free at the beginning or at the end */
/* TODO: Profile ImageCopy&Co's */
YND VkResult
vkDrawImpl(VkContext* pCtx)
{
	TracyCZoneN(drawCtx, "yDraw", 1);

	YMB VkDevice	device				= pCtx->device.logicalDev;
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

	/* NOTE: Start command recording */
	b8				bSingleUse			= TRUE;
	b8				bSimultaneousUse	= FALSE;
	b8				bRenderPassContinue	= FALSE;
	DrawImage		drawImage			= pCtx->drawImage;
	VkImage			swapchainImage		= pCtx->swapchain.pImages[pCtx->imageIndex];
	vkCommandBufferBegin(pCmd, bSingleUse, bRenderPassContinue, bSimultaneousUse);

	/* NOTE: Make the swapchain image into general mode */
	VkImageLayout	currentLayout		= VK_IMAGE_LAYOUT_UNDEFINED;
	VkImageLayout	newLayout			= VK_IMAGE_LAYOUT_GENERAL;
	vkImageTransition(pCmd, drawImage.image.handle, currentLayout, newLayout);

	/* NOTE: Clear the background */
	/* vkClearBackground(pCmd, drawImage.image.handle); */

	/* NOTE: Compute shader invocation */
	VK_CHECK(vkComputeShaderInvocation(pCtx, pCmd, pCtx->pComputeShaders[gShaderFileIndex]));

	/* NOTE: Make the swapchain image into presentable mode */
	newLayout		= VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
	currentLayout	= VK_IMAGE_LAYOUT_GENERAL;
	vkImageTransition(pCmd, drawImage.image.handle, currentLayout, newLayout);
	vkImageTransition(pCmd, swapchainImage, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

	/* NOTE: Copy the image */
	VkExtent2D drawExtent		= {.width = drawImage.extent.width, .height = drawImage.extent.height};
	VkExtent2D swapchainExtent	= {.width = pCtx->swapchain.extent.width, .height = drawImage.extent.height};
	vkImageCopy(pCmd->handle, drawImage.image.handle, swapchainImage, drawExtent, swapchainExtent);

	vkImageTransition(pCmd, swapchainImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

	/* NOTE: End command recording */
	VK_CHECK(vkCommandBufferEnd(pCmd));
	/*
	 * NOTE: submit command buffer to the queue and execute it.
	 * 	_renderFence will now block until the graphic commands finish execution
	 */
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
	VK_CHECK(vkWaitForFences(device.logicalDev, fenceCount, &device.immediateSubmit.fence.handle, TRUE, 9999999999));

	return VK_SUCCESS;
}
