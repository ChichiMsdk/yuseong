#include "os.h"

#ifdef YGLFW3

extern b8 gRunning;

#include "core/event.h"
#include "core/input.h"
#include "core/logger.h"
#include "glfw_callback.h"

#include <vulkan/vulkan.h>

#include <math.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <string.h>

#include "renderer/renderer.h"

typedef struct InternalState
{
	VkSurfaceKHR	surface;

	GLFWwindow*		pWindow;
	uint32_t		windowWidth;
	uint32_t		windowHeight;
	b8				bGlfwInit;
}InternalState;

YND b8 
OsInit(OsState *pOsState, AppConfig appConfig)
{
	/*
	 * NOTE: Thorough error handling
	 */
	pOsState->pInternalState = calloc(1, sizeof(InternalState));
	InternalState *pState = (InternalState *)pOsState->pInternalState;
	glfwSetErrorCallback(_ErrorCallback);
	if (!glfwInit())
	{
		const char *pDesc; 
		glfwGetError(&pDesc);
		YFATAL("Error glfwInit() in %s at %d: %s", __FILE__, __LINE__, pDesc);
		return FALSE;
	}
	if (!glfwVulkanSupported())
	{
		const char *pDesc; 
		glfwGetError(&pDesc);
		YFATAL("Error vulkan not supported in %s at %d: %s", __FILE__, __LINE__, pDesc);
		return FALSE;
	}
	switch (appConfig.pRendererConfig->type)
	{
		case RENDERER_TYPE_VULKAN:
			glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
			YDEBUG("GLFW_VULKAN_API chosen");
			break;
		case RENDERER_TYPE_OPENGL:
			/* glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); */
			YDEBUG("GLFW_OPENGL_API chosen");
		default:
			break;
	}
	int minor, patch;
	glfwGetVersion(NULL, &minor, &patch);
	if (minor > 3 || (minor == 3 && patch >= 9))
		glfwInitHint(0x00053001 /*GLFW_WAYLAND_LIBDECOR*/, 0x00038002 /*GLFW_WAYLAND_DISABLE_LIBDECOR*/);

	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
	/* GLFWmonitor *pMonitor = glfwGetPrimaryMonitor(); */
	GLFWmonitor *pMonitor = NULL;
	pState->pWindow = glfwCreateWindow(appConfig.w, appConfig.h, appConfig.pAppName, pMonitor, NULL);
	if (!pState->pWindow)
	{
		const char *pDesc; 
		glfwGetError(&pDesc);
		YFATAL("Error window creation in %s at %d: %s", __FILE__, __LINE__, pDesc);
		return FALSE;
	}
	/* glfwMakeContextCurrent(pState->pWindow); */
	pState->windowWidth = appConfig.w;
	pState->windowHeight = appConfig.h;
    glfwSetKeyCallback(pState->pWindow, _KeyCallback);
	glfwSetMouseButtonCallback(pState->pWindow, _MouseCallback);
	pState->bGlfwInit = TRUE;
	return pState->bGlfwInit ;
}

YND const char**
OsGetRequiredInstanceExtensions(uint32_t* pCount)
{
	return glfwGetRequiredInstanceExtensions(pCount);
}

YND VkResult
OsCreateVkSurface(OsState *pOsState, VkContext *pContext)
{
	InternalState *pState = (InternalState *) pOsState->pInternalState;
	VK_CHECK(glfwCreateWindowSurface(pContext->instance, pState->pWindow, pContext->pAllocator, &pContext->surface));
	return VK_SUCCESS;
}

void _FramebufferSizeCallback(YMB GLFWwindow* pWindow, int width, int height);

YND b8
OsCreateGlContext(OsState* pOsState)
{
	InternalState *pState = (InternalState *) pOsState->pInternalState;
	glfwSetFramebufferSizeCallback(pState->pWindow, _FramebufferSizeCallback);
	return pState->bGlfwInit;
}

YND void *
OsGetGLFuncAddress(const char* pName)
{
	return glfwGetProcAddress(pName);
}

YND b8
OsSwapBuffers(OsState *pOsState)
{
	InternalState *pState = (InternalState *) pOsState->pInternalState;
	glfwSwapBuffers(pState->pWindow);
	return TRUE;
}

void
FramebufferUpdateInternalDimensions(OsState* pOsState, uint32_t width, uint32_t height)
{
	InternalState *pState = (InternalState *) pOsState->pInternalState;
	pState->windowWidth = width;
	pState->windowHeight = height;
}

void
OsFramebufferGetDimensions(OsState* pOsState, uint32_t* pWidth, uint32_t* pHeight)
{
	InternalState *pState = (InternalState *) pOsState->pInternalState;
	*pWidth = pState->windowWidth;
	*pHeight = pState->windowHeight;
}

void
OsShutdown(OsState* pOsState)
{ 
	InternalState *pState = (InternalState *) pOsState->pInternalState;
	glfwDestroyWindow(pState->pWindow);
	/* NOTE: Main thread only */
	glfwTerminate();
	free(pState);
	pState = NULL;
}

void
OsWrite(const char *pMessage, REDIR redir)
{
	size_t msgLength = strlen(pMessage);
	write(redir, pMessage, msgLength);
}

b8
OsPumpMessages(YMB OsState* pOsState)
{
	YMB InternalState *pState = (InternalState *) pOsState->pInternalState;
	glfwPollEvents();
	return TRUE;
}
/**
 * Get current time in unit
 */
YND f64 
OsGetAbsoluteTime(SECOND_UNIT unit)
{
	struct timespec tp;
	clock_gettime(CLOCK_REALTIME, &tp);
	f64 unitScale = 1.0f;
	switch (unit)
	{
		case NANOSECONDS:
			unitScale *= 1000.0f * 1000.0f * 1000.0f;
			break;
		case MICROSECONDS:
			unitScale *= 1000.0f * 1000.0f;
			break;
		case MILLISECONDS:
			unitScale *= 1000.0f;
			break;
		case SECONDS:
			unitScale *= 1.0f;
			break;
		default:
			YERROR("Unkown unit %d defaults to milliseconds", unit);
			break;
	}
	return tp.tv_sec * unitScale + tp.tv_nsec;
	/* return tp.tv_sec * 1000000000LL + tp.tv_nsec; */
}

void
OsSleep(uint64_t ms)
{
	sleep(ms / 1000);
}

void
_FramebufferSizeCallback(YMB GLFWwindow* pWindow, int width, int height)
{
	glViewport(0, 0, width, height);
}

void
_ErrorCallback(int error, const char* pDescription)
{
	YFATAL("Error[%d]: %s", error, pDescription);
	exit(1);
}

static void
_KeyCallback(YMB GLFWwindow* pWindow, YMB int key, YMB int scancode, int action, YMB int mods)
{
	b8 bPressed = FALSE;
	if (action == GLFW_PRESS || action == GLFW_REPEAT)
		bPressed = TRUE;
	InputProcessKey(key, bPressed);
}

static void
_MouseCallback(YMB GLFWwindow* pWindow, YMB int button, YMB int action, YMB int mods)
{
}

#endif // YGLFW3
