#include "vulkan_image.h"

void
vkImageTransition(VulkanCommandBuffer* pCmd, VkImage pImg, VkImageLayout currentLayout, VkImageLayout newLayout)
{
	/* NOTE: This is vulkan 1.3 only use renderpass otherwise */
    VkImageMemoryBarrier2 imageBarrier = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
		.srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
		.srcAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT,
		/* 
		 * WARN: This is inefficient, if more than a few transitions, change StageMasks and use more accurate ones
		 *
		 * Ex: https://github.com/KhronosGroup/Vulkan-Docs/wiki/Synchronization-Examples
		 */
		.dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
		.dstAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT | VK_ACCESS_2_MEMORY_READ_BIT,
		.oldLayout = currentLayout,
		.newLayout = newLayout,
		.image = pImg,
	};

    VkImageAspectFlags aspectMask = 
		(newLayout == VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL) ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;

    imageBarrier.subresourceRange = (VkImageSubresourceRange){ 
		.aspectMask = aspectMask,
		.baseMipLevel = 0,
		.levelCount = VK_REMAINING_MIP_LEVELS,
		.baseArrayLayer = 0,
		.layerCount = VK_REMAINING_ARRAY_LAYERS,
	};

    VkDependencyInfo depInfo = {
		.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
		.imageMemoryBarrierCount = 1,
		.pImageMemoryBarriers = &imageBarrier,
	};

    vkCmdPipelineBarrier2(pCmd->handle, &depInfo);
}
