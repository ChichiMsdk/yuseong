#include "yuseong.h"

#include "core/ystring.h"

void
ArgvCheck(int argc, char **ppArgv, RendererType *pType)
{
	if (argc >= 2)
	{
		int result = yAtoi(ppArgv[1]);
		if (result < MAX_RENDERER_TYPE && result >= 0)
		{
			*pType = result;
			YINFO("Renderer type chosen -> %s", pRendererType[*pType]);
		}
		else
			YERROR("Unknown rendererType option. Using default %s", pRendererType[*pType]);
	}
}

b8
_OnResized(uint16_t code, YMB void* pSender, YMB void* pListenerInst, EventContext context) 
{
	if (code == EVENT_CODE_RESIZED) 
	{
		uint16_t width = context.data.uint16_t[0];
		uint16_t height = context.data.uint16_t[1];

		// Check if different. If so, trigger a resize event.
		if (width != gAppConfig.w || height != gAppConfig.h) 
		{
			gAppConfig.w = width;
			gAppConfig.h = height;

			YDEBUG("Window resize: %i, %i", width, height);

			// Handle minimization
			if (width == 0 || height == 0) 
			{
				YINFO("Window minimized, suspending application.");
				gAppConfig.bSuspended = TRUE;
				return TRUE;
			}
			else 
			{
				if (gAppConfig.bSuspended) 
				{
					YINFO("Window restored, resuming application.");
					gAppConfig.bSuspended = FALSE;
				}
				YU_ASSERT(YuResizeWindow(&gOsState, gAppConfig.pRenderer, width, height) == YU_SUCCESS);
			}
		}
	}
    // Event purposely not handled to allow other listeners to get this.
    return FALSE;
}

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