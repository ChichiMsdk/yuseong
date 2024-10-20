#include "os.h"

#ifdef YPLATFORM_WINDOWS
#	include "core/event.h"
#	include "core/input.h"
#	include "core/logger.h"
#	include "core/assert.h"

#	include <stdio.h>

#	include <sal.h>

#	define _CRT_SECURE_NO_WARNINGS
#	define WIN32_LEAN_AND_MEAN
#	include <windows.h>
#	include <windowsx.h> //param input extraction
#	include <strsafe.h>
#	include <malloc.h>
#	include <vulkan/vulkan.h>
#	include <vulkan/vulkan_win32.h>
#	include <vulkan/vk_enum_string_helper.h>

LRESULT CALLBACK Win32ProcessMessage(HWND hwnd, uint32_t msg, WPARAM wParam, LPARAM lParam);

void ErrorExit(LPCTSTR lpszFunction, DWORD dw);
void ErrorMsgBox(LPCTSTR lpszFunction, DWORD dw);

typedef struct InternalState
{
	HINSTANCE hInstance;
	HWND hwnd;
	VkSurfaceKHR surface;
	uint32_t windowWidth;
	uint32_t windowHeight;
}InternalState;

static f64 gClockFrequency;
static LARGE_INTEGER gStartTime;

YND b8 
OS_Init(OS_State *pOsState, const char *pAppName, int32_t x, int32_t y, int32_t w, int32_t h)
{
	pOsState->pInternalState = calloc(1, sizeof(InternalState));
	InternalState *pState = (InternalState *)pOsState->pInternalState;
	pState->hInstance = GetModuleHandleA(0);
	if (!pState->hInstance) ErrorExit("state->hInstance", GetLastError());

    // Setup and register window class.
	HICON icon = LoadIcon(pState->hInstance, IDI_APPLICATION);
	WNDCLASSA wc = {0};
	ZeroMemory(&wc, sizeof(WNDCLASSA));
	wc.style = CS_DBLCLKS;  // Get double-clicks
	wc.lpfnWndProc = Win32ProcessMessage;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = pState->hInstance;
	wc.hIcon = icon;

	wc.hCursor = LoadCursor(NULL, IDC_ARROW);  // NULL; // Manage the cursor manually
	if (!wc.hCursor) ErrorExit("wc.hCursor = LoadCursor(NULL, IDC_ARROW)", GetLastError());

	wc.hbrBackground = NULL;                   // Transparent
	wc.lpszClassName = "chichi_window_class";

	if (!RegisterClass(&wc)) ErrorExit("Window registration failed", GetLastError());

    // Create window
	uint32_t clientX = x;
	uint32_t clientY = y;
	uint32_t clientWidth = w;
	uint32_t clientHeight = h;
	
	uint32_t windowX = clientX;
	uint32_t windowY = clientY;
	uint32_t windowWidth = clientWidth;
	uint32_t windowHeight = clientHeight;
	
	uint32_t windowStyle = WS_OVERLAPPED | WS_SYSMENU | WS_CAPTION;
	uint32_t windowExStyle = WS_EX_APPWINDOW;
	
	windowStyle |= WS_MAXIMIZEBOX;
	windowStyle |= WS_MINIMIZEBOX;
	windowStyle |= WS_THICKFRAME;

	RECT borderRect = {0, 0, 0, 0};
    AdjustWindowRectEx(&borderRect, windowStyle, 0, windowExStyle);

	windowX += borderRect.left;
    windowY += borderRect.top;

	windowWidth += borderRect.right - borderRect.left;
	windowHeight += borderRect.bottom - borderRect.top;

    HWND handle = CreateWindowEx(
        windowExStyle, "chichi_window_class", pAppName,
        windowStyle, windowX, windowY, windowWidth, windowHeight,
        0, 0, pState->hInstance, 0);

	if (handle == 0)
	{
		MessageBoxA(NULL, "Window creation failed!", "Error!", MB_ICONEXCLAMATION | MB_OK);
		YFATAL("Window creation failed!");
		return FALSE;
	}
	else pState->hwnd = handle;

	pState->windowHeight = clientHeight;
	pState->windowWidth = clientWidth;

    // Show the window
	// TODO: if the window should not accept input, this should be false.
    b32 bShouldActivate = 1;  
    int32_t showWindowCommandFlag = bShouldActivate ? SW_SHOW : SW_SHOWNOACTIVATE;
    // If initially minimized, use SW_MINIMIZE : SW_SHOWMINNOACTIVE;
    // If initially maximized, use SW_SHOWMAXIMIZED : SW_MAXIMIZE
    ShowWindow(pState->hwnd, showWindowCommandFlag);

    // Clock setup
    LARGE_INTEGER frequency;
    QueryPerformanceFrequency(&frequency);
    gClockFrequency = 1.0 / (f64)frequency.QuadPart;
    QueryPerformanceCounter(&gStartTime);

    return TRUE;
}

