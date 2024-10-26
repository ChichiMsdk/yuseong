#ifndef YOPENGL_H
#define YOPENGL_H

#include "mydefines.h"


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

void glShutdown(void);

#endif // OPENGL_H
