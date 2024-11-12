#ifndef MACRO_UTILS_H
#define MACRO_UTILS_H

#include <vulkan/vulkan.h>
#include <vulkan/vk_enum_string_helper.h>

#include "core/logger.h"
#include "core/myassert.h"

#define VK_ASSERT(expr) YASSERT_MSG(expr == VK_SUCCESS, string_VkResult(expr));

#define VK_RESULT(expr) \
	do { \
		VkResult errcode = (expr); \
		if (errcode != VK_SUCCESS) \
		{ \
			return errcode; \
		} \
	} while (0);

#define VK_CHECK(expr) \
	do { \
		VkResult errcode = (expr); \
		if (errcode != VK_SUCCESS) \
		{ \
			YERROR2("%s", string_VkResult(errcode)); \
			return errcode; \
		} \
	} while (0);

#define vkPipelinesCleanUp(pCtx, device) \
	do { \
		VK_ASSERT(vkDeviceWaitIdle(device)); \
		vkDestroyPipeline(device, pCtx->gradientComputePipeline, pCtx->pAllocator); \
		vkDestroyPipelineLayout(device, pCtx->gradientComputePipelineLayout, pCtx->pAllocator); \
		uint64_t size = DarrayCapacity(pCtx->pComputeShaders); \
		for (uint64_t i = 0; i < size; i++) { \
			vkDestroyPipeline(device, pCtx->pComputeShaders[i].pipeline, pCtx->pAllocator); \
		} \
		DarrayDestroy(pCtx->pComputeShaders);\
		vkDestroyPipeline(device, pCtx->triPipeline.pipeline, pCtx->pAllocator); \
		vkDestroyPipelineLayout(device, pCtx->triPipeline.pipelineLayout, pCtx->pAllocator); \
		vkDestroyPipeline(device, pCtx->meshPipeline.pipeline, pCtx->pAllocator); \
		vkDestroyPipelineLayout(device, pCtx->meshPipeline.pipelineLayout, pCtx->pAllocator); \
	} while(0);

#endif  // MACRO_UTILS_H
