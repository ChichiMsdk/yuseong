#ifndef VULKAN_PIPELINE_H
#define VULKAN_PIPELINE_H

#include "renderer/vulkan/yvulkan.h"
#include "core/filesystem.h"
#include "core/darray.h"
#include "core/logger.h"
#include "core/assert.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>


/*
 * TODO: Change error handling here
 * Needs to take into account fopen failures
 */
VkResult
vkLoadShaderModule(VkContext *pCtx, const char* pFilePath, VkDevice device, VkShaderModule* pOutShaderModule)
{
	FILE* pStream;
	int errcode = OsFopen(&pStream, pFilePath, "rb");
	if (!errcode)
	{
		char buffer[1024];
		OsStrError(buffer, 1024, errno);
		YERROR("%s in %s:%d", buffer, __FILE__, __LINE__);
		return VK_ERROR_UNKNOWN;
	}
	fseek(pStream, 0, SEEK_END);
	size_t fileSize = ftell(pStream);
	YDEBUG("file size: %llu", fileSize);

	uint32_t* pBuffer = DarrayReserve(uint32_t, fileSize);
	fseek(pStream, 0, SEEK_SET);
	OsFread(pBuffer, pStream, 1, 1, fileSize);
	OsFclose(pStream);

	VkShaderModuleCreateInfo shaderModuleCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
		.pNext = VK_NULL_HANDLE,
		.codeSize = fileSize,
		.pCode = pBuffer,
	};
	VK_CHECK(vkCreateShaderModule(device, &shaderModuleCreateInfo, pCtx->pAllocator, pOutShaderModule));
	DarrayDestroy(pBuffer);
	return VK_SUCCESS;
}

VkResult
vkPipelineInit(VkContext *pCtx, VkDevice device, const char* pFilePath)
{
	VkShaderModule computeDrawShader;
	VK_CHECK(vkLoadShaderModule(pCtx, pFilePath, device, &computeDrawShader));

	VkPipelineShaderStageCreateInfo pipelineShaderStageInfo = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
		.pNext = VK_NULL_HANDLE,
		.stage = VK_SHADER_STAGE_COMPUTE_BIT,
		.module = computeDrawShader,
		.pName = "main",
	};
	VkComputePipelineCreateInfo computePipelineCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
		.pNext = VK_NULL_HANDLE,
		// .layout = _gradientPipelineLayout,
		.stage = pipelineShaderStageInfo,
	};
	uint32_t createInfoCount = 1;
	VkPipelineCache pipelineCache = VK_NULL_HANDLE;
	VkPipeline *pPipelines = VK_NULL_HANDLE;

	VK_CHECK(vkCreateComputePipelines(
				device,
				pipelineCache,
				createInfoCount,
				&computePipelineCreateInfo,
				pCtx->pAllocator,
				pPipelines));

	return VK_SUCCESS;
}

#endif //VULKAN_PIPELINE_H
