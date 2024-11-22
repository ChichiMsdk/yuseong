#include "vulkan_memory.h"
#include "vulkan_command.h"

#include <string.h>

#include "core/darray.h"

YND VkResult
vkBufferCreate(
		VulkanDevice						device,
		VkAllocationCallbacks*				pAllocator,
		uint64_t							allocSize,
		VkBufferUsageFlags					usageFlags,
		uint32_t							propertyFlags,
		VulkanBuffer*						pOutBuffer)
{
	/* NOTE: Create buffer */
	VkBufferCreateInfo createInfo = {
		.sType	= VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		.usage	= usageFlags,
		.size	= allocSize,
	};
	VK_CHECK(vkCreateBuffer(device.handle, &createInfo, pAllocator, &pOutBuffer->handle));

	/* NOTE: Get memory requirements before allocating */
	VkMemoryRequirements memoryRequirements;
	vkGetBufferMemoryRequirements(device.handle, pOutBuffer->handle, &memoryRequirements);

	int32_t		memoryTypeIndex;

	VK_RESULT(MemoryTypeFindIndex(
				device.physicalDevice,
				memoryRequirements.memoryTypeBits,
				propertyFlags,
				&memoryTypeIndex));

	VkMemoryAllocateFlagsInfo	allocateFlagsInfo = {
		.sType	= VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO,
		.flags	= VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT,
	};

	VkMemoryAllocateInfo memoryAllocateInfo = {
		.sType				= VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		.allocationSize		= memoryRequirements.size,
		.memoryTypeIndex	= (uint32_t) memoryTypeIndex,
		.pNext				= &allocateFlagsInfo,
	};

	/* NOTE: Allocate the memory */
	VkDeviceMemory	bufferMemory;
	VK_CHECK(vkAllocateMemory(device.handle, &memoryAllocateInfo, pAllocator, &bufferMemory));

	/* TODO: Check this !! */
	VkDeviceSize memoryOffset = 0;
	VK_CHECK(vkBindBufferMemory(device.handle, pOutBuffer->handle, bufferMemory, memoryOffset));

	pOutBuffer->memory	= bufferMemory;
	pOutBuffer->offset	= memoryOffset;
	pOutBuffer->size	= memoryRequirements.size;
	pOutBuffer->flags	= 0;

    /*
	 * void*				pData;
	 * VkMemoryMapFlags	memoryMapFlags	= 0;
	 * VK_CHECK(vkMapMemory(device.handle, bufferMemory, memoryOffset, allocSize, memoryMapFlags, &pData));
     */
	return VK_SUCCESS;
}

struct ContextTemp
{
	const uint64_t	vertexBufferSize;
	const uint64_t	indexBufferSize;
	VulkanBuffer*	pStaging;
	GpuMeshBuffers*	pNewSurface;
};

