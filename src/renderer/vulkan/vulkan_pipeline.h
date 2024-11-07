#ifndef VULKAN_PIPELINE_H
#define VULKAN_PIPELINE_H

#include "renderer/vulkan/yvulkan.h"
#include "core/vec4.h"
#include "core/yerror.h"

YND VkResult vkLoadShaderModule(
		VkContext*							pCtx,
		const char*							pFilePath,
		VkDevice							device,
		VkShaderModule*						pOutShaderModule);

YND VkResult vkComputePipelineInit(
		VkContext*							pCtx,
		VkDevice							device,
		const char**						ppFilePath);

YND VkResult vkPipelineInit(
		VkContext*							pCtx,
		VkDevice							device,
		const char*							pFilePath);

YND VkResult vkPipelineReset(
		VkContext*							pCtx,
		VkDevice							device,
		const char*							pFilePath);

#endif //VULKAN_PIPELINE_H
