#ifndef KEYBOARD_H
#define KEYBOARD_H

// https://www.x.org/releases/X11R7.6/doc/libX11/specs/XKB/xkblib.html#acknowledgement
// https://xkbcommon.org/doc/current/
#include <xkbcommon/xkbcommon.h>
#include <wayland-client.h>

void WlKeyboardKeymap(void *data, [[maybe_unused]]struct wl_keyboard *wl_keyboard, uint32_t format, int32_t fd, uint32_t size);
void WlKeyboardEnter(void *data, [[maybe_unused]]struct wl_keyboard *wl_keyboard, [[maybe_unused]]uint32_t serial, [[maybe_unused]]struct wl_surface *surface, struct wl_array *keys);
void WlKeyboardKey(void *data, [[maybe_unused]]struct wl_keyboard *wl_keyboard, [[maybe_unused]]uint32_t serial, [[maybe_unused]]uint32_t time, uint32_t key, uint32_t state);
void WlKeyboardLeave([[maybe_unused]]void *data, [[maybe_unused]]struct wl_keyboard *wl_keyboard, [[maybe_unused]]uint32_t serial, [[maybe_unused]]struct wl_surface *surface);
void WlKeyboardModifiers(void *data, [[maybe_unused]]struct wl_keyboard *wl_keyboard, [[maybe_unused]]uint32_t serial, uint32_t mods_depressed, uint32_t mods_latched, uint32_t mods_locked, uint32_t group);
void WlKeyboardRepeatInfo([[maybe_unused]]void *data, [[maybe_unused]]struct wl_keyboard *wl_keyboard, [[maybe_unused]]int32_t rate, [[maybe_unused]]int32_t delay);

#endif // KEYBOARD_H