YND VkResult
vkMeshUpload(
		VulkanDevice						device,
		VkAllocationCallbacks*				pAllocator,
		uint32_t*							pIndices,
		Vertex*								pVertices,
		GpuMeshBuffers*						pOutGpuMeshBuffers)
{
	const uint64_t vertexBufferSize	= DarrayCapacity(pVertices) * sizeof(Vertex);
	const uint64_t indexBufferSize	= DarrayCapacity(pIndices) * sizeof(uint32_t);

	GpuMeshBuffers	newSurface;
	/* NOTE: create vertex buffer */
	VK_RESULT(vkBufferCreate(
				device,
				pAllocator,
				vertexBufferSize, 
				VK_BUFFER_USAGE_STORAGE_BUFFER_BIT 
				| VK_BUFFER_USAGE_TRANSFER_DST_BIT 
				| VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
				&newSurface.vertexBuffer
				));

	/* NOTE: Find the adress of the vertex buffer */
	VkBufferDeviceAddressInfo deviceAdressInfo = { 
		.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
		.buffer = newSurface.vertexBuffer.handle,
	};
	newSurface.vertexBufferAddress = vkGetBufferDeviceAddress(device.handle, &deviceAdressInfo);

	/* NOTE: Create index buffer */
	VK_RESULT(vkBufferCreate(
				device,
				pAllocator,
				indexBufferSize, 
				VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, 

				/* NOTE: GPU only ! */
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
				&newSurface.indexBuffer));

	VulkanBuffer	staging;

	VK_RESULT(vkBufferCreate(
				device,
				pAllocator,
				indexBufferSize + vertexBufferSize, 
				VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 

				/* NOTE: CPU only ! */
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,

				&staging));

	/* NOTE: Map memory to copy form CPU to GPU */
	void*	pData;
	VK_CHECK(vkMapMemory(device.handle, staging.memory, staging.offset, staging.size, staging.flags, &pData));

	/* NOTE: Copy vertex buffer */
	memcpy(pData, pVertices, vertexBufferSize);

	/* NOTE: Copy index buffer */
	memcpy(pData + vertexBufferSize, pIndices, indexBufferSize);

	/* NOTE: Prepare to submit commands */
	void Submit(void* pCtx, VulkanCommandBuffer cmd);
	struct ContextTemp ctx = {
		.vertexBufferSize = vertexBufferSize,
		.indexBufferSize = indexBufferSize,
		.pStaging = &staging,
		.pNewSurface = &newSurface,
	};

	/* NOTE: Submit immediate command with pfn */
	VK_RESULT(vkCommandSubmitImmediate(Submit, device, device.immediateSubmit, &ctx));

	/* NOTE: should this be done ? */
	/* vkUnmapMemory(device.handle, staging.memory); */

	vkDestroyBuffer(device.handle, staging.handle, pAllocator);
	vkFreeMemory(device.handle, staging.memory, pAllocator);

	(*pOutGpuMeshBuffers).vertexBufferAddress = newSurface.vertexBufferAddress;
	(*pOutGpuMeshBuffers).indexBuffer = newSurface.indexBuffer;
	(*pOutGpuMeshBuffers).vertexBuffer = newSurface.vertexBuffer;
	return VK_SUCCESS;
}

void
Submit(void* pCtx, VulkanCommandBuffer command)
{
	struct ContextTemp*	pContext			= (struct ContextTemp*)pCtx;
	uint32_t			vertexBufferSize	= pContext->vertexBufferSize;
	uint32_t			indexBufferSize 	= pContext->indexBufferSize; 
	GpuMeshBuffers*		newSurface			= pContext->pNewSurface;
	VulkanBuffer*		staging				= pContext->pStaging;

	VkBufferCopy 	vertexCopy = { 
		.dstOffset = 0,
		.srcOffset = 0,
		.size = vertexBufferSize,
	};

	uint32_t	regionCount	= 1;
	vkCmdCopyBuffer(command.handle, staging->handle, newSurface->vertexBuffer.handle, regionCount, &vertexCopy);

	VkBufferCopy indexCopy = {
		.dstOffset = 0,
		.srcOffset = vertexBufferSize,
		.size = indexBufferSize,
	};

	vkCmdCopyBuffer(command.handle, staging->handle, newSurface->indexBuffer.handle, regionCount, &indexCopy);
}


YND VkResult
vkMappedBufferGetData(VkDevice device, VulkanBuffer* pBuffer, void** ppOutData)
{
	VK_CHECK(vkMapMemory(device, pBuffer->memory, pBuffer->offset, pBuffer->size, pBuffer->flags, ppOutData));
	return VK_SUCCESS;
}

YND VkResult
MemoryTypeFindIndex(VkPhysicalDevice physicalDevice, uint32_t typeFilter, uint32_t propertyFlags, int32_t* pOutIndex)
{
    VkPhysicalDeviceMemoryProperties memoryProperties;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties);
	uint32_t	i =0;

    for (i = 0; i < memoryProperties.memoryTypeCount; ++i)
	{
        // Check each memory type to see if its bit is set to 1.
        if (typeFilter & (1 << i) && (memoryProperties.memoryTypes[i].propertyFlags & propertyFlags) == propertyFlags)
		{
			*pOutIndex = i;
            return VK_SUCCESS;
        }
    }
    YWARN("Unable to find suitable memory type.");
    return VK_ERROR_UNKNOWN;
}
