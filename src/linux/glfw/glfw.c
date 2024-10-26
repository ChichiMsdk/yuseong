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

typedef struct InternalState
{
	VkSurfaceKHR	surface;

	GLFWwindow*		pWindow;
	uint32_t		windowWidth;
	uint32_t		windowHeight;
}InternalState;


YND b8 
OsInit(OS_State *pOsState, AppConfig appConfig)
{
	/*
	 * NOTE: Thorough error handling
	 */
	pOsState->pInternalState = calloc(1, sizeof(InternalState));
	InternalState *pState = (InternalState *)pOsState->pInternalState;
	glfwSetErrorCallback(ErrorCallback);
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

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
	/* GLFWmonitor *pMonitor = glfwGetPrimaryMonitor(); */
	GLFWmonitor *pMonitor = NULL;
	pState->pWindow = glfwCreateWindow(appConfig.w, appConfig.h, appConfig.pAppName, pMonitor, NULL);
	if (!pState->pWindow)
	{
		const char *pDesc; 
		glfwGetError(&pDesc);
		YFATAL("Error vulkan not supported in %s at %d: %s", __FILE__, __LINE__, pDesc);
		return FALSE;
	}
	pState->windowWidth = appConfig.w;
	pState->windowHeight = appConfig.h;
    glfwSetKeyCallback(pState->pWindow, _KeyCallback);
	glfwSetMouseButtonCallback(pState->pWindow, _MouseCallback);
	return TRUE;
}

YND VkResult
OsCreateVkSurface(OS_State *pOsState, VkContext *pContext)
{
	InternalState *pState = (InternalState *) pOsState->pInternalState;
	VK_CHECK(glfwCreateWindowSurface(pContext->instance, pState->pWindow, pContext->pAllocator, &pContext->surface));
	return VK_SUCCESS;
}

void
FramebufferUpdateInternalDimensions(OS_State* pOsState, uint32_t width, uint32_t height)
{
	InternalState *pState = (InternalState *) pOsState->pInternalState;
	pState->windowWidth = width;
	pState->windowHeight = height;
}

void
FramebufferGetDimensions(OS_State* pOsState, uint32_t* pWidth, uint32_t* pHeight)
{
	InternalState *pState = (InternalState *) pOsState->pInternalState;
	*pWidth = pState->windowWidth;
	*pHeight = pState->windowHeight;
}

void
OsShutdown(OS_State* pOsState)
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
OsPumpMessages(YMB OS_State* pOsState)
{
	YMB InternalState *pState = (InternalState *) pOsState->pInternalState;
	glfwPollEvents();
	static uint64_t startTime;
	uint64_t currentTime = OS_GetAbsoluteTime();
	if (currentTime - startTime >= 100000000)
	{
		startTime = OS_GetAbsoluteTime();
		/* YINFO("time: %llu",  startTime); */
	}
	return TRUE;
}

YND f64 
OsGetAbsoluteTime(void)
{
	struct timespec tp;
	clock_gettime(CLOCK_REALTIME, &tp);
	return tp.tv_sec * 1000000000LL + tp.tv_nsec;
}

vid
OsSleep(uint64_t ms)
{
	sleep(ms / 1000);
}

void
ErrorCallback(int error, const char* pDescription)
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
