#include "vulkan_descriptor.h"

#include "core/darray.h"
#include "core/darray_debug.h"
#include "core/logger.h"
#include "core/myassert.h"

/**
 * pOutDescriptorSetLayout must be a valid pointer to `VkDescriptorSetLayout` structure
 *
 * @param pBindings 
 * @param device 
 * @param shaderStageFlags 
 * @param pNext 
 * @param flags 
 * @param pOutDescriptorSetLayout Valid pointer to `VkDescriptorSetLayout` structure
 * @return 
 */
YND VkResult
vkDescriptorSetLayoutCreate(
		VkDescriptorSetLayoutBinding*		pBindings,
		VkDevice							device,
		VkContext*							pCtx,
		VkShaderStageFlags					shaderStageFlags,
		void*								pNext,
		VkDescriptorSetLayoutCreateFlags	flags,
		VkDescriptorSetLayout*				pOutDescriptorSetLayout)
{
	uint64_t bindingCount = DarrayCapacity(pBindings);
	for (uint64_t i = 0; i < bindingCount; i++)
	{
		pBindings[i].stageFlags = shaderStageFlags;
	}

	VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo = {
		.sType			= VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		.pNext			= pNext,
		.pBindings		= pBindings,
		.bindingCount	= bindingCount,
		.flags			= flags,
	};

	VK_RESULT(vkCreateDescriptorSetLayout(
				device,
				&descriptorSetLayoutCreateInfo,
				pCtx->pAllocator,
				pOutDescriptorSetLayout));

	return VK_SUCCESS;
}

void
vkDescriptorSetLayoutAddBinding(VkDescriptorSetLayoutBinding* pBindings, uint32_t binding, VkDescriptorType type)
{
	VkDescriptorSetLayoutBinding setLayoutBinding = {
		.binding			= binding,
		.descriptorCount	= 1,
		.descriptorType		= type,
	};
	DarrayPush(pBindings, setLayoutBinding);
}

VkResult
vkDescriptorAllocatorPoolInit(
		VkContext*							pCtx,
		VkDescriptorPool*					pOutPool,
		VkDevice							device,
		uint32_t							maxSets,
		PoolSizeRatio*						pPoolRatios)
{
	VkDescriptorPoolSize *pPoolSizes = DarrayCreate(VkDescriptorPoolSize);
	uint64_t poolRatioCount = DarrayCapacity(pPoolRatios);

	for (uint64_t i = 0; i < poolRatioCount; i++)
	{
		VkDescriptorPoolSize new = {
			.type = pPoolRatios[i].type,
			.descriptorCount = pPoolRatios[i].ratio * maxSets,
		};
		YDEBUG("Descriptor type: %s", string_VkDescriptorType(pPoolRatios[i].type));
		DarrayPush(pPoolSizes, new);
	}

	VkDescriptorPoolCreateInfo poolCreateInfo = {
		.sType			= VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
		.flags			= 0,
		.maxSets		= maxSets,
		.poolSizeCount	= DarrayLength(pPoolSizes),
		.pPoolSizes		= pPoolSizes,
	};

	VK_RESULT(vkCreateDescriptorPool(device, &poolCreateInfo, pCtx->pAllocator, pOutPool));

	DarrayDestroy(pPoolSizes);
	return VK_SUCCESS;
}

/* NOTE: Why is this even a function ?! */
VkResult
vkDescriptorSetAllocate(
		VkDevice							device,
		VkDescriptorSetLayout				layout,
		VkDescriptorSet*					pOutDescriptorSet,
		VkDescriptorPool					pool)
{
	VkDescriptorSetAllocateInfo allocateInfo = {
		.sType				= VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		.descriptorPool		= pool,
		.descriptorSetCount	= 1,
		.pSetLayouts		= &layout,
	};
	VK_RESULT(vkAllocateDescriptorSets(device, &allocateInfo, pOutDescriptorSet));

	return VK_SUCCESS;
}

YND VkResult
vkDescriptorsInit(VkContext* pCtx, VkDevice device)
{
	uint32_t			binding				= 0;
	uint32_t			maxSets				= 10;
	void*				pNext				= VK_NULL_HANDLE;
	PoolSizeRatio*		pSizeRatios			= DarrayCreate(PoolSizeRatio);
	VkDescriptorType	descriptorType		= VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	VkShaderStageFlags	shaderStageFlags	= VK_SHADER_STAGE_COMPUTE_BIT;

	pSizeRatios[0].type		= VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	pSizeRatios[0].ratio	= 1.0f;
	pCtx->pBindings			= DarrayCreate(VkDescriptorSetLayoutBinding);

	VK_RESULT(vkDescriptorAllocatorPoolInit(pCtx, &pCtx->descriptorPool.handle, device, maxSets, pSizeRatios));

	vkDescriptorSetLayoutAddBinding(pCtx->pBindings, binding, descriptorType);

	VkDescriptorSetLayoutCreateFlags flags = 0;
	VK_CHECK(vkDescriptorSetLayoutCreate(
				pCtx->pBindings,
				device,
				pCtx,
				shaderStageFlags,
				pNext,
				flags,
				&pCtx->drawImageDescriptorSetLayout));

	VK_CHECK(vkDescriptorSetAllocate(
				device,
				pCtx->drawImageDescriptorSetLayout,
				&pCtx->drawImageDescriptorSet,
				pCtx->descriptorPool.handle));

	VkDescriptorImageInfo descriptorImageInfo = {
		.imageLayout	= VK_IMAGE_LAYOUT_GENERAL,
		.imageView		= pCtx->drawImage.image.view,
	};
	VkWriteDescriptorSet writeDescriptorSet = {
		.sType				= VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		.pNext				= VK_NULL_HANDLE,
		.dstBinding			= 0,
		.dstSet				= pCtx->drawImageDescriptorSet,
		.descriptorCount	= 1,
		.descriptorType		= VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
		.pImageInfo			= &descriptorImageInfo,
	};

	const VkCopyDescriptorSet*	pDescriptorCopies		= VK_NULL_HANDLE;
	uint32_t					descriptorCopyCount		= 0;
	uint32_t					descriptorWriteCount	= 1;
	vkUpdateDescriptorSets(device, descriptorWriteCount, &writeDescriptorSet, descriptorCopyCount, pDescriptorCopies);

	DarrayDestroy(pSizeRatios);
	DarrayDestroy(pCtx->pBindings);
	return VK_SUCCESS;
}
