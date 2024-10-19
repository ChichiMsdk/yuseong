#ifdef PLATFORM_LINUX
#include "pointers.h"
#include "internals.h"

void
WlPointerEnter(void *pData, YMB struct wl_pointer *wl_pointer, uint32_t serial, 
		YMB struct wl_surface *surface, wl_fixed_t surfaceX, wl_fixed_t surfaceY)
{
       InternalState *pState = (InternalState *) pData;
       pState->pointerEvent.eventMask |= POINTER_EVENT_ENTER;
       pState->pointerEvent.serial = serial;
       pState->pointerEvent.surfaceX = surfaceX;
	   pState->pointerEvent.surfaceY = surfaceY;
}

void
WlPointerLeave(void *data, YMB struct wl_pointer *wl_pointer, uint32_t serial, 
		YMB struct wl_surface *surface)
{
       InternalState *pState = data;
       pState->pointerEvent.serial = serial;
       pState->pointerEvent.eventMask |= POINTER_EVENT_LEAVE;
}

void
WlPointerMotion(void *data, YMB struct wl_pointer *wl_pointer, uint32_t time, wl_fixed_t surfaceX, 
		wl_fixed_t surfaceY)
{
       InternalState *pState = data;
       pState->pointerEvent.eventMask |= POINTER_EVENT_MOTION;
       pState->pointerEvent.time = time;
       pState->pointerEvent.surfaceX = surfaceX,
               pState->pointerEvent.surfaceY = surfaceY;
}

void
WlPointerButton(void *data, YMB struct wl_pointer *wl_pointer, uint32_t serial, uint32_t time, 
		uint32_t button, uint32_t state)
{
       InternalState *pState = data;
       pState->pointerEvent.eventMask |= POINTER_EVENT_BUTTON;
       pState->pointerEvent.time = time;
       pState->pointerEvent.serial = serial;
       pState->pointerEvent.button = button,
               pState->pointerEvent.state = state;
}

void
WlPointerAxis(void *data, YMB struct wl_pointer *wl_pointer, uint32_t time, uint32_t axis, wl_fixed_t value)
{
       InternalState *pState = data;
       pState->pointerEvent.eventMask |= POINTER_EVENT_AXIS;
       pState->pointerEvent.time = time;
       pState->pointerEvent.axes[axis].valid = true;
       pState->pointerEvent.axes[axis].value = value;
}

void
WlPointerAxisSource(void *data, YMB struct wl_pointer *wl_pointer, uint32_t axisSource)
{
       InternalState *pState = data;
       pState->pointerEvent.eventMask |= POINTER_EVENT_AXIS_SOURCE;
       pState->pointerEvent.axisSource = axisSource;
}

void
WlPointerAxisStop(void *data,YMB  struct wl_pointer *wl_pointer, uint32_t time, uint32_t axis)
{
       InternalState *pState = data;
       pState->pointerEvent.time = time;
       pState->pointerEvent.eventMask |= POINTER_EVENT_AXIS_STOP;
       pState->pointerEvent.axes[axis].valid = true;
}

void
WlPointerAxisDiscrete(void *data,YMB  struct wl_pointer *wl_pointer, uint32_t axis, int32_t discrete)
{
       InternalState *pState = data;
       pState->pointerEvent.eventMask |= POINTER_EVENT_AXIS_DISCRETE;
       pState->pointerEvent.axes[axis].valid = true;
       pState->pointerEvent.axes[axis].discrete = discrete;
}

void
WlPointerFrame(void *data, YMB struct wl_pointer *wl_pointer)
{
       InternalState *pState = data;
       struct PointerEvent *event = &pState->pointerEvent;
       YDEBUG("Pointer frame @ %d: ", event->time);

       if (event->eventMask & POINTER_EVENT_ENTER) 
	   {
               fprintf(stderr, "entered %f, %f ",
                               wl_fixed_to_double(event->surfaceX),
                               wl_fixed_to_double(event->surfaceY));
       }

       if (event->eventMask & POINTER_EVENT_LEAVE) 
	   {
               fprintf(stderr, "leave");
       }

       if (event->eventMask & POINTER_EVENT_MOTION)
	   {
               fprintf(stderr, "motion %f, %f ",
                               wl_fixed_to_double(event->surfaceX),
                               wl_fixed_to_double(event->surfaceY));
       }

       if (event->eventMask & POINTER_EVENT_BUTTON) 
	   {
               char *state = event->state == WL_POINTER_BUTTON_STATE_RELEASED ?
                       "released" : "pressed";
               fprintf(stderr, "button %d %s ", event->button, state);
       }

       uint32_t axis_events = POINTER_EVENT_AXIS
               | POINTER_EVENT_AXIS_SOURCE
               | POINTER_EVENT_AXIS_STOP
               | POINTER_EVENT_AXIS_DISCRETE;
       char *axis_name[2] = {
               [WL_POINTER_AXIS_VERTICAL_SCROLL] = "vertical",
               [WL_POINTER_AXIS_HORIZONTAL_SCROLL] = "horizontal",
       };
       char *axisSource[4] = {
               [WL_POINTER_AXIS_SOURCE_WHEEL] = "wheel",
               [WL_POINTER_AXIS_SOURCE_FINGER] = "finger",
               [WL_POINTER_AXIS_SOURCE_CONTINUOUS] = "continuous",
               [WL_POINTER_AXIS_SOURCE_WHEEL_TILT] = "wheel tilt",
       };
       if (event->eventMask & axis_events) 
	   {
               for (size_t i = 0; i < 2; ++i) 
			   {
                       if (!event->axes[i].valid) 
					   {
                               continue;
                       }
                       fprintf(stderr, "%s axis ", axis_name[i]);
                       if (event->eventMask & POINTER_EVENT_AXIS) 
					   {
                               fprintf(stderr, "value %f ", wl_fixed_to_double( event->axes[i].value));
                       }
                       if (event->eventMask & POINTER_EVENT_AXIS_DISCRETE) 
					   {
                               fprintf(stderr, "discrete %d ", event->axes[i].discrete);
                       }
                       if (event->eventMask & POINTER_EVENT_AXIS_SOURCE) 
					   {
                               fprintf(stderr, "via %s ", axisSource[event->axisSource]);
                       }
                       if (event->eventMask & POINTER_EVENT_AXIS_STOP) 
					   {
                               fprintf(stderr, "(stopped) ");
                       }
               }
       }
       fprintf(stderr, "\n");
       memset(event, 0, sizeof(*event));
}

#endif //PLATFORM_LINUX
