#include "vulkan_pipeline.h"

#include "renderer/rendererimpl.h"

#include "core/filesystem.h"
#include "core/darray.h"
#include "core/darray_debug.h"
#include "core/logger.h"
#include "core/myassert.h"
#include "core/yerror.h"

#include "core/yvec4.h"

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
 *
 * TODO: Keep those files loaded in memory ?
 */
YND VkResult
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

YND VkResult
vkComputePipelineInit(VkContext *pCtx, VkDevice device, const char** ppShaderPaths)
{
	pCtx->pComputeShaders = DarrayReserve(ComputeShaderFx, 3);

	/* NOTE: Init pipelineLayout */
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

	/* NOTE: Setup PipelineInfo */
	uint32_t createInfoCount = 1;
	VkPipelineCache pipelineCache = VK_NULL_HANDLE;

	VkPipelineShaderStageCreateInfo pipelineShaderStageInfo = {
		.sType	= VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
		.pNext	= VK_NULL_HANDLE,
		.stage	= VK_SHADER_STAGE_COMPUTE_BIT,
		.pName	= "main",
	};
	VkComputePipelineCreateInfo computePipelineCreateInfo = {
		.sType	= VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
		.pNext	= VK_NULL_HANDLE,
		.layout	= pCtx->gradientComputePipelineLayout,
		.stage	= pipelineShaderStageInfo,
	};

	/* NOTE: Load and init pipeline of gradientColorDrawShader */
	VkShaderModule gradientDrawShader;
	VkShaderModule skyDrawShader;
	VkShaderModule gradientColorDrawShader;
	VK_CHECK(vkLoadShaderModule(pCtx, ppShaderPaths[0], device, &gradientColorDrawShader));
	VK_CHECK(vkLoadShaderModule(pCtx, ppShaderPaths[1], device, &gradientDrawShader));
	VK_CHECK(vkLoadShaderModule(pCtx, ppShaderPaths[2], device, &skyDrawShader));

	/* NOTE: Load and init pipeline of gradientDrawShader */
	pCtx->pComputeShaders[0].pFilePath = ppShaderPaths[0];
	pCtx->pComputeShaders[0].pipelineLayout = pCtx->gradientComputePipelineLayout;
	Vec4Fill(1.0f, 0.0f, 0.0f, 1.0f, pCtx->pComputeShaders[0].pushConstant.data1);
	Vec4Fill(0.0f, 0.0f, 1.0f, 1.0f, pCtx->pComputeShaders[0].pushConstant.data2);
	computePipelineCreateInfo.stage.module = gradientColorDrawShader;
	VK_CHECK(vkCreateComputePipelines(
				device,
				pipelineCache,
				createInfoCount,
				&computePipelineCreateInfo,
				pCtx->pAllocator,
				&pCtx->pComputeShaders[0].pipeline));

	/* NOTE: Load and init pipeline of gradientDrawShader */
	pCtx->pComputeShaders[1].pFilePath = ppShaderPaths[1];
	pCtx->pComputeShaders[1].pipelineLayout = pCtx->gradientComputePipelineLayout;
	Vec4Fill(1.0f, 0.0f, 0.0f, 1.0f, pCtx->pComputeShaders[1].pushConstant.data1);
	Vec4Fill(0.0f, 0.0f, 1.0f, 1.0f, pCtx->pComputeShaders[1].pushConstant.data2);
	computePipelineCreateInfo.stage.module = gradientDrawShader;
	VK_CHECK(vkCreateComputePipelines(
				device,
				pipelineCache,
				createInfoCount,
				&computePipelineCreateInfo,
				pCtx->pAllocator,
				&pCtx->pComputeShaders[1].pipeline));

	/* NOTE: Load and init pipeline of skyDrawShader */
	pCtx->pComputeShaders[2].pFilePath = ppShaderPaths[2];
	pCtx->pComputeShaders[2].pipelineLayout = pCtx->gradientComputePipelineLayout;
	Vec4Fill(0.1f, 0.2f, 0.4f, 0.97f, pCtx->pComputeShaders[2].pushConstant.data1);
	computePipelineCreateInfo.stage.module = skyDrawShader;
	VK_CHECK(vkCreateComputePipelines(
				device,
				pipelineCache,
				createInfoCount,
				&computePipelineCreateInfo,
				pCtx->pAllocator,
				&pCtx->pComputeShaders[2].pipeline));

	/* NOTE: Cleanup */
	vkDestroyShaderModule(device, skyDrawShader, pCtx->pAllocator);
	vkDestroyShaderModule(device, gradientColorDrawShader, pCtx->pAllocator);
	vkDestroyShaderModule(device, gradientDrawShader, pCtx->pAllocator);

	return VK_SUCCESS;
}

YND VkResult
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

YND VkResult
vkPipelineReset(VkContext *pCtx, VkDevice device, const char* pFilePath)
{
	VK_ASSERT(vkDeviceWaitIdle(device));
	vkDestroyPipeline(device, pCtx->gradientComputePipeline, pCtx->pAllocator);
	vkDestroyPipelineLayout(device, pCtx->gradientComputePipelineLayout, pCtx->pAllocator);

	VK_CHECK(vkPipelineInit(pCtx, device, pFilePath));
	YINFO("Pipeline was successfully reset.");
	return VK_SUCCESS;
}
