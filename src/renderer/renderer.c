#include "rendererimpl.h"

const char *pRendererType[] = { "Vulkan", "OpenGL", "D3D11", "D3D12", "Metal", "Software" };

YND YuRenderer*
RendererInit(OsState* pOsState, RendererConfig rendererConfig)
{
	YuRenderer *pRenderer = NULL;
	switch (rendererConfig.type)
	{
		case RENDERER_TYPE_VULKAN:
			if (vkErrorToYuseong(vkInit(pOsState)) != YU_SUCCESS)
				return pRenderer;
			pRenderer = yAlloc(sizeof(YuRenderer), MEMORY_TAG_RENDERER);
			pRenderer->YuDraw = vkDraw;
			pRenderer->YuShutdown = vkShutdown;
			pRenderer->YuResize = vkResize;
			goto finish;
		case RENDERER_TYPE_OPENGL:
			if (glErrorToYuseong(glInit(pOsState)) != YU_SUCCESS)
				return pRenderer;
			pRenderer = yAlloc(sizeof(YuRenderer), MEMORY_TAG_RENDERER);
			pRenderer->YuDraw = glDraw;
			pRenderer->YuShutdown = glShutdown;
			pRenderer->YuResize = glResize;
			goto finish;
		case RENDERER_TYPE_D3D11:
			if (D11ErrorToYuseong(D11Init(pOsState, rendererConfig.bVsync))!= YU_SUCCESS)
				return pRenderer;
			pRenderer = yAlloc(sizeof(YuRenderer), MEMORY_TAG_RENDERER);
			pRenderer->YuDraw = D11Draw;
			pRenderer->YuShutdown = D11Shutdown;
			pRenderer->YuResize = D11Resize;
			goto finish;
		case RENDERER_TYPE_D3D12:
		case RENDERER_TYPE_METAL:
		case RENDERER_TYPE_SOFTWARE:
			YERROR("Renderer type %s has yet to be implemented", pRendererType[rendererConfig.type]);
			return pRenderer;
		default:
			YFATAL("No renderer found.");
			return pRenderer;
	}
finish:
	return pRenderer;
}

YND YuResult
YuResizeWindow(OsState* pOsState, YuRenderer* pRenderer, uint32_t width, uint32_t height)
{
	return pRenderer->YuResize(pOsState, width, height);
}

void
YuShutdown(YuRenderer *pRenderer)
{
	pRenderer->YuShutdown();
	yFree(pRenderer, 1, MEMORY_TAG_RENDERER);
	YDEBUG("Renderer shutdown.")
}

YuResult
YuDraw(OsState* pOsState, YuRenderer *pRenderer)
{
	return pRenderer->YuDraw(pOsState);
}

/******************************************************************************************************************/
/***************************************** Renderer Specific functions ********************************************/
/******************************************************************************************************************/

YND YuResult
vkErrorToYuseong(VkResult result)
{
	switch (result)
	{
		case VK_SUCCESS:
			return YU_SUCCESS;
		default:
			return YU_FAILURE;
	}
}

YND YuResult
D11ErrorToYuseong(int result)
{
	switch (result)
	{
		case TRUE:
			return YU_SUCCESS;
		default:
			return YU_FAILURE;
	}
}

YND YuResult
glErrorToYuseong(GlResult result)
{
	switch (result)
	{
		case TRUE:
			return YU_SUCCESS;
		default:
			return YU_FAILURE;
	}
}

YND YuResult
vkDraw(YMB OsState* pOsState)
{
	return vkErrorToYuseong(vkDrawImpl());
}

YND YuResult
vkResize(YMB OsState* pOsState, YMB uint32_t width,YMB  uint32_t height)
{
	return YU_SUCCESS;
}

YND YuResult
glDraw(OsState* pOsState)
{
	return glErrorToYuseong(glDrawImpl(pOsState));
}

YND YuResult
glResize(OsState* pOsState, uint32_t width, uint32_t height)
{
	return glErrorToYuseong(glResizeImpl(pOsState, width, height));
}

YND YuResult
D11Draw(OsState* pOsState)
{
	return D11ErrorToYuseong(D11DrawImpl(pOsState));
}

YND YuResult
D11Resize(OsState* pOsState, uint32_t width, uint32_t height)
{
	return D11ErrorToYuseong(D11ResizeImpl(pOsState, width, height));
}
