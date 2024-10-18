#ifndef INTERNALS_H
#define INTERNALS_H

#	include "linux_utils.h"

#	include <vulkan/vulkan_core.h>
#	include <vulkan/vulkan_wayland.h>
#	include <vulkan/vk_enum_string_helper.h>

typedef struct internal_state
{
	/* Globals */
	struct wl_display *pDisplay;
	struct wl_registry *pRegistry;
	struct wl_compositor *pCompositor;
	struct wl_shm *pShm;
	struct xdg_wm_base *pWmBase;
	struct wl_seat *pWlSeat;

	/* Objects */
	struct wl_surface *pSurface;
	struct xdg_surface *pXdgSurface;
	struct xdg_toplevel *pXdgTopLevel;
	struct wl_keyboard *pWlKeyboard;
	struct wl_pointer *pWlPointer;
	struct wl_touch *pWlTouch;

	struct xdg_popup *pPopup;
	struct wl_shell *pShell;
	struct wl_shell_surface *pShellSurface;

	float offset;
	uint32_t lastFrame;
	int32_t width;
	int32_t height;
	int32_t x;
	int32_t y;

	VkSurfaceKHR vkSurface;
	struct PointerEvent pointerEvent;
	struct xkb_state *pXkbState;
	struct xkb_context *pXkbContext;
	struct xkb_keymap *pXkbKeymap;

    /*
	 * HINSTANCE hInstance;
	 * HWND hwnd;
     */
}InternalState;

#endif // INTERNALS_H
