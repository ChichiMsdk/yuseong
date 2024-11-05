#include "vulkan_pipeline.h"

#include "renderer/rendererimpl.h"

#include "core/filesystem.h"
#include "core/darray.h"
#include "core/darray_debug.h"
#include "core/logger.h"
#include "core/assert.h"
#include "core/yerror.h"

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
	if (errcode != 0)
	{
		char buffer[1024];
		OsStrError(buffer, 1024, errno);
		YERROR("%s", buffer);
		return VK_ERROR_UNKNOWN;
	}
	fseek(pStream, 0, SEEK_END);
	size_t fileSize = ftell(pStream);

	uint32_t* pBuffer = DarrayReserve(uint32_t, fileSize / sizeof(uint32_t));
	fseek(pStream, 0, SEEK_SET);

	size_t elemCount = 1;
	/* WARN: What should happen when this fails ? */
	size_t count = OsFread(pBuffer, fileSize, fileSize, elemCount, pStream);
	if (count != elemCount)
	{
		char msg[100];
		sprintf(msg, "OsFread: %zu", count);
		perror(msg);
	}

	errcode = OsFclose(pStream);
	if (errcode != 0)
	{
		char buffer[1024];
		OsStrError(buffer, 1024, errno);
		YERROR("%s", buffer);
		return VK_ERROR_UNKNOWN;
	}

	VkShaderModuleCreateInfo shaderModuleCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
		.pNext = VK_NULL_HANDLE,
		.codeSize = fileSize,
		.pCode = pBuffer,
	};
	VkShaderModule shaderModule;
	VK_CHECK(vkCreateShaderModule(device, &shaderModuleCreateInfo, pCtx->pAllocator, &shaderModule));
	DarrayDestroy(pBuffer);
	*pOutShaderModule = shaderModule;
	return VK_SUCCESS;
}

VkResult
vkPipelineInit(VkContext *pCtx, VkDevice device, const char* pFilePath)
{
	VkPipelineLayoutCreateInfo computePipelineLayoutCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		.pNext = VK_NULL_HANDLE,
		.pSetLayouts = &pCtx->drawImageDescriptorSetLayout,
		.setLayoutCount = 1,
	};
	VK_CHECK(vkCreatePipelineLayout(
				device,
				&computePipelineLayoutCreateInfo,
				pCtx->pAllocator,
				&pCtx->gradientComputePipelineLayout));

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
		.layout = pCtx->gradientComputePipelineLayout,
		.stage = pipelineShaderStageInfo,
	};

	uint32_t createInfoCount = 1;
	VkPipelineCache pipelineCache = VK_NULL_HANDLE;

	VK_CHECK(vkCreateComputePipelines(
				device,
				pipelineCache,
				createInfoCount,
				&computePipelineCreateInfo,
				pCtx->pAllocator,
				&pCtx->gradientComputePipeline));

	vkDestroyShaderModule(device, computeDrawShader, pCtx->pAllocator);

	return VK_SUCCESS;
}
