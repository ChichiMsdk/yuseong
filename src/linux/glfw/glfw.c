#include "os.h"

#ifdef PLATFORM_LINUX
#ifdef YGLFW3

#include "core/logger.h"
#include <vulkan/vulkan.h>

#include <math.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <string.h>

#define GLFW_CHECK(expr)\
	do { \
		const char *pDes;\
		int errcode = (expr); \
		if (errcode != GLFW_NO_ERROR) \
		{ \
			glfwGetError(&pDes);\
			YERROR("Error[%d] %s:%d: %s", errcode, __FILE__, __LINE__,  pDes); \
			return errcode; \
		} \
	} while (0);

void ErrorCallback(int error, const char* pDescription);

typedef struct InternalState
{
	VkSurfaceKHR surface;

	GLFWwindow* pWindow;
	uint32_t windowWidth;
	uint32_t windowHeight;
}InternalState;

static void
_KeyCallback(GLFWwindow* pWindow, int key, YMB int scancode, int action, YMB int mods)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(pWindow, GLFW_TRUE);
}

YND b8 
OS_Init(OS_State *pOsState, const char *pAppName, YMB int32_t x, YMB int32_t y, int32_t w, int32_t h)
{
	/*
	 * NOTE: Thorough error handling
	 */
	pOsState->pInternalState = calloc(1, sizeof(InternalState));
	InternalState *pState = (InternalState *)pOsState->pInternalState;
	glfwSetErrorCallback(ErrorCallback);
    /*
	 * GLFW_CHECK(glfwInit());
     */
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
	/* GLFWmonitor *pMonitor = glfwGetPrimaryMonitor(); */
	GLFWmonitor *pMonitor = NULL;
	pState->pWindow = glfwCreateWindow(w, h, pAppName, pMonitor, NULL);
	if (!pState->pWindow)
	{
		const char *pDesc; 
		glfwGetError(&pDesc);
		YFATAL("Error vulkan not supported in %s at %d: %s", __FILE__, __LINE__, pDesc);
		return FALSE;
	}
	pState->windowWidth = w;
	pState->windowHeight = w;
    glfwSetKeyCallback(pState->pWindow, _KeyCallback);
	return TRUE;
}

YND VkResult
OS_CreateVkSurface(OS_State *pOsState, VkContext *pContext)
{
	InternalState *pState = (InternalState *) pOsState->pInternalState;
	VK_CHECK(glfwCreateWindowSurface(pContext->instance, pState->pWindow, pContext->pAllocator, &pContext->surface));
	return VK_SUCCESS;
}

void
FramebufferGetDimensions(OS_State* pOsState, uint32_t* pWidth, uint32_t* pHeight)
{
	InternalState *pState = (InternalState *) pOsState->pInternalState;
	*pWidth = pState->windowWidth;
	*pHeight = pState->windowHeight;
}

void
OS_Shutdown(OS_State* pOsState)
{ 
	InternalState *pState = (InternalState *) pOsState->pInternalState;
	glfwDestroyWindow(pState->pWindow);
	/* NOTE: Main thread only */
	glfwTerminate();
	free(pState);
	pState = NULL;
}

void
OS_Write(const char *pMessage, REDIR redir)
{
	size_t msgLength = strlen(pMessage);
	write(redir, pMessage, msgLength);
}

b8
OS_PumpMessages(YMB OS_State* pOsState)
{
	InternalState *pState = (InternalState *) pOsState->pInternalState;
	if (glfwWindowShouldClose(pState->pWindow))
		exit(1);
	glfwPollEvents();
	static uint64_t startTime;
	uint64_t currentTime = OS_GetAbsoluteTime();
	if (currentTime - startTime >= 100000000)
	{
		startTime = OS_GetAbsoluteTime();
		YINFO("time: %llu",  startTime);
	}
	return TRUE;
}

YND uint64_t 
OS_GetAbsoluteTime(void)
{
	struct timespec tp;
	clock_gettime(CLOCK_REALTIME, &tp);
	return tp.tv_sec * 1000000000LL + tp.tv_nsec;
}

void
OS_Sleep(uint64_t ms)
{
	sleep(ms / 1000);
}

void
ErrorCallback(int error, const char* pDescription)
{
	YFATAL("Error[%d]: %s", error, pDescription);
	exit(1);
}

#endif // YGLFW3
#endif // PLATFORM_LINUX
