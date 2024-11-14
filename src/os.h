#ifndef OS_H
#define OS_H

#include "mydefines.h"
#include "renderer/vulkan/yvulkan.h"
#include <stdint.h>

typedef struct OsState
{
	void *pInternalState;
}OsState;

typedef struct YuRenderer YuRenderer;
typedef struct RendererConfig RendererConfig;

// TODO: Change the name to reflect the state 
typedef struct AppConfig
{
	const char*		pAppName;
	int32_t			x, y, w, h;
	b8				bSuspended;
	b8				bRunning;
	YuRenderer*		pRenderer;
	RendererConfig*	pRendererConfig;
} AppConfig;

#ifdef PLATFORM_WINDOWS
	#ifndef _WINDEF_
		typedef unsigned long DWORD;
	#endif

	typedef DWORD REDIR;
	#define VK_KHR_SURFACE_OS "VK_KHR_win32_surface"

#elif PLATFORM_LINUX
	#include <stdio.h>
	typedef FILE* STDREDIR;
	typedef int REDIR;

	#ifdef COMP_WAYLAND
		#define VK_KHR_SURFACE_OS "VK_KHR_wayland_surface"
	#elif COMP_X11
		#define VK_KHR_SURFACE_OS "VK_KHR_xlib_surface"
	#elif YGLFW3
		#define VK_KHR_SURFACE_OS "VK_KHR_xcb_surface"
	#endif
#endif // PLATFORM_WINDOWS

extern b8 gRunning;

YND const char** OsGetRequiredInstanceExtensions(
		uint32_t*							pCount);

void FramebufferUpdateInternalDimensions(
		OsState*							pOsState,
		uint32_t							width,
		uint32_t							height);

void OsFramebufferGetDimensions(
		OsState*							pOsState,
		uint32_t*							pWidth,
		uint32_t*							pHeight);

b8 OsPumpMessages(
		OsState*							pState);

YND b8 OsInit(
		OsState*							pState,
		AppConfig							appConfig);

void OsShutdown(
		OsState*							pState);

void OsWrite(
		const char*							pMessage,
		REDIR								redir);

[[deprecated]] void OS_WriteOld(
		const char*							pMessage,
		u8									colour);

[[deprecated]] void OS_WriteError(
		const char*							pMessage,
		u8									colour);

void OsSleep(
		uint64_t							ms);

YND f64 OsGetAbsoluteTime(
		SECOND_UNIT							unit);

YND VkResult OsCreateVkSurface(
		OsState*							pState,
		VkContext*							pContext);

YND void *OsGetGLFuncAddress(
		const char*							pName);

YND b8 OsCreateGlContext(
		OsState*							pOsState);

YND b8 OsSwapBuffers(
		OsState*							pOsState);

#endif // OS_H