void 
OS_Shutdown(OS_State *pState)
{
    // Simply cold-cast to the known type.
    InternalState *state = (InternalState *)pState->pInternalState;
    if (state->hwnd) 
	{
        DestroyWindow(state->hwnd);
        state->hwnd = 0;
    }
}

b8
OS_PumpMessages(YMB OS_State *pState) 
{
    MSG message;
    while (PeekMessageA(&message, NULL, 0, 0, PM_REMOVE)) 
	{
        TranslateMessage(&message);
        DispatchMessageA(&message);
    }
    return TRUE;
}

void
ErrorMsgBox(LPCTSTR lpszFunction, DWORD dw)
{ 
    LPVOID lpMsgBuf;
    LPVOID lpDisplayBuf;

    FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | 
        FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        dw,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPTSTR) &lpMsgBuf,
        0, NULL );

    // Display the error message and exit the process

    lpDisplayBuf = (LPVOID)LocalAlloc(LMEM_ZEROINIT, 
        (lstrlen((LPCTSTR)lpMsgBuf) + lstrlen((LPCTSTR)lpszFunction) + 40) * sizeof(TCHAR)); 
    StringCchPrintf((LPTSTR)lpDisplayBuf, 
        LocalSize(lpDisplayBuf) / sizeof(TCHAR),
        TEXT("%s failed with error %d: %s"), 
        lpszFunction, dw, lpMsgBuf); 
    MessageBox(NULL, (LPCTSTR)lpDisplayBuf, TEXT("Error"), MB_OK); 

    LocalFree(lpMsgBuf);
    LocalFree(lpDisplayBuf);
	printf("%lu\n", dw);
}

void
ErrorExit(LPCTSTR lpszFunction, DWORD dw) 
{
	ErrorMsgBox(lpszFunction, dw);
	ExitProcess(dw); 
}

/* TODO: ERROR Handling !! */
void
OS_Write(const char *pMessage, DWORD redir)
{
	HANDLE consoleHandle = GetStdHandle(redir); 
	DWORD dwMode = 0;

	GetConsoleMode(consoleHandle, &dwMode);
	dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
	SetConsoleMode(consoleHandle, dwMode);

	OutputDebugString(pMessage); //DEBUG console output windows only
	size_t length = strlen(pMessage);
	LPDWORD numberWritten = 0;
	WriteConsole(GetStdHandle(redir), pMessage, (DWORD)length, numberWritten, 0);
}

YND f64
OS_GetAbsoluteTime(void)
{
    LARGE_INTEGER nowTime;
    QueryPerformanceCounter(&nowTime);
    return (f64)nowTime.QuadPart * gClockFrequency;
}

void
OS_Sleep(uint64_t ms)
{
    Sleep(ms);
}

// Surface creation for Vulkan
YND VkResult
OS_CreateVkSurface(OS_State *pOsState, VkContext *pContext)
{
    InternalState *pState = (InternalState *)pOsState->pInternalState;

    VkWin32SurfaceCreateInfoKHR createInfo = {VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR};
    createInfo.hinstance = pState->hInstance;
    createInfo.hwnd = pState->hwnd;

    VK_CHECK(vkCreateWin32SurfaceKHR(pContext->instance, &createInfo, pContext->pAllocator, &pState->surface));

    pContext->surface = pState->surface;
    return VK_SUCCESS;
}

void
FramebufferGetDimensions(OS_State *pOsState, uint32_t* pWidth, uint32_t* pHeight)
{
	InternalState *pState = (InternalState *) pOsState->pInternalState;
	*pWidth = pState->windowWidth;
	*pHeight = pState->windowHeight;
}

