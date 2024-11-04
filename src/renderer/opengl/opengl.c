#include "opengl.h"

#include "os.h"
#include "core/logger.h"
#include "core/assert.h"

#ifdef PLATFORM_WINDOWS
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#include "glad/glad.h"

#ifdef YGLFW3
#include <GLFW/glfw3.h>
#endif

typedef struct GLContext
{
	uint32_t frameBufferWidth;
	uint32_t frameBufferHeight;
	uint32_t currentFrame;
	uint64_t nbFrames;
}GLContext;

static GLContext gGLContext;

GlResult
glInit(OsState *pOsState)
{
	YDEBUG("Creating OpenGL context..");
	b8 errCode = OsCreateGlContext(pOsState);
	if (errCode == FALSE)
	{
		YFATAL("Error on %s:%d: %s", __FILE__, __LINE__);
		return FALSE;
	}
	YDEBUG("Retrieving OpenGL functions..");
	int value = gladLoadGLLoader((GLADloadproc)OsGetGLFuncAddress);
	if (!value)
	{
		YFATAL("Error on %s:%d: %s", __FILE__, __LINE__);
		return FALSE;
	}
	YDEBUG("OpenGL version %s", glGetString(GL_VERSION));
	OsFramebufferGetDimensions(pOsState, &gGLContext.frameBufferWidth, &gGLContext.frameBufferHeight);
	uint32_t width = gGLContext.frameBufferWidth;
	uint32_t height = gGLContext.frameBufferHeight;
	glViewport(0, 0, width, height);
	YDEBUG("OpenGL renderer initialized.");
	return TRUE;
}

YND b8
glResizeImpl(YMB OsState *pOsState, uint32_t width, uint32_t height)
{
	gGLContext.frameBufferWidth = width;
	gGLContext.frameBufferHeight = height;
	glViewport(0, 0, width, height);
	YDEBUG("Resizing window -> w: %u h: %u", width, height);
	return OsSwapBuffers(pOsState);
}

YND b8
glDrawImpl(YMB OsState *pOsState)
{
	glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	return OsSwapBuffers(pOsState);
}

void
glShutdown(YMB void* pCtx)
{
}
