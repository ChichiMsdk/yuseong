#include "renderer.h"
#include "core/logger.h"
#include "core/ymemory.h"

#include "vulkan/yvulkan.h"
#include "opengl/opengl.h"
#include "directx/directx.h"
#include "metal/metal.h"

YND YuResult VulkanErrorToYuseong(VkResult result);

YND YuResult
vkDraw(void)
{
	return VulkanErrorToYuseong(vkDrawImpl());
}

YND YuRenderer*
RendererInit(OsState* pState, RendererConfig rendererConfig)
{
	(void) pState;
	YuRenderer *pRenderer = yAlloc(sizeof(YuRenderer), MEMORY_TAG_RENDERER);
	switch (rendererConfig.type)
	{
		case RENDERER_TYPE_VULKAN:
			if (VulkanErrorToYuseong(vkInit(pState)) != YU_SUCCESS)
				return YU_NULL;
			pRenderer->YuDraw = vkDraw;
			pRenderer->YuShutdown = vkShutdown;
			break;
		default:
			YFATAL("No renderer found.");
			return YU_NULL;
	}
	return pRenderer;
}

void
YuShutdown(YuRenderer *pRenderer)
{
	pRenderer->YuShutdown();
	yFree(pRenderer, 1, MEMORY_TAG_RENDERER);
	YDEBUG("Renderer shutdown.")
}

YuResult
YuDraw(YuRenderer *pRenderer)
{
	return pRenderer->YuDraw();
}

YND YuResult
VulkanErrorToYuseong(VkResult result)
{
	switch (result)
	{
		case VK_SUCCESS:
			return YU_SUCCESS;
		default:
			return YU_FAILURE;
	}
}
