#include "rendererimpl.h"

const char *pRendererType[] = { "Vulkan", "OpenGL", "D3D11", "D3D12", "Metal", "Software" };

/**
 * @brief	Allocates pOutREnderer Assign pointer to functions for the right backend
 *
 * @param	`pOsState` valid pointer to OsState struct
 * @param	`pOutRenderer`
 * @param	`rendererConfig` The configuration for the renderer
 * @returns	`YU_SUCCESS` on success or anything else on failure;
 *			`pOutRenderer` is set to `NULL` on failure.
 */
YND YuResult
RendererInit(OsState* pOsState, YuRenderer** ppOutRenderer, RendererConfig rendererConfig)
{
	(*ppOutRenderer) = NULL;
	switch (rendererConfig.type)
	{
		case RENDERER_TYPE_VULKAN:
			if (vkErrorToYuseong(vkInit(pOsState)) != YU_SUCCESS)
				goto error;
			(*ppOutRenderer) = yAlloc(sizeof(YuRenderer), MEMORY_TAG_RENDERER);
			(*ppOutRenderer)->YuDraw = vkDraw;
			(*ppOutRenderer)->YuShutdown = vkShutdown;
			(*ppOutRenderer)->YuResize = vkResize;
			goto finish;
		case RENDERER_TYPE_OPENGL:
			if (glErrorToYuseong(glInit(pOsState)) != YU_SUCCESS)
				goto error;
			(*ppOutRenderer) = yAlloc(sizeof(YuRenderer), MEMORY_TAG_RENDERER);
			(*ppOutRenderer)->YuDraw = glDraw;
			(*ppOutRenderer)->YuShutdown = glShutdown;
			(*ppOutRenderer)->YuResize = glResize;
			goto finish;
		case RENDERER_TYPE_D3D11:
#ifdef PLATFORM_WINDOWS
			if (D11ErrorToYuseong(D11Init(pOsState, rendererConfig.bVsync))!= YU_SUCCESS)
				goto error;
			(*ppOutRenderer) = yAlloc(sizeof(YuRenderer), MEMORY_TAG_RENDERER);
			(*ppOutRenderer)->YuDraw = D11Draw;
			(*ppOutRenderer)->YuShutdown = D11Shutdown;
			(*ppOutRenderer)->YuResize = D11Resize;
			goto finish;
#else
			YFATAL("Renderer type %s is not available on this platform", pRendererType[rendererConfig.type]);
			goto finish;
#endif // PLATFORM_WINDOWS
		case RENDERER_TYPE_D3D12:
#ifdef PLATFORM_WINDOWS
#else
			YFATAL("Renderer type %s is not available on this platform", pRendererType[rendererConfig.type]);
			goto finish;
#endif // PLATFORM_WINDOWS
		case RENDERER_TYPE_METAL:
		case RENDERER_TYPE_SOFTWARE:
			YERROR("Renderer type %s has yet to be implemented", pRendererType[rendererConfig.type]);
			goto error;
		default:
			YFATAL("No renderer found.");
			goto error;
	}
finish:
	return YU_SUCCESS;
error:
	return YU_FAILURE;
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

/****************************************************************************/
/******************************* Vulkan *************************************/
/****************************************************************************/
YND YuResult
vkDraw(YMB OsState* pOsState)
{
	return vkErrorToYuseong(vkDrawImpl());
}

YND YuResult
vkResize(YMB OsState* pOsState, YMB uint32_t width, YMB uint32_t height)
{
	return YU_SUCCESS;
}

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

/****************************************************************************/
/******************************* DirectX ************************************/
/****************************************************************************/
#ifdef PLATFORM_WINDOWS
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
#endif //PLATFORM_WINDOWS

/****************************************************************************/
/******************************* OpenGL *************************************/
/****************************************************************************/
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
