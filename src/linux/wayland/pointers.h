#ifndef POINTERS_H
#define POINTERS_H

#include <wayland-client.h>
#include "xdg-shell-client-protocol.h"
#include <stdbool.h>

extern const struct wl_pointer_listener gWlPointerListener;

enum PointerEventMask 
{
       POINTER_EVENT_ENTER = 1 << 0,
       POINTER_EVENT_LEAVE = 1 << 1,
       POINTER_EVENT_MOTION = 1 << 2,
       POINTER_EVENT_BUTTON = 1 << 3,
       POINTER_EVENT_AXIS = 1 << 4,
       POINTER_EVENT_AXIS_SOURCE = 1 << 5,
       POINTER_EVENT_AXIS_STOP = 1 << 6,
       POINTER_EVENT_AXIS_DISCRETE = 1 << 7,
};

struct PointerEvent 
{
       uint32_t eventMask;
       wl_fixed_t surfaceX, surfaceY;
       uint32_t button, state;
       uint32_t time;
       uint32_t serial;
       struct {
               bool valid;
               wl_fixed_t value;
               int32_t discrete;
       } axes[2];
       uint32_t axisSource;
};

void WlPointerLeave(void *data, struct wl_pointer *wl_pointer, uint32_t serial, struct wl_surface *surface);
void WlPointerEnter(void *pData, struct wl_pointer *wl_pointer, uint32_t serial, struct wl_surface *surface, wl_fixed_t surface_x, wl_fixed_t surface_y);
void WlPointerMotion(void *data, struct wl_pointer *wl_pointer, uint32_t time, wl_fixed_t surface_x, wl_fixed_t surface_y);
void WlPointerButton(void *data, struct wl_pointer *wl_pointer, uint32_t serial, uint32_t time, uint32_t button, uint32_t state);
void WlPointerAxis(void *data, struct wl_pointer *wl_pointer, uint32_t time, uint32_t axis, wl_fixed_t value);
void WlPointerAxisSource(void *data, struct wl_pointer *wl_pointer, uint32_t axis_source);
void WlPointerAxisStop(void *data, struct wl_pointer *wl_pointer, uint32_t time, uint32_t axis);
void WlPointerAxisDiscrete(void *data, struct wl_pointer *wl_pointer, uint32_t axis, int32_t discrete);
void WlPointerFrame(void *data, struct wl_pointer *wl_pointer);

#endif // POINTERS_H
