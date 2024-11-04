#ifndef RENDERER_H
#define RENDERER_H

#include "mydefines.h"
#include "core/yerror.h"

typedef struct OsState OsState;

extern const char *pRendererType[];

typedef enum RendererType
{
    RENDERER_TYPE_VULKAN 	= 0x00,
    RENDERER_TYPE_OPENGL 	= 0x01,
    RENDERER_TYPE_D3D11 	= 0x02,
    RENDERER_TYPE_D3D12 	= 0x03,
    RENDERER_TYPE_METAL 	= 0x04,
	RENDERER_TYPE_SOFTWARE	= 0x05,
	MAX_RENDERER_TYPE
} RendererType;

typedef struct RendererConfig
{
	RendererType	type;
	b8				bVsync;
} RendererConfig;

typedef struct YuRenderer
{
	YuResult	(*YuDraw)(OsState* pOsState, void* pCtx);
	YuResult	(*YuResize)(OsState* pOsState, uint32_t width, uint32_t height);
	void		(*YuShutdown)(void* pCtx);
	void*		internalContext;
} YuRenderer;


/**
 * @brief	Assign pointer to functions for the right backend
 *
 * @param	`pOsState` valid pointer to OsState struct
 * @param	`pOutRenderer` valid pointer to YuRenderer struct
 * @param	`rendererConfig` The configuration for the renderer
 * @returns	`YU_SUCCESS` on success or anything else on failure;
 *			`pOutRenderer` is set to `NULL` on failure.
 */
YND YuResult RendererInit(
		OsState*							pState,
		YuRenderer**						ppOutRenderer,
		RendererConfig						rendererConfig);

YND YuResult YuDraw(
		OsState*							pOsState,
		YuRenderer*							pRenderer);

YND YuResult YuResizeWindow(
		OsState*							pOsState,
		YuRenderer*							pRenderer,
		uint32_t							width,
		uint32_t							height);

void		 YuShutdown(
		YuRenderer*							pRenderer);

#endif //RENDERER_H

