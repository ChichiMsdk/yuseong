#include "vulkan_image.h"

void
vkImageTransition(VulkanCommandBuffer* pCmd, VkImage pImg, VkImageLayout currentLayout, VkImageLayout newLayout)
{
	/* NOTE: This is vulkan 1.3 only use renderpass otherwise */
    VkImageMemoryBarrier2 imageBarrier = {
		.sType			= VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
		.srcStageMask	= VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
		.srcAccessMask	= VK_ACCESS_2_MEMORY_WRITE_BIT,
		/* 
		 * WARN: This is inefficient, if more than a few transitions
		 * change StageMasks and use more accurate ones
		 * Ex: https://github.com/KhronosGroup/Vulkan-Docs/wiki/Synchronization-Examples
		 */
		.dstStageMask	= VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
		.dstAccessMask	= VK_ACCESS_2_MEMORY_WRITE_BIT | VK_ACCESS_2_MEMORY_READ_BIT,
		.oldLayout		= currentLayout,
		.newLayout		= newLayout,
		.image			= pImg,
	};

    VkImageAspectFlags aspectMask = 
		(newLayout == VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL) ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;

    imageBarrier.subresourceRange = (VkImageSubresourceRange){ 
		.aspectMask		= aspectMask,
		.baseMipLevel	= 0,
		.levelCount		= VK_REMAINING_MIP_LEVELS,
		.baseArrayLayer	= 0,
		.layerCount		= VK_REMAINING_ARRAY_LAYERS,
	};

    VkDependencyInfo dependencyInfo = {
		.sType						= VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
		.imageMemoryBarrierCount	= 1,
		.pImageMemoryBarriers		= &imageBarrier,
	};

    vkCmdPipelineBarrier2(pCmd->handle, &dependencyInfo);
}
/**
 * VkCmdCopyImage or VkCmdBlitImage. CopyImage is faster, but its much more restricted,
 * for example the resolution on both images must match. Meanwhile, blit image lets you copy images of different
 * formats and different sizes into one another
 */
void
vkImageCopy(VkCommandBuffer commandBuffer, VkImage srcImage, VkImage dstImage, VkExtent2D srcSize, VkExtent2D dstSize)
{
	VkImageBlit2 blitRegion = { 
		.sType							= VK_STRUCTURE_TYPE_IMAGE_BLIT_2,
		.srcOffsets[1].x				= srcSize.width,
		.srcOffsets[1].y				= srcSize.height,
		.srcOffsets[1].z				= 1,

		.dstOffsets[1].x				= dstSize.width,
		.dstOffsets[1].y				= dstSize.height,
		.dstOffsets[1].z				= 1,

		.srcSubresource.aspectMask		= VK_IMAGE_ASPECT_COLOR_BIT,
		.srcSubresource.baseArrayLayer	= 0,
		.srcSubresource.layerCount		= 1,
		.srcSubresource.mipLevel		= 0,

		.dstSubresource.aspectMask		= VK_IMAGE_ASPECT_COLOR_BIT,
		.dstSubresource.baseArrayLayer	= 0,
		.dstSubresource.layerCount		= 1,
		.dstSubresource.mipLevel		= 0,
	};

	VkBlitImageInfo2 blitImageInfo = {
		.sType			= VK_STRUCTURE_TYPE_BLIT_IMAGE_INFO_2,
		.dstImage		= dstImage,
		.dstImageLayout	= VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		.srcImage		= srcImage,
		.srcImageLayout	= VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
		.filter			= VK_FILTER_LINEAR,
		.regionCount	= 1,
		.pRegions		= &blitRegion,
	};

	vkCmdBlitImage2(commandBuffer, &blitImageInfo);
}
