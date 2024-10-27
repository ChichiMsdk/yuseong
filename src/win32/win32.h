#ifndef WIN32_H
#define WIN32_H

#include <sal.h>

#define _CRT_SECURE_NO_WARNINGS
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <windowsx.h> //param input extraction
#include <strsafe.h>
#include <malloc.h>

//TODO: Add define per Renderer
#if 1

#include <vulkan/vulkan.h>
#include <vulkan/vulkan_win32.h>
#include <vulkan/vk_enum_string_helper.h>

#endif // 1

typedef struct InternalState
{
	HINSTANCE hInstance;
	HWND hWindow;
	VkSurfaceKHR surface;
	uint32_t windowWidth;
	uint32_t windowHeight;

	HDC glCtx;
}InternalState;

LRESULT CALLBACK Win32ProcessMessage(
		HWND								hwnd,
		uint32_t							msg,
		WPARAM								wParam,
		LPARAM								lParam);

void ErrorExit(
		LPCTSTR								lpszFunction,
		DWORD								dw);

void ErrorMsgBox(
		LPCTSTR								lpszFunction,
		DWORD								dw);

#endif
