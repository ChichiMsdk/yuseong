#ifndef RENDERER_H
#define RENDERER_H

#include "mydefines.h"

typedef struct OsState OsState;

typedef enum YuResult
{
	YU_SUCCESS = 0,
	YU_FAILURE = 1,
}YuResult;

typedef enum RendererType
{
    RENDERER_TYPE_VULKAN = 0x00,
    RENDERER_TYPE_OPENGL = 0x01,
    RENDERER_TYPE_DIRECTX = 0x02,
    RENDERER_TYPE_METAL = 0x03,
	RENDERER_TYPE_SOFTWARE = 0x04,
	MAX_RENDERER_TYPE
}RendererType;

extern const char *pRendererType[];

typedef struct RendererConfig
{
	RendererType type;
}RendererConfig;

typedef struct YuRenderer
{
	YuResult (*YuDraw)(OsState *pOsState);
	void (*YuShutdown)(void);
	YuResult (*YuResize)(OsState *pOsState, uint32_t width, uint32_t height);
}YuRenderer;


YND YuRenderer *RendererInit(
		OsState*							pState,
		RendererConfig						rendererConfig);

void YuShutdown(
		YuRenderer*							pRenderer);

YND YuResult YuDraw(
		OsState*							pOsState,
		YuRenderer*							pRenderer);

YND YuResult YuResizeWindow(
		OsState*							pOsState,
		YuRenderer*							pRenderer,
		uint32_t							width,
		uint32_t							height);

#endif //RENDERER_H

