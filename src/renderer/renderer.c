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
	(*ppOutRenderer) = yAlloc(sizeof(YuRenderer), MEMORY_TAG_RENDERER);
	switch (rendererConfig.type)
	{
		case RENDERER_TYPE_VULKAN:
			if (vkErrorToYuseong(vkInit(pOsState, &(*ppOutRenderer)->internalContext)) != YU_SUCCESS)
				goto error;
			(*ppOutRenderer)->YuDraw = vkDraw;
			(*ppOutRenderer)->YuShutdown = vkShutdown;
			(*ppOutRenderer)->YuResize = vkResize;
			goto finish;

		case RENDERER_TYPE_OPENGL:
#ifdef YOPENGL
			if (glErrorToYuseong(glInit(pOsState)) != YU_SUCCESS)
				goto error;
			(*ppOutRenderer)->YuDraw = glDraw;
			(*ppOutRenderer)->YuShutdown = glShutdown;
			(*ppOutRenderer)->YuResize = glResize;
			goto finish;
#else
			YFATAL("Renderer type %s is not available for this build", pRendererType[rendererConfig.type]);
#endif
		case RENDERER_TYPE_D3D11:
#if defined (PLATFORM_WINDOWS) && defined (YDIRECTX)
			if (D11ErrorToYuseong(D11Init(pOsState, rendererConfig.bVsync))!= YU_SUCCESS)
				goto error;
			(*ppOutRenderer)->YuDraw = D11Draw;
			(*ppOutRenderer)->YuShutdown = D11Shutdown;
			(*ppOutRenderer)->YuResize = D11Resize;
			goto finish;

		case RENDERER_TYPE_D3D12:
			YERROR("Renderer type %s has yet to be implemented", pRendererType[rendererConfig.type]);
			goto error;
#else
			YFATAL("Renderer type %s is not available for this build", pRendererType[rendererConfig.type]);
			goto finish;
#endif
		case RENDERER_TYPE_METAL:
#ifdef PLATFORM_APPLE
			YERROR("Renderer type %s has yet to be implemented", pRendererType[rendererConfig.type]);
			goto error;
#else
			YFATAL("Renderer type %s is not available for this build", pRendererType[rendererConfig.type]);
			goto finish;
#endif
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
	yFree((*ppOutRenderer), 1, MEMORY_TAG_RENDERER);
	(*ppOutRenderer) = NULL;
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
	pRenderer->YuShutdown(pRenderer->internalContext);
	yFree(pRenderer, 1, MEMORY_TAG_RENDERER);
	YDEBUG("Renderer shutdown.")
}

YuResult
YuDraw(OsState* pOsState, YuRenderer *pRenderer)
{
	return pRenderer->YuDraw(pOsState, pRenderer->internalContext);
}

/****************************************************************************/
/******************************* Vulkan *************************************/
/****************************************************************************/
YND YuResult
vkDraw(YMB OsState* pOsState, void *pCtx)
{
	return vkErrorToYuseong(vkDrawImpl(pCtx));
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
#if defined (PLATFORM_WINDOWS) && defined (YDIRECTX)
YND YuResult
D11Draw(OsState* pOsState, void* pCtx)
{
	return D11ErrorToYuseong(D11DrawImpl(pOsState, pCtx));
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
#ifdef YOPENGL
YND YuResult
glDraw(OsState* pOsState, YMB void* pCtx)
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
#endif // YOPENGL
