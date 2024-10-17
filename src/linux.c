#include "os.h"

#ifdef YPLATFORM_LINUX
#	include "core/event.h"
#	include "core/input.h"
#	include "core/logger.h"
#	include "core/assert.h"

#	include "xdg-shell-client-protocol.h"
#	include <wayland-client.h>
#	include <vulkan/vulkan_wayland.h>
#	include <stdio.h>
#	include <stdlib.h>
#	include <unistd.h>
#	include <string.h>
#	include <vulkan/vulkan.h>
#	include <vulkan/vk_enum_string_helper.h>

void ErrorExit(char *pMsg, unsigned long dw);

typedef struct internal_state
{
	struct wl_display *pDisplay;
	struct wl_compositor *pCompositor;
	struct wl_surface *pSurface;

	struct xdg_surface *pXDGSurface;
	struct xdg_toplevel *pXDGTopLevel;
	struct xdg_wm_base *pWMBase;
	struct xdg_popup *pPopup;

	struct wl_shell *pShell;
	struct wl_shell_surface *pShellSurface;
	VkSurfaceKHR vkSurface;

    /*
	 * HINSTANCE hInstance;
	 * HWND hwnd;
     */
}InternalState;

void PopupDoneListener(void *pData, struct wl_shell_surface *pShellSurface);
void PingListener(void *pData, struct wl_shell_surface *pShellSurface, uint32_t serial);
void ConfigureListener(void *pData, struct wl_shell_surface *pShellSurface, uint32_t edges, int32_t w, int32_t h);
void RegistryListener(void *pData, struct wl_registry *pRegistry, uint32_t id, const char *pInterface, uint32_t ver);
void HandleWMBasePing(void *pData, struct xdg_wm_base *pBase, uint32_t serial);
void HandleTopLevelClose(void *pData, struct xdg_toplevel *pTopLevel);

void HandleConfigure([[maybe_unused]]void *pData, struct xdg_toplevel *pTopLevel, int32_t w, int32_t h,
		struct wl_array *pStates);

static struct xdg_wm_base_listener gWMBaseListener = {
	.ping = HandleWMBasePing,
};

static struct xdg_toplevel_listener gTopLevelListener = {
	.configure = NULL, // resize here
	.close = HandleTopLevelClose
};

[[nodiscard]] b8 
OS_Init(OS_State *pState, const char *pAppName, int32_t x, int32_t y, int32_t w, int32_t h)
{
	/* WARN: Check errors !! */

	(void)pAppName; (void)x; (void)y; (void)w; (void)h;

	pState->pInternalState = calloc(1, sizeof(InternalState));
	InternalState *state = (InternalState *)pState->pInternalState;

	state->pDisplay = wl_display_connect(NULL);
	if (!state->pDisplay) ErrorExit("wl_display_connect", 1);

    struct wl_registry *pRegistry = wl_display_get_registry(state->pDisplay);

	struct wl_registry_listener registryListener = {
		.global = RegistryListener
	};
	wl_registry_add_listener(pRegistry, &registryListener, state);

	wl_display_dispatch(state->pDisplay);
	wl_display_roundtrip(state->pDisplay);

	state->pSurface = wl_compositor_create_surface(state->pCompositor);
	if (!state->pSurface) ErrorExit("wl_compositor_create_surface", 1);
	YINFO("Surface created !");

	state->pXDGSurface = xdg_wm_base_get_xdg_surface(state->pWMBase, state->pSurface);
	state->pXDGTopLevel = xdg_surface_get_toplevel(state->pXDGSurface);
	xdg_toplevel_add_listener(state->pXDGTopLevel, &gTopLevelListener, state);
	xdg_wm_base_add_listener(state->pWMBase, &gWMBaseListener, state);
	wl_surface_commit(state->pSurface);
	YDEBUG("Surface commited !");

		/*
		 * 
		 * 
		 * 
		 * struct wl_shell_surface_listener shellListener = {
		 * 	.ping = PingListener,
		 * 	.configure = ConfigureListener,
		 * 	.popup_done = PopupDoneListener,
		 * };
		 * state->pShellSurface = wl_shell_get_shell_surface(state->pShell, state->pSurface);
		 * wl_shell_surface_set_toplevel(state->pShellSurface);
		 * 
		 * #<{(| TODO: Why NULL here ? |)}>#
		 * wl_shell_surface_add_listener(state->pShellSurface, &shellListener, NULL);
		 */

	return TRUE;
}

void 
OS_Shutdown(OS_State *pState)
{
	InternalState *state = (InternalState *)pState;
	wl_display_disconnect(state->pDisplay);
}

