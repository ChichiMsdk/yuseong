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
    RENDERER_TYPE_VULKAN = 0x12,
    RENDERER_TYPE_OPENGL = 0x13,
    RENDERER_TYPE_DIRECTX = 0x14,
    RENDERER_TYPE_METAL = 0x15,
	RENDERER_TYPE_SOFTWARE = 0x16,
	MAX_RENDERER_TYPE
}RendererType;

typedef struct RendererConfig
{
	RendererType type;
}RendererConfig;

typedef struct YuRenderer
{
	YuResult (*YuDraw)(void);
	void (*YuShutdown)(void);
}YuRenderer;


YND YuRenderer *RendererInit(
		OsState*							pState,
		RendererConfig						rendererConfig);

void YuShutdown(
		YuRenderer*							pRenderer);

YND YuResult YuDraw(
		YuRenderer*							pRenderer);


#endif //RENDERER_H

