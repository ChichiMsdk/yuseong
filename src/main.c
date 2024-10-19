#include <stdio.h>
#include <stdlib.h>

#include "os.h"
#include "core/event.h"
#include "core/input.h"
#include "core/ymemory.h"
#include "core/logger.h"

#include "renderer/yvulkan.h"
#include <vulkan/vk_enum_string_helper.h>

b8 gRunning = TRUE;

b8 _OnEvent(uint16_t code, void* pSender, void* pListenerInst, EventContext context);
b8 _OnKey(uint16_t code, void* pSender, void* pListenerInst, EventContext context);
b8 _OnResized(uint16_t code, void* pSender, void* pListenerInst, EventContext context);

#define TESTING

#ifdef TESTING
int
main(void)
{
	struct test {
		int a;
		int b;
		int c;
		int d;
		int e;
	};

	struct test test = {0};
	int a = 0;
	char b = 0;
	long c = 0;
	void *d = 0;
	YINFO("size int %d\n", sizeof(typeof(a)));
	YINFO("size char %d\n", sizeof(typeof(b)));
	YINFO("size long %d\n", sizeof(typeof(c)));
	YINFO("size void* %d\n", sizeof(typeof(d)));
	YINFO("size struct %d\n", sizeof(typeof(test)));
}

#endif

#ifndef TESTING

#ifdef PLATFORM_WINDOWS
int
main(void)
{
	OS_State state = {0};
	if(!OS_Init(&state, "yuseong", 100, 100, 500, 500))
		exit(1);

	EventInit();
	InputInitialize();
	EventRegister(EVENT_CODE_APPLICATION_QUIT, 0, _OnEvent);
	EventRegister(EVENT_CODE_KEY_PRESSED, 0, _OnKey);
	EventRegister(EVENT_CODE_KEY_RELEASED, 0, _OnKey);

	VK_ASSERT(RendererInit(&state));

	while (gRunning)
	{
		OS_PumpMessages(&state);
		InputUpdate(1);
	}

	RendererShutdown(&state);
	return 0;
}
#elif PLATFORM_LINUX
int
main(void)
{
	OS_State state = {0};
	if(!OS_Init(&state, "yuseong", 100, 100, 500, 500))
		exit(1);
	while(gRunning)
	{
		OS_PumpMessages(&state);
	}
	OS_Shutdown(&state);
	return 0;
}

#endif  //PLATFORM_WINDOWS

#endif // TESTING

b8
_OnEvent(uint16_t code, void* pSender, void* pListenerInst, EventContext context)
{
	(void)pSender;
	(void)pListenerInst;
	(void)context;
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
_OnKey(uint16_t code, void* pSender, void* pListenerInst, EventContext context) 
{
	(void)pSender;
	(void)pListenerInst;
	if (code == EVENT_CODE_KEY_PRESSED) 
	{
		uint16_t keyCode = context.data.uint16_t[0];
		if (keyCode == KEY_ESCAPE) 
		{
			// NOTE: Technically firing an event to itself, but there may be other listeners.
			EventContext data = {};
			EventFire(EVENT_CODE_APPLICATION_QUIT, 0, data);

			// Block anything else from processing this.
			return TRUE;
		}
		YINFO("'%c' key pressed in window.\n", keyCode);
	}
	else if (code == EVENT_CODE_KEY_RELEASED) 
	{
		uint16_t keyCode = context.data.uint16_t[0];
		YINFO("'%c' key released in window.\n", keyCode);
	}
	return FALSE;
}
