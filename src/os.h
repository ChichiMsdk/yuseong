#ifndef OS_H
#define OS_H

#include "mydefines.h"
#include "renderer/vulkan/yvulkan.h"
#include <stdint.h>

typedef struct OsState
{
	void *pInternalState;
}OsState;

#ifdef PLATFORM_WINDOWS
#	ifndef _WINDEF_
		typedef unsigned long DWORD;
#	endif

	typedef DWORD REDIR;
#	define VK_KHR_SURFACE_OS "VK_KHR_win32_surface"

#elif PLATFORM_LINUX
#	include <stdio.h>
	typedef FILE* STDREDIR;
	typedef int REDIR;

#	ifdef COMP_WAYLAND
#		define VK_KHR_SURFACE_OS "VK_KHR_wayland_surface"
#	elif COMP_X11
#		define VK_KHR_SURFACE_OS "VK_KHR_xlib_surface"
#	elif YGLFW3
#		define VK_KHR_SURFACE_OS "VK_KHR_xcb_surface"
#	endif

#endif // PLATFORM_WINDOWS

extern b8 gRunning;

void FramebufferUpdateInternalDimensions(
		OsState*							pOsState,
		uint32_t							width,
		uint32_t							height);

void FramebufferGetDimensions(
		OsState*							pOsState,
		uint32_t*							pWidth,
		uint32_t*							pHeight);

b8 OS_PumpMessages(
		OsState*							pState);

YND b8 OsInit(
		OsState*							pState,
		const char*							pAppName,
		int32_t								x,
		int32_t								y,
		int32_t								w,
		int32_t								h);

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

void OS_Sleep(
		uint64_t							ms);

YND f64 OsGetAbsoluteTime(void);

YND VkResult OsCreateVkSurface(
		OsState*							pState,
		VkContext*							pContext);

#endif // OS_H
