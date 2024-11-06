#include <stdio.h>
#include <stdlib.h>

#include "yuseong.h"

#include "core/darray_debug.h"
#include "core/darray.h"

b8 gRunning = TRUE;

/* const char* __asan_default_options() { return "detect_leaks=0"; } */
#include "test.h"

AppConfig gAppConfig = { .pAppName = "yuseong", .x = 100, .y = 100, .w = 1500, .h = 900, };
OsState gOsState = {0};

/* FIXME: Bug in DarrayLength - DarrayGetCapacity !! */

#ifndef TESTING
int
main(int argc, char **ppArgv)
{
	RendererType defaultType = RENDERER_TYPE_VULKAN;

	ArgvCheck(argc, ppArgv, &defaultType);
	RendererConfig config = {.type = defaultType};
	gAppConfig.pRendererConfig = &config;

	if (!OsInit(&gOsState, gAppConfig))
		exit(1);

	EventInit();
	InputInitialize();
	EventRegister(EVENT_CODE_APPLICATION_QUIT, 0, _OnEvent);
	EventRegister(EVENT_CODE_KEY_PRESSED, 0, _OnKey);
	EventRegister(EVENT_CODE_KEY_RELEASED, 0, _OnKey);
	EventRegister(EVENT_CODE_RESIZED, 0, _OnResized);

	YU_ASSERT(RendererInit(&gOsState, &gAppConfig.pRenderer, config));
	KASSERT(gAppConfig.pRenderer);

	f64 deltaFrameTime = 0.0f;
	f64 startFrameTime = 0.0f;
	f64 endFrameTime = 0.0f;

	while (gRunning)
	{
		deltaFrameTime = endFrameTime - startFrameTime;
		startFrameTime = OsGetAbsoluteTime(NANOSECONDS);

		OsPumpMessages(&gOsState);
		if (!gAppConfig.bSuspended)
		{
			InputUpdate(deltaFrameTime);
			YU_ASSERT(YuDraw(&gOsState, gAppConfig.pRenderer));
		}

		endFrameTime = OsGetAbsoluteTime(NANOSECONDS);
	}

	YuShutdown(gAppConfig.pRenderer);
	InputShutdown();
	EventShutdown();
	OsShutdown(&gOsState);
	GetLeaks();
	SystemMemoryUsagePrint();
	return 0;
}
#endif // TESTING
