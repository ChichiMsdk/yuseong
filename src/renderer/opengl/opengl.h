#define BACKEND_OPENGL

#ifdef BACKEND_OPENGL
#ifndef YOPENGL_H
#define YOPENGL_H

#include "mydefines.h"
#include "renderer/renderer_defines.h"

typedef struct OsState OsState;

typedef b8 GlResult;

GlResult glInit(
		OsState*							pOsState);

YND b8 glResizeImpl(
		OsState*							pOsState,
		uint32_t							width,
		uint32_t							height);

YND b8 glDrawImpl(
		OsState*							pOsState);

void glShutdown(
		void*							pCtx);

#endif // OPENGL_H
#endif // BACKEND_OPENGL
