#include <stdio.h>
#include <stdlib.h>

#include "yuseong.h"

b8 gRunning = TRUE;

b8 _OnEvent(uint16_t code, void* pSender, void* pListenerInst, EventContext context);
b8 _OnKey(uint16_t code, void* pSender, void* pListenerInst, EventContext context);
b8 _OnResized(uint16_t code, void* pSender, void* pListenerInst, EventContext context);

const char* __asan_default_options() { return "detect_leaks=0"; }
#include "test.h"

#ifndef TESTING
int
main(void)
{
	OsState state = {0};
	int32_t x = 100; int32_t y = 100;
	int32_t w = 500; int32_t h = 500;
	const char *pAppName = "yuseong";
	if (!OsInit(&state, pAppName, x, y, w, h))
		exit(1);

	EventInit();
	InputInitialize();
	EventRegister(EVENT_CODE_APPLICATION_QUIT, 0, _OnEvent);
	EventRegister(EVENT_CODE_KEY_PRESSED, 0, _OnKey);
	EventRegister(EVENT_CODE_KEY_RELEASED, 0, _OnKey);

	RendererConfig config = {.type = RENDERER_TYPE_VULKAN};

	YuRenderer *pRenderer = RendererInit(&state, config);
	YU_ASSERT(pRenderer);

	while (gRunning)
	{
		OS_PumpMessages(&state);
		InputUpdate(1);
		YU_ASSERT(YuDraw(pRenderer) == YU_SUCCESS);
	}
	YuShutdown(pRenderer);
	InputShutdown();
	OsShutdown(&state);
	/* SystemMemoryUsagePrint(); */
	return 0;
}
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
	if (code == EVENT_CODE_KEY_PRESSED || code == EVENT_CODE_KEY_RELEASED) 
	{
		uint16_t keyCode = context.data.uint16_t[0];
		if (keyCode == KEY_ESCAPE) 
		{
			// NOTE: Technically firing an event to itself, but there may be other listeners.
			EventContext data = {0};
			EventFire(EVENT_CODE_APPLICATION_QUIT, 0, data);
			return TRUE;
		}
		YINFO("%d -> '%c' key pressed in window.\n", keyCode, keyCode);
	}
	else if (code == EVENT_CODE_KEY_RELEASED) 
	{
		YMB uint16_t keyCode = context.data.uint16_t[0];
		YINFO("%d -> '%c' key released in window.\n", keyCode, keyCode);
	}
	return FALSE;
}