LRESULT CALLBACK
Win32ProcessMessage(HWND hwnd, uint32_t msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) 
	{
        case WM_ERASEBKGND:
            // Notify the OS that erasing will be handled by the application to prevent flicker.
            return 1;
        case WM_CLOSE:
			EventContext data = {0};
			EventFire(EVENT_CODE_APPLICATION_QUIT, 0, data);
            return 0;
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
        case WM_SIZE: {
            // Get the updated size.
            RECT r;
            GetClientRect(hwnd, &r);
            uint32_t width = r.right - r.left;
            uint32_t height = r.bottom - r.top;
           	// Fire the event. The application layer should pick this up, but not handle it
            // as it shouldn be visible to other parts of the application.
            EventContext context;
            context.data.uint16_t[0] = (uint16_t)width;
            context.data.uint16_t[1] = (uint16_t)height;
            EventFire(EVENT_CODE_RESIZED, 0, context);
        } break;
        case WM_KEYDOWN:
        case WM_SYSKEYDOWN:
        case WM_KEYUP:
        case WM_SYSKEYUP: {
            // Key pressed/released
            b8 bPressed = (msg == WM_KEYDOWN || msg == WM_SYSKEYDOWN);
            Keys key = (uint16_t)wParam;
			InputProcessKey(key, bPressed);

        } break;
        case WM_MOUSEMOVE: {
            // Mouse move
            int32_t positionX = GET_X_LPARAM(lParam);
            int32_t positionY = GET_Y_LPARAM(lParam);
            InputProcessMouseMove(positionX, positionY);
        } break;
        case WM_MOUSEWHEEL: {
            int32_t deltaZ = GET_WHEEL_DELTA_WPARAM(wParam);
            if (deltaZ != 0) 
			{
                // Flatten the input to an OS-independent (-1, 1)
                deltaZ = (deltaZ < 0) ? -1 : 1;
				InputProcessMouseWheel(deltaZ);
            }
        } break;
        case WM_LBUTTONDOWN:
        case WM_MBUTTONDOWN:
        case WM_RBUTTONDOWN:
        case WM_LBUTTONUP:
        case WM_MBUTTONUP:
        case WM_RBUTTONUP: {
            b8 bPressed = msg == WM_LBUTTONDOWN || msg == WM_RBUTTONDOWN || msg == WM_MBUTTONDOWN;
			Buttons mouseButton = BUTTON_MAX_BUTTONS;
			switch(msg)
			{
				case WM_LBUTTONDOWN:
				case WM_LBUTTONUP:
					mouseButton = BUTTON_LEFT;
					break;
				case WM_MBUTTONDOWN:
				case WM_MBUTTONUP:
					mouseButton = BUTTON_MIDDLE;
					break;
				case WM_RBUTTONDOWN:
				case WM_RBUTTONUP:
					mouseButton = BUTTON_RIGHT;
					break;
			}
			if (mouseButton != BUTTON_MAX_BUTTONS)
			{
				InputProcessButton(mouseButton, bPressed);
			}
        } break;
    }

    return DefWindowProcA(hwnd, msg, wParam, lParam);
}

[[deprecated]] void
OS_WriteOld(const char *pMessage, YMB uint8_t colour) 
{
	// TODO: check error looks boring but important
	HANDLE console_handle = GetStdHandle(STD_OUTPUT_HANDLE); 
	DWORD dwMode = 0;
	// always call GetConsoleMode before SetConsoleMode ..
	GetConsoleMode(console_handle, &dwMode);
	dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
	SetConsoleMode(console_handle, dwMode);

	// FATAL,ERROR,WARN,INFO,DEBUG,TRACE
	//    static u8 levels[6] = {BACKGROUND_RED, FOREGROUND_RED, FOREGROUND_GREEN | FOREGROUND_RED, FOREGROUND_GREEN,FOREGROUND_BLUE, FOREGROUND_INTENSITY};
	//    SetConsoleTextAttribute(console_handle, levels[colour]); //check MSDN deprecated for powershell
	OutputDebugStringA(pMessage); //DEBUG console output windows only
	uint64_t length = strlen(pMessage);
	LPDWORD number_written = 0;
	WriteConsoleA(GetStdHandle(STD_OUTPUT_HANDLE), pMessage, (DWORD)length, number_written, 0);
	//    SetConsoleTextAttribute(console_handle, 0x0F); //resets the color
}

[[deprecated]] void
OS_WriteError(const char *pMessage, YMB uint8_t colour) 
{
	//  TODO: check error looks boring but important
	HANDLE console_handle = GetStdHandle(STD_ERROR_HANDLE);

	DWORD dwMode = 0;
	GetConsoleMode(console_handle, &dwMode);
	dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
	SetConsoleMode(console_handle, dwMode);

	// FATAL,ERROR,WARN,INFO,DEBUG,TRACE
	//    static u8 levels[6] = {BACKGROUND_RED, FOREGROUND_RED, FOREGROUND_GREEN | FOREGROUND_RED, FOREGROUND_GREEN,FOREGROUND_BLUE, FOREGROUND_INTENSITY};
	//    SetConsoleTextAttribute(console_handle, levels[colour]);
	OutputDebugStringA(pMessage);
	uint64_t length = strlen(pMessage);
	LPDWORD number_written = 0;
	WriteConsoleA(GetStdHandle(STD_ERROR_HANDLE), pMessage, (DWORD)length, number_written, 0);
	//    SetConsoleTextAttribute(console_handle, 0x0F); //resets the color
}

#endif // YPLATFORM_WINDOWS
