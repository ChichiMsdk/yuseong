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

VkResult
vkLoadShaderModule(VkContext *pCtx, const char* pFilePath, VkDevice device, VkShaderModule* pOutShaderModule)
{
	FILE* pStream;
	int errcode = OsFopen(&pStream, pFilePath, "rb");
	if (!errcode)
	{
		char buffer[1024];
		strerror_s(buffer, 1024, errno);
		YERROR("%s in %s:%d",buffer, __FILE__, __LINE__);
		return VK_ERROR_UNKNOWN;
	}
	fseek(pStream, 0, SEEK_END);
	size_t fileSize = ftell(pStream);
	YDEBUG("file size: %llu", fileSize);

	uint32_t* pBuffer = DarrayReserve(uint32_t, fileSize);
	fseek(pStream, 0, SEEK_SET);
	OsFread(pBuffer, pStream, 1, 1, fileSize);
	fclose(pStream);
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

#endif //VULKAN_PIPELINE_H
