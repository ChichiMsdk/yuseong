#ifndef VULKAN_DESCRIPTOR_H
#define VULKAN_DESCRIPTOR_H

#include "renderer/vulkan/yvulkan.h"

typedef struct DescriptorLayout
{
} DescriptorLayout;

YND VkResult
vkDescriptorSetLayoutCreate(
		VkDescriptorSetLayoutBinding*		pBindings,
		VkDevice							device,
		VkContext*							pCtx,
		VkShaderStageFlags					shaderStageFlags,
		void*								pNext,
		VkDescriptorSetLayoutCreateFlags	flags,
		VkDescriptorSetLayout*				pOutDescriptorSetLayout);

void vkDescriptorSetLayoutAddBinding(
		VkDescriptorSetLayoutBinding*		pBindings,
		uint32_t							binding,
		VkDescriptorType					type);

YND VkResult vkDescriptorAllocatorPoolInit(
		VkContext*							pCtx,
		VkDescriptorPool*					pOutPool,
		VkDevice							device,
		uint32_t							maxSets,
		PoolSizeRatio*						pPoolRatios);

YND VkResult vkDescriptorsInit(
		VkContext*							pCtx,
		VkDevice							device);

YND VkResult vkDescriptorSetAllocate(
		VkDevice							device,
		VkDescriptorSetLayout				layout,
		VkDescriptorSet						*pOutDescriptorSet,
		VkDescriptorPool					pool);

#endif // VULKAN_DESCRIPTOR_H
