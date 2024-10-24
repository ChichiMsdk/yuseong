#include "mydefines.h"
#include "core/input.h"
#include "core/event.h"
#include "core/logger.h"

#include <stdio.h>
#include <string.h>

typedef struct KeyboardState 
{
    b8 pKeys[256];
} KeyboardState;

typedef struct MouseState 
{
    int16_t x;
    int16_t y;
    uint8_t pButtons[BUTTON_MAX_BUTTONS];
} MouseState;

typedef struct InputState 
{
    KeyboardState keyboardCurrent;
    KeyboardState keyboardPrevious;
    MouseState mouseCurrent;
    MouseState mousePrevious;
} InputState;

// Internal input state
static b8 gbInitialized = FALSE;
static InputState gState = {};

void
InputInitialize(void) 
{
    gbInitialized = TRUE;
    YINFO("Input subsystem initialized.");
}

void
InputShutdown(void) 
{
    // TODO: Add shutdown routines when needed.
    gbInitialized = FALSE;
}

void
InputUpdate(YMB f64 deltaTime) 
{
    if (!gbInitialized) 
	{
        return;
    }
    // Copy current states to previous states.
    memcpy(&gState.keyboardPrevious, &gState.keyboardCurrent, sizeof(KeyboardState));
    memcpy(&gState.mousePrevious, &gState.mouseCurrent, sizeof(MouseState));
}

void
InputProcessKey(Keys key, b8 bPressed) 
{
    // Only handle this if the state actually changed.
	if (gState.keyboardCurrent.pKeys[key] != bPressed) 
	{
		// Update internal state.
		gState.keyboardCurrent.pKeys[key] = bPressed;
		// Fire off an event for immediate processing.
		EventContext context;
		context.data.uint16_t[0] = key;
		EventFire(bPressed ? EVENT_CODE_KEY_PRESSED : EVENT_CODE_KEY_RELEASED, 0, context);
	}
}

void
InputProcessMouseButton(MouseButtons button, b8 bPressed) 
{
    // If the state changed, fire an event.
    if (gState.mouseCurrent.pButtons[button] != bPressed) 
	{
        gState.mouseCurrent.pButtons[button] = bPressed;
        // Fire the event.
        EventContext context;
        context.data.uint16_t[0] = button;
        EventFire(bPressed ? EVENT_CODE_BUTTON_PRESSED : EVENT_CODE_BUTTON_RELEASED, 0, context);
    }
}

void
InputProcessMouseMove(int16_t x, int16_t y) 
{
    // Only process if actually different
    if (gState.mouseCurrent.x != x || gState.mouseCurrent.y != y) 
	{
  		/* YDEBUG("Mouse pos: %i, %i!", x, y); */

        // Update internal state.
        gState.mouseCurrent.x = x;
        gState.mouseCurrent.y = y;
        // Fire the event.
        EventContext context;
        context.data.uint16_t[0] = x;
        context.data.uint16_t[1] = y;
        EventFire(EVENT_CODE_MOUSE_MOVED, 0, context);
    }
}

void
InputProcessMouseWheel(int8_t z_delta) 
{
    // NOTE: no internal state to update.

    // Fire the event.
    EventContext context;
    context.data.uint8_t[0] = z_delta;
    EventFire(EVENT_CODE_MOUSE_WHEEL, 0, context);
}

YND b8
InputIsKeyDown(Keys key) 
{
    if (!gbInitialized) 
	{
        return FALSE;
    }
    return gState.keyboardCurrent.pKeys[key] == TRUE;
}

YND b8
InputIsKeyUp(Keys key) 
{
    if (!gbInitialized) 
	{
        return TRUE;
    }
    return gState.keyboardCurrent.pKeys[key] == FALSE;
}

YND b8
InputWasKeyDown(Keys key) 
{
    if (!gbInitialized) 
	{
        return FALSE;
    }
    return gState.keyboardPrevious.pKeys[key] == TRUE;
}

YND b8
InputWasKeyUp(Keys key) 
{
    if (!gbInitialized) 
	{
        return TRUE;
    }
    return gState.keyboardPrevious.pKeys[key] == FALSE;
}

// mouse input
YND b8
InputIsMouseButtonDown(MouseButtons button) 
{
    if (!gbInitialized) 
	{
        return FALSE;
    }
    return gState.mouseCurrent.pButtons[button] == TRUE;
}

YND b8
InputIsMouseButtonUp(MouseButtons button) 
{
    if (!gbInitialized) 
	{
        return TRUE;
    }
    return gState.mouseCurrent.pButtons[button] == FALSE;
}

YND b8
InputWasMouseButtonDown(MouseButtons button) 
{
    if (!gbInitialized) 
	{
        return FALSE;
    }
    return gState.mousePrevious.pButtons[button] == TRUE;
}

YND b8
InputWasMouseButtonUp(MouseButtons button) 
{
    if (!gbInitialized) 
	{
        return TRUE;
    }
    return gState.mousePrevious.pButtons[button] == FALSE;
}

void
InputGetMousePosition(int32_t* pX, int32_t* pY) 
{
    if (!gbInitialized) 
	{
        *pX = 0;
        *pY = 0;
        return;
    }
    *pX = gState.mouseCurrent.x;
    *pY = gState.mouseCurrent.y;
}

void
InputGetPreviousMousePosition(int32_t* pX, int32_t* pY) 
{
    if (!gbInitialized) 
	{
        *pX = 0;
        *pY = 0;
        return;
    }
    *pX = gState.mousePrevious.x;
    *pY = gState.mousePrevious.y;
}
