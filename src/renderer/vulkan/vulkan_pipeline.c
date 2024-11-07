#include "vulkan_pipeline.h"

#include "renderer/rendererimpl.h"

#include "core/filesystem.h"
#include "core/darray.h"
#include "core/darray_debug.h"
#include "core/logger.h"
#include "core/myassert.h"
#include "core/yerror.h"
#include "core/vec4.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>

#define IO_CHECK(expr) \
	do { \
		int errcode = expr; \
		if (errcode != 0) { \
			char buffer[1024]; \
			OsStrError(buffer, 1024, errno); \
			YERROR("%s", buffer); \
			return VK_ERROR_UNKNOWN; \
		} \
	} while (0);

/*
 * TODO: Change error handling here
 * Needs to take into account fopen failures
 */
VkResult
vkLoadShaderModule(VkContext *pCtx, const char* pFilePath, VkDevice device, VkShaderModule* pOutShaderModule)
{
	uint32_t*	pBuffer;
	FILE*		pStream;
	size_t		fileSize;
	size_t		count;
	size_t		elemCount = 1;

	IO_CHECK(OsFopen(&pStream, pFilePath, "rb"));
	IO_CHECK(fseek(pStream, 0, SEEK_END));
	fileSize = ftell(pStream);

	pBuffer = DarrayReserve(uint32_t, fileSize / sizeof(uint32_t));
	fseek(pStream, 0, SEEK_SET);

	/* WARN: What should happen when this fails ? */
	count = OsFread(pBuffer, fileSize, fileSize, elemCount, pStream);
	if (count != elemCount)
	{
		char msg[100];
		sprintf(msg, "OsFread: %zu", count);
		perror(msg);
	}
	IO_CHECK(OsFclose(pStream));

	VkShaderModuleCreateInfo shaderModuleCreateInfo = {
		.sType		= VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
		.pNext		= VK_NULL_HANDLE,
		.codeSize	= fileSize,
		.pCode		= pBuffer,
	};
	VK_CHECK(vkCreateShaderModule(device, &shaderModuleCreateInfo, pCtx->pAllocator, pOutShaderModule));

	DarrayDestroy(pBuffer);
	return VK_SUCCESS;
}

VkResult
vkPipelineInit(VkContext *pCtx, VkDevice device, const char* pFilePath)
{
	VkPushConstantRange pushConstantRange = {
		.offset = 0,
		.size = sizeof(ComputePushConstant),
		.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
	};
	VkPipelineLayoutCreateInfo computePipelineLayoutCreateInfo = {
		.sType					= VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		.pNext					= VK_NULL_HANDLE,
		.pSetLayouts			= &pCtx->drawImageDescriptorSetLayout,
		.setLayoutCount			= 1,
		.pPushConstantRanges	= &pushConstantRange,
		.pushConstantRangeCount	= 1,
	};
	VK_CHECK(vkCreatePipelineLayout(
				device,
				&computePipelineLayoutCreateInfo,
				pCtx->pAllocator,
				&pCtx->gradientComputePipelineLayout));

	VkShaderModule computeDrawShader;
	VK_CHECK(vkLoadShaderModule(pCtx, pFilePath, device, &computeDrawShader));

	VkPipelineShaderStageCreateInfo pipelineShaderStageInfo = {
		.sType	= VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
		.pNext	= VK_NULL_HANDLE,
		.stage	= VK_SHADER_STAGE_COMPUTE_BIT,
		.module	= computeDrawShader,
		.pName	= "main",
	};
	VkComputePipelineCreateInfo computePipelineCreateInfo = {
		.sType	= VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
		.pNext	= VK_NULL_HANDLE,
		.layout	= pCtx->gradientComputePipelineLayout,
		.stage	= pipelineShaderStageInfo,
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
