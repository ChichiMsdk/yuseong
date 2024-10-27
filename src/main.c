#include <stdio.h>
#include <stdlib.h>

#include "yuseong.h"

b8 gRunning = TRUE;

const char* __asan_default_options() { return "detect_leaks=0"; }
#include "test.h"

AppConfig gAppConfig = { .pAppName = "yuseong", .x = 100, .y = 100, .w = 500, .h = 500, };
OsState gOsState = {0};

#ifndef TESTING
int
main(int argc, char **ppArgv)
{
	RendererType defaultType = RENDERER_TYPE_VULKAN;
	ArgvCheck(argc, ppArgv, &defaultType);
	if (!OsInit(&gOsState, gAppConfig))
		exit(1);

	EventInit();
	InputInitialize();
	EventRegister(EVENT_CODE_APPLICATION_QUIT, 0, _OnEvent);
	EventRegister(EVENT_CODE_KEY_PRESSED, 0, _OnKey);
	EventRegister(EVENT_CODE_KEY_RELEASED, 0, _OnKey);
	EventRegister(EVENT_CODE_RESIZED, 0, _OnResized);

	RendererConfig config = {.type = defaultType};
	gAppConfig.pRenderer = RendererInit(&gOsState, config);
	YU_ASSERT(gAppConfig.pRenderer);

	f64 deltaFrameTime = 0.0f;
	f64 startFrameTime = 0.0f;
	f64 endFrameTime = 0.0f;
	while (gRunning)
	{
		deltaFrameTime = endFrameTime - startFrameTime;
		startFrameTime = OsGetAbsoluteTime(NANOSECONDS);
		OS_PumpMessages(&gOsState);
		if (gAppConfig.bSuspended)
		{
			InputUpdate(deltaFrameTime);
			YU_ASSERT(YuDraw(&gOsState, gAppConfig.pRenderer) == YU_SUCCESS);
		}
		endFrameTime = OsGetAbsoluteTime(MICROSECONDS);
	}
	YuShutdown(gAppConfig.pRenderer);
	InputShutdown();
	OsShutdown(&gOsState);
	/* SystemMemoryUsagePrint(); */
	return 0;
}
#endif // TESTING
