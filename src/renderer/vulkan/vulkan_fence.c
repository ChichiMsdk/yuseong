#include "vulkan_fence.h"

#include "core/logger.h"

YND VkResult
vkFenceCreate(VkDevice device, b8 bSignaled, VkAllocationCallbacks* pAllocator, VulkanFence *pOutFence)
{
    /* NOTE: Make sure to signal the fence if required. */
    pOutFence->bSignaled = bSignaled;
    VkFenceCreateInfo fenceCreateInfo = {
		.sType	= VK_STRUCTURE_TYPE_FENCE_CREATE_INFO
	};

    if (pOutFence->bSignaled)
		fenceCreateInfo.flags	= VK_FENCE_CREATE_SIGNALED_BIT;

    VK_CHECK(vkCreateFence(device, &fenceCreateInfo, pAllocator, &pOutFence->handle));
	return VK_SUCCESS;
}

void 
vkFenceDestroy(VkContext *pCtx, VulkanFence *pFence)
{
	vkDestroyFence(pCtx->device.handle, pFence->handle, pCtx->pAllocator);
	pFence->handle = VK_NULL_HANDLE;
    pFence->bSignaled = FALSE;
}

YND VkResult 
vkFenceWait(VkContext *pCtx, VulkanFence *pFence, uint64_t timeoutNs) 
{
	VkResult res = VK_SUCCESS;
	if (!pFence->bSignaled) 
	{
		res = vkWaitForFences(pCtx->device.handle, 1, &pFence->handle, TRUE, timeoutNs);
		switch (res) 
		{
			case VK_SUCCESS: pFence->bSignaled = TRUE; return res;
			case VK_TIMEOUT: YWARN("vk_fence_wait - %s", string_VkResult(res)); break;
			case VK_ERROR_DEVICE_LOST: YERROR("vk_fence_wait - %s.", string_VkResult(res)); break;
			case VK_ERROR_OUT_OF_HOST_MEMORY: YERROR("vk_fence_wait - %s.", string_VkResult(res)); break;
			case VK_ERROR_OUT_OF_DEVICE_MEMORY: YERROR("vk_fence_wait - %s.", string_VkResult(res)); break;
			default: YERROR("vk_fence_wait - %s.", string_VkResult(res)); break;
		}
	} 
	return res;
}

YND VkResult 
vkFenceReset(VkContext *pCtx, VulkanFence *pFence) 
{
    if (pFence->bSignaled) 
	{
        VK_CHECK(vkResetFences(pCtx->device.handle, 1, &pFence->handle));
        pFence->bSignaled = FALSE;
    }
	return VK_SUCCESS;
}
