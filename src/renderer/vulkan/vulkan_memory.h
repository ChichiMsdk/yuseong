#ifndef VULKAN_MEMORY_H
#define VULKAN_MEMORY_H

#include "yvulkan.h"
#include "cglm/vec3.h"
#include "cglm/vec4.h"


/* WARN: This needs to be very compact in the future */
typedef struct Vertex
{
	vec3			position;
	f32				uvX;
	vec3			normal;
	f32				uvY;
	vec4			color;
	// alignas(8) f32	color[4];
/*
 * FIXME: This doesnt work for now, alignment issues
 * 	vec4	color;
 */

} Vertex;

YND VkResult vkMeshUpload(
		VulkanDevice						device,
		VkAllocationCallbacks*				pAllocator,
		uint32_t*							pIndices,
		Vertex*								pVertices,
		GpuMeshBuffers*						pOutGpuMeshBuffers);

YND VkResult vkMappedBufferGetData(
		VkDevice							device,
		VulkanBuffer*						pBuffer,
		void**								ppOutData);

YND VkResult MemoryTypeFindIndex(
		VkPhysicalDevice					physicalDevice,
		uint32_t							typeFilter,
		uint32_t							propertyFlags,
		int32_t*							pOutIndex);

YND VkResult vkBufferCreate(
		VulkanDevice						device,
		VkAllocationCallbacks*				pAllocator,
		uint64_t							allocSize,
		VkBufferUsageFlags					usageFlags,
		uint32_t							propertyFlags,
		VulkanBuffer*						pOutBuffer);

#endif //VULKAN_MEMORY_H
