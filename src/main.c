#include <stdio.h>
#include <stdlib.h>

#include "os.h"
#include "core/event.h"
#include "core/input.h"
#include "core/ymemory.h"
#include "core/logger.h"

#include "renderer/yvulkan.h"
#include <vulkan/vk_enum_string_helper.h>

YND VkResult yDraw(void);

b8 gRunning = TRUE;

b8 _OnEvent(uint16_t code, void* pSender, void* pListenerInst, EventContext context);
b8 _OnKey(uint16_t code, void* pSender, void* pListenerInst, EventContext context);
b8 _OnResized(uint16_t code, void* pSender, void* pListenerInst, EventContext context);

/* #include "test.h" */

/* const char* __asan_default_options() { return "detect_leaks=0"; } */

#ifndef TESTING
#ifdef PLATFORM_WINDOWS
int
main(void)
{
	OS_State state = {0};
	if (!OS_Init(&state, "yuseong", 100, 100, 500, 500))
		exit(1);

	EventInit();
	InputInitialize();
	EventRegister(EVENT_CODE_APPLICATION_QUIT, 0, _OnEvent);
	EventRegister(EVENT_CODE_KEY_PRESSED, 0, _OnKey);
	EventRegister(EVENT_CODE_KEY_RELEASED, 0, _OnKey);

	VK_ASSERT(RendererInit(&state));
	YINFO("%s", StrGetMemoryUsage());

	while (gRunning)
	{
		OS_PumpMessages(&state);
		InputUpdate(1);
		VK_ASSERT(yDraw());
	}
	RendererShutdown(&state);
	InputShutdown();
	OS_Shutdown(&state);
	YDEBUG("%s", StrGetMemoryUsage());
	return 0;
}
#elif PLATFORM_LINUX
const char* __asan_default_options() { return "detect_leaks=0"; }
int
main(void)
{
	OS_State state = {0};
	if(!OS_Init(&state, "yuseong", 100, 100, 500, 500))
		exit(1);
	VK_ASSERT(RendererInit(&state));
	while(gRunning)
	{
		OS_PumpMessages(&state);
		VK_ASSERT(yDraw());
	}
	RendererShutdown(&state);
	OS_Shutdown(&state);
	InputShutdown();
	SystemMemoryUsagePrint();
	return 0;
}

#endif  //PLATFORM_WINDOWS
#endif // TESTING

b8
_OnEvent(uint16_t code, YMB void* pSender, YMB void* pListenerInst, YMB EventContext context)
{
	switch (code) 
	{
		case EVENT_CODE_APPLICATION_QUIT: 
			{
				YDEBUG("EVENT_CODE_APPLICATION_QUIT received, shutting down.");
				gRunning = FALSE;
				return TRUE;
			}
	}
	return FALSE;
}

b8
_OnKey(uint16_t code, YMB void* pSender, YMB void* pListenerInst, EventContext context) 
{
	if (code == EVENT_CODE_KEY_PRESSED) 
	{
		uint16_t keyCode = context.data.uint16_t[0];
		if (keyCode == KEY_ESCAPE) 
		{
			// NOTE: Technically firing an event to itself, but there may be other listeners.
			EventContext data = {0};
			EventFire(EVENT_CODE_APPLICATION_QUIT, 0, data);

			// Block anything else from processing this.
			return TRUE;
		}
		YINFO("'%c' key pressed in window.\n", keyCode);
	}
	else if (code == EVENT_CODE_KEY_RELEASED) 
	{
		YMB uint16_t keyCode = context.data.uint16_t[0];
		YINFO("'%c' key released in window.\n", keyCode);
	}
	return FALSE;
}
