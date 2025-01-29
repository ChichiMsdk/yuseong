#include <stdio.h>
#include <stdlib.h>

#include "yuseong.h"

#include "core/darray_debug.h"
#include "core/darray.h"

b8 gRunning = TRUE;

/* TODO: Make it a function that looks config file for the folder ? */
const char *gppShaderFilePath[] = {
    "./build/obj/shaders/gradient_color.comp.spv",
    "./build/obj/shaders/gradient.comp.spv",
    "./build/obj/shaders/sky.comp.spv",
};

uint32_t gFilePathSize = COUNT_OF(gppShaderFilePath);
int32_t gShaderFileIndex = 0;

const char* __asan_default_options() { return "detect_leaks=0"; }
#include "test.h"

AppConfig gAppConfig = { .pAppName = "yuseong", .x = 100, .y = 100, .w = 1500, .h = 900, };
OsState gOsState = {0};

/* FIXME: Bug in DarrayLength - DarrayGetCapacity !! */
/* FIXME: xmm0 registry bug alignment is wrong */

#ifndef TESTING

int
main(int argc, char **ppArgv)
{
    RendererType	defaultType	= RENDERER_TYPE_VULKAN;

    ArgvCheck(argc, ppArgv, &defaultType);
    RendererConfig config = {.type = defaultType};
    gAppConfig.pRendererConfig = &config;

    if (!OsInit(&gOsState, gAppConfig))
	exit(1);

    AddEventCallbackAndInit();
    InputInitialize();

    YU_ASSERT(RendererInit(&gOsState, &gAppConfig.pRenderer, config));
    YASSERT(gAppConfig.pRenderer);

    f64	deltaFrameTime	= 0.0f;
    f64	startFrameTime	= 0.0f;
    f64	endFrameTime	= 0.0f;

    while (gRunning)
    {
	deltaFrameTime	= endFrameTime - startFrameTime;
	startFrameTime	= OsGetAbsoluteTime(NANOSECONDS);

	OsPumpMessages(&gOsState);
	if (!gAppConfig.bSuspended)
	{
	    InputUpdate(deltaFrameTime);
	    YU_ASSERT(YuDraw(&gOsState, gAppConfig.pRenderer));
	}
	endFrameTime	= OsGetAbsoluteTime(NANOSECONDS);
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
