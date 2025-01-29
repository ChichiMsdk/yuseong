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

#if defined (PLATFORM_WINDOWS) && defined (YDIRECTX)
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
#endif //PLATFORM_WINDOWS