[[nodiscard]] VkResult
OS_CreateVkSurface(OS_State *pState, VkContext *pCtx)
{
	InternalState *pNewState = (InternalState *)pState->pInternalState;

	VkWaylandSurfaceCreateInfoKHR createInfo = {
		.sType = VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR,
		.display = pNewState->pDisplay,
		.surface = pNewState->pSurface,
	};
	VK_CHECK(vkCreateWaylandSurfaceKHR(pCtx->instance, &createInfo, pCtx->pAllocator, &pNewState->vkSurface));
	pCtx->surface = pNewState->vkSurface;
	return VK_SUCCESS;
}

b8
OS_PumpMessages(OS_State *pState) 
{
	InternalState *state = (InternalState*) pState;
	wl_display_dispatch_pending(state->pDisplay);
	wl_display_flush(state->pDisplay);
	wl_display_dispatch(state->pDisplay);
    return TRUE;
}

void
OS_Write(const char *pMessage, REDIR redir)
{
	size_t msgLength = strlen(pMessage);
	write(redir, pMessage, msgLength);
}

void
ErrorMsgBox(char *pMsg, unsigned long dw)
{
	(void)dw;
	YFATAL("%s", pMsg);
}

void
ErrorExit(char *pMsg, unsigned long dw) 
{
	ErrorMsgBox(pMsg, dw);
	exit((int)dw);
}

[[nodiscard]] f64
OS_GetAbsoluteTime(void)
{
	return 0;
}

void
OS_Sleep(uint64_t ms)
{
	sleep(ms / 1000);
}

/*
 * void
 * RegistryListener(void *pData, struct wl_registry *pRegistry, uint32_t id, const char *pInterface, uint32_t version)
 * {
 * 	(void)version;
 * 	InternalState *state = (InternalState *)pData;
 * 	if (strcmp(pInterface, "wl_compositor") == 0) 
 * 		state->pCompositor = wl_registry_bind(pRegistry, id, &wl_compositor_interface, 1);
 * 	else if (strcmp(pInterface, "wl_shell") == 0) 
 * 		state->pShell = wl_registry_bind(pRegistry, id, &wl_shell_interface, 1);
 * }
 */

void 
RegistryListener(void *data, struct wl_registry *registry, uint32_t id, const char *interface,
		[[maybe_unused]] uint32_t version)
{
	InternalState *state = (InternalState *)data;
	if (strcmp(interface, "wl_compositor") == 0)
		state->pCompositor = wl_registry_bind(registry, id, &wl_compositor_interface, 1);
	else if (strcmp(interface, "xdg_wm_base") == 0) 
		state->pWMBase = wl_registry_bind(registry, id, &xdg_wm_base_interface, 1);
}

/*
 * void 
 * HandlePopupDone(void *pData, struct xdg_popup *xdg_popup)
 * {
 *     InternalState *state = (InternalState *)pData;
 *     // Handle the popup being dismissed.
 *     printf("Popup done, should hide or destroy the popup.\n");
 *     // If popup surface was created, this would be a good time to clean it up.
 *     if (state->pPopup)
 * 	{
 *         wl_surface_destroy(state->pPopup);
 *         state->pPopup = NULL;
 *     }
 * }
 */

void
HandleConfigure([[maybe_unused]]void *pData, [[maybe_unused]]struct xdg_toplevel *pTopLevel,
		int32_t width, int32_t height, [[maybe_unused]]struct wl_array *pStates)
{
	InternalState *state = (InternalState *)pData;
	if (width > 0 && height > 0)
		YINFO("Window resized to %d x %d", width, height);
	wl_surface_commit(state->pSurface);
}

void
HandleWMBasePing([[maybe_unused]] void *pData, struct xdg_wm_base *pBase, uint32_t serial)
{
	(void)pData;
	xdg_wm_base_pong(pBase, serial);
}

void
HandleTopLevelClose([[maybe_unused]]void *pData, [[maybe_unused]]struct xdg_toplevel *pTopLevel)
{
	(void)pData;
	gRunning = FALSE;
}

void PopupDoneListener([[maybe_unused]]void *pData, [[maybe_unused]]struct wl_shell_surface *pShellSurface)
{
}

void PingListener([[maybe_unused]]void *pData, [[maybe_unused]]struct wl_shell_surface *pShellSurface,
		[[maybe_unused]] uint32_t serial)
{
	(void)pData;

}

void ConfigureListener([[maybe_unused]] void *pData,[[maybe_unused]] struct wl_shell_surface *pShellSurface, 
		[[maybe_unused]] uint32_t edges, [[maybe_unused]] int32_t w, [[maybe_unused]] int32_t h)
{

}

#endif // YPLATFORM_LINUX
