#include "vulkan_command.h"
#include "core/logger.h"
#include "core/darray.h"

#include <string.h>
#include <vulkan/vk_enum_string_helper.h>

/**
 * Each thread needs to have its very own CommandPool AND
 * CommandBuffer (minimum) -> https://vkguide.dev/docs/new_chapter_1/vulkan_command_flow/ 
 */
YND VkResult
vkCommandPoolCreate(VkContext *pContext)
{
	VkCommandPoolCreateInfo poolCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
		.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT
	};
	VK_CHECK(vkCreateCommandPool(pContext->device.logicalDev, &poolCreateInfo, pContext->pAllocator, 
				&pContext->device.graphicsCommandPool));
    YINFO("Graphics command pool created.");
	return VK_SUCCESS;
}

YND VkResult 
vkCommandBufferCreate(VkContext *pCtx)
{
    if (!pCtx->pGfxCommands) 
	{
        pCtx->pGfxCommands = darray_reserve(VulkanCommandBuffer, pCtx->swapchain.imageCount);
        for (uint32_t i = 0; i < pCtx->swapchain.imageCount; ++i) {
            memset(&pCtx->pGfxCommands[i], 0, sizeof(VulkanCommandBuffer));
        }
    }

    for (uint32_t i = 0; i < pCtx->swapchain.imageCount; ++i) 
	{
        if (pCtx->pGfxCommands[i].handle) 
		{
            /*
             * vulkan_command_buffer_free(
             *     &context,
             *     context.device.graphics_command_pool,
             *     &context.graphics_command_buffers[i]);
             */
        }
        memset(&pCtx->pGfxCommands[i], 0, sizeof(VulkanCommandBuffer));
        VK_CHECK(vkCommandBufferAllocate(pCtx, VK_COMMAND_BUFFER_LEVEL_PRIMARY, pCtx->device.graphicsCommandPool,
				&pCtx->pGfxCommands[i]));
    }
    YINFO("Vulkan command buffers created.");
	return VK_SUCCESS;
}

YND VkResult
vkCommandBufferAllocate(VkContext *pCtx, VkCommandBufferLevel cmdBufferLvl, VkCommandPool pool,
		VulkanCommandBuffer *pOutCommandBuffers)
{
	VkDevice device = pCtx->device.logicalDev;
	memset(pOutCommandBuffers, 0, sizeof(VulkanCommandBuffer));

	VkCommandBufferAllocateInfo allocateInfo = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.level = cmdBufferLvl,
		.pNext = VK_NULL_HANDLE,
		.commandPool = pool,
		.commandBufferCount = 1
	};
	VK_CHECK(vkAllocateCommandBuffers(device, &allocateInfo, &pOutCommandBuffers->handle));
	pOutCommandBuffers->state = COMMAND_BUFFER_STATE_READY;
	return VK_SUCCESS;
}

YND VkResult
vkCommandPoolDestroy(VkContext *pCtx, VkCommandPool *pCmdPools, uint32_t poolCount)
{
	VkDevice device = pCtx->device.logicalDev;
	VK_CHECK(vkDeviceWaitIdle(device));
	for (uint32_t i = 0; i < poolCount; i++)
		vkDestroyCommandPool(device, pCmdPools[i], pCtx->pAllocator);
	return VK_SUCCESS;
}

YND VkResult
vkCommandBufferFree(VkContext *pCtx, VulkanCommandBuffer *pCommandBuffers, VkCommandPool *pPool, uint32_t poolCount)
{
	VkDevice device = pCtx->device.logicalDev;
	VK_CHECK(vkDeviceWaitIdle(device));

	for (uint32_t i = 0; i < poolCount; i++)
		vkFreeCommandBuffers(device, *pPool, 1, &pCommandBuffers[i].handle);

    /*
	 * NOTE: This here feels redundant 
	 * if NULL then assume COMMAND_BUFFER_STATE_NOT_ALLOCATED
     */
	pCommandBuffers->handle = VK_NULL_HANDLE;
	pCommandBuffers->state = COMMAND_BUFFER_STATE_NOT_ALLOCATED;
	return VK_SUCCESS;
}

void
vkCommandBufferBegin(VulkanCommandBuffer* pCmd, b8 bSingleUse, b8 bRenderPassContinue, b8 bSimultaneousUse)
{
    VkCommandBufferBeginInfo beginInfo = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.flags = 0,
	};
    if (bSingleUse) 
        beginInfo.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    if (bRenderPassContinue) 
        beginInfo.flags |= VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;
    if (bSimultaneousUse) 
        beginInfo.flags |= VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

    VK_ASSERT(vkBeginCommandBuffer(pCmd->handle, &beginInfo));
    pCmd->state = COMMAND_BUFFER_STATE_RECORDING;
}

YND VkResult 
vkCommandBufferEnd(VulkanCommandBuffer* pCmd)
{
	VK_CHECK(vkEndCommandBuffer(pCmd->handle));
	pCmd->state = COMMAND_BUFFER_STATE_RECORDING_ENDED;
	return VK_SUCCESS;
}

YND VkResult 
vkCommandBufferReset(VulkanCommandBuffer* pCmd, VkCommandBufferResetFlags flags)
{
	VK_CHECK(vkResetCommandBuffer(pCmd->handle, flags));
	pCmd->state = COMMAND_BUFFER_STATE_READY;
	return VK_SUCCESS;
}
