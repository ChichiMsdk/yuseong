#ifndef OS_H
#define OS_H

#include "mydefines.h"
#include "renderer/yvulkan.h"
#include <stdint.h>

typedef struct OS_State
{
	void *pInternalState;
}OS_State;

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
		OS_State*							pOsState,
		uint32_t							width,
		uint32_t							height);

void FramebufferGetDimensions(
		OS_State*							pOsState,
		uint32_t*							pWidth,
		uint32_t*							pHeight);

b8 OS_PumpMessages(
		OS_State*							pState);

YND b8 OS_Init(
		OS_State*							pState,
		const char*							pAppName,
		i32									x,
		i32									y,
		i32									w,
		i32									h);

void OS_Shutdown(
		OS_State*							pState);

void OS_Write(
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

YND f64 OS_GetAbsoluteTime(void);

YND VkResult OS_CreateVkSurface(
		OS_State*							pState,
		VkContext*							pContext);

#endif // OS_H
