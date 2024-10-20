#include "os.h"

#ifdef YPLATFORM_LINUX

#	include "internals.h"

#	include "core/event.h"
#	include "core/input.h"
#	include "core/logger.h"
#	include "core/assert.h"
#	include <time.h>
#	include <errno.h>
#	include <sys/types.h>
#	include <sys/mman.h>
#	include <sys/stat.h>					/* For mode constants */
#	include <fcntl.h>						/* For O_* constants */
#	include <stdio.h>
#	include <stdlib.h>

struct wl_buffer* FrameDraw(
		InternalState*						State);

static const struct wl_keyboard_listener gWlKeyboardListener = {
		.keymap =							WlKeyboardKeymap,
		.enter =							WlKeyboardEnter,
		.leave =							WlKeyboardLeave,
		.key =								WlKeyboardKey,
		.modifiers =						WlKeyboardModifiers,
		.repeat_info =						WlKeyboardRepeatInfo,
};

const struct wl_pointer_listener gWlPointerListener = {
		.enter =							WlPointerEnter,
		.leave =							WlPointerLeave,
		.motion =							WlPointerMotion,
		.button =							WlPointerButton,
		.axis =								WlPointerAxis,
		.frame =							WlPointerFrame,
		.axis_source =						WlPointerAxisSource,
		.axis_stop =						WlPointerAxisStop,
		.axis_discrete =					WlPointerAxisDiscrete };

static const struct wl_buffer_listener gWlBufferListener = {
		.release =							WlBufferRelease, };

static struct xdg_surface_listener gXdgSurfaceListener = {
		.configure =						XdgSurfaceConfigure };

static struct xdg_wm_base_listener gWmBaseListener = {
		.ping =								HandleWmBasePing, };

static struct wl_seat_listener gWlSeatListener = {
		.capabilities =						WlSeatCapabilities,
		.name =								WlSeatName };

YMB static struct xdg_toplevel_listener gTopLevelListener = {
		.configure =						HandleConfigure, // resize here
		.close =							HandleTopLevelClose };

struct wl_registry_listener gRegistryListener = {
		.global =							RegistryListener,
		.global_remove =					RegistryRemover };

YND b8 
OS_Init(OS_State *pOsState, const char *pAppName, int32_t x, int32_t y, int32_t w, int32_t h)
{
	// TODO: Check errors !!
	// TODO: export WAYLAND_DEBUG=1

	pOsState->pInternalState = calloc(1, sizeof(InternalState));
	InternalState *pState = (InternalState *)pOsState->pInternalState;
	pState->x = x;
	pState->y = y;
	pState->width = w;
	pState->height = h;

	pState->pDisplay = wl_display_connect(NULL);
	if (!pState->pDisplay) ErrorExit("wl_display_connect", 1);

    pState->pRegistry = wl_display_get_registry(pState->pDisplay);
	if (!pState->pRegistry) ErrorExit("wl_display_get_registry", 1);

	//NOTE: Maybe put that in "InputInit()" ?
	//TODO: free this
	pState->pXkbContext = xkb_context_new(XKB_CONTEXT_NO_FLAGS);

	wl_registry_add_listener(pState->pRegistry, &gRegistryListener, pState);
	wl_display_roundtrip(pState->pDisplay);

	pState->pSurface = wl_compositor_create_surface(pState->pCompositor);
	if (!pState->pSurface) ErrorExit("wl_compositor_create_surface", 1);

	pState->pXdgSurface = xdg_wm_base_get_xdg_surface(pState->pWmBase, pState->pSurface);

	xdg_surface_add_listener(pState->pXdgSurface, &gXdgSurfaceListener, pState);

	pState->pXdgTopLevel = xdg_surface_get_toplevel(pState->pXdgSurface);
	xdg_toplevel_add_listener(pState->pXdgTopLevel, &gTopLevelListener, pState);

	xdg_toplevel_set_title(pState->pXdgTopLevel, pAppName);
	wl_surface_commit(pState->pSurface);

	return TRUE;
}

void 
OS_Shutdown(OS_State *pOsState)
{
	InternalState *pState = (InternalState *) pOsState->pInternalState;
	wl_display_disconnect(pState->pDisplay);
}

struct wl_buffer *
FrameDraw(InternalState *pState)
{
	const int width = pState->width;
	const int height = pState->height;
	int stride = width * 4;
	int size = stride * height;

	int fd = ShmFileAllocate(size);
	if (fd == -1) return NULL;

	uint32_t *pData = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (pData == MAP_FAILED) 
	{
		close(fd);
		return NULL;
	}

	struct wl_shm_pool *pPool = wl_shm_create_pool(pState->pShm, fd, size);
	struct wl_buffer *pBuffer = wl_shm_pool_create_buffer(pPool, 0, width, height, stride, WL_SHM_FORMAT_XRGB8888);
	wl_shm_pool_destroy(pPool);
	close(fd);

	/* Draw checkerboxed background */
	for (int y = 0; y < height; ++y) 
	{
		for (int x = 0; x < width; ++x) 
		{
			if ((x + y / 8 * 8) % 16 < 8)
				pData[y * width + x] = 0xFF666666;
			else
				pData[y * width + x] = 0xFFEEEEEE;
		}
	}

	munmap(pData, size);
	wl_buffer_add_listener(pBuffer, &gWlBufferListener, NULL);
	return pBuffer;
}

YND VkResult
OS_CreateVkSurface(OS_State *pOsState, VkContext *pVkCtx)
{
	InternalState *pState = (InternalState *)pOsState->pInternalState;

	VkWaylandSurfaceCreateInfoKHR createInfo = {
		.sType = VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR,
		.display = pState->pDisplay,
		.surface = pState->pSurface,
	};
	VK_CHECK(vkCreateWaylandSurfaceKHR(pVkCtx->instance, &createInfo, pVkCtx->pAllocator, &pState->vkSurface));
	pVkCtx->surface = pState->vkSurface;
	return VK_SUCCESS;
}

b8
OS_PumpMessages(OS_State *pOsState) 
{
	InternalState *pState = (InternalState*) pOsState->pInternalState;
	if (pState->pDisplay == NULL)
		ErrorExit("state->pDisplay == NULL", 1);
	wl_display_dispatch_pending(pState->pDisplay);
	wl_display_flush(pState->pDisplay);
	wl_display_dispatch(pState->pDisplay);
    return TRUE;
}

void
OS_Write(const char *pMessage, REDIR redir)
{
	size_t msgLength = strlen(pMessage);
	write(redir, pMessage, msgLength);
}

void
ErrorMsgBox(char *pMsg, YMB  unsigned long dw)
{
	YFATAL("%s", pMsg);
}

void
ErrorExit(char *pMsg, unsigned long dw) 
{
	ErrorMsgBox(pMsg, dw);
	exit((int)dw);
}

YND f64
OS_GetAbsoluteTime(void)
{
	struct timespec tp;
	clock_gettime(CLOCK_REALTIME, &tp);
	return tp.tv_nsec;
}

void
OS_Sleep(uint64_t ms)
{
	sleep(ms / 1000);
}

void 
WlSeatCapabilities(YMB void* pData, YMB struct wl_seat* pWlSeat,
		YMB uint32_t capabilities)
{
	InternalState *pState = (InternalState *) pData;
	b8 bHasPointer = capabilities & WL_SEAT_CAPABILITY_POINTER;
	if (bHasPointer && pState->pWlPointer == NULL)
	{
		pState->pWlPointer = wl_seat_get_pointer(pState->pWlSeat);
		wl_pointer_add_listener(pState->pWlPointer, &gWlPointerListener, pState);
	}
	else if (!bHasPointer && pState->pWlPointer != NULL)
	{
		wl_pointer_release(pState->pWlPointer);
		pState->pWlPointer = NULL;
	}

	b8 bHasKeyboard = capabilities & WL_SEAT_CAPABILITY_KEYBOARD;
	if (bHasKeyboard && pState->pWlKeyboard == NULL) 
	{
		pState->pWlKeyboard = wl_seat_get_keyboard(pState->pWlSeat);
		wl_keyboard_add_listener(pState->pWlKeyboard, &gWlKeyboardListener, pState);
	}
	else if (!bHasKeyboard && pState->pWlKeyboard != NULL)
	{
		wl_keyboard_release(pState->pWlKeyboard);
		pState->pWlKeyboard = NULL;
	}
}

void 
WlSeatName(YMB void* pData, YMB struct wl_seat* pWlSeat, const char* pName)
{
	YDEBUG("Seat name: %s", pName);
}

void 
RegistryListener(void *pData, struct wl_registry *pRegistry, uint32_t id, const char *pInterface,
		YMB  uint32_t version)
{
	InternalState *pState = (InternalState *)pData;
	if (strcmp(pInterface, wl_compositor_interface.name) == 0)
		pState->pCompositor = wl_registry_bind(pRegistry, id, &wl_compositor_interface, 4);
	else if (strcmp(pInterface, wl_shm_interface.name) == 0) 
		pState->pShm = wl_registry_bind(pRegistry, id, &wl_shm_interface, 1);
	else if (strcmp(pInterface, xdg_wm_base_interface.name) == 0)
	{
		pState->pWmBase = wl_registry_bind(pRegistry, id, &xdg_wm_base_interface, 1);
		xdg_wm_base_add_listener(pState->pWmBase, &gWmBaseListener, pState);
	}
	else if (strcmp(pInterface, wl_seat_interface.name) == 0)
	{
		pState->pWlSeat = wl_registry_bind(pRegistry, id, &wl_seat_interface, 7);
		wl_seat_add_listener(pState->pWlSeat, &gWlSeatListener, pState);
	}
}

void
RegistryRemover(void *pData, YMB struct wl_registry *pRegistry, YMB uint32_t id)
{
	YMB InternalState *pState = (InternalState *)pData;
}

void
HandleConfigure(YMB void *pData, YMB struct xdg_toplevel *pTopLevel,
		int32_t width, int32_t height, YMB struct wl_array *pStates)
{
	InternalState *pState = (InternalState *)pData;
	if (width > 0 && height > 0)
	{
		pState->width = width;
		pState->height = height;
		YINFO("Window resized to %d x %d", width, height);
	}
	wl_surface_commit(pState->pSurface);
}

void
WlBufferRelease(YMB void *pData, struct wl_buffer *pWlBuffer)
{
    /* Sent by the compositor when it's no longer using this buffer */
    wl_buffer_destroy(pWlBuffer);
}

void
XdgSurfaceConfigure(void *pData, struct xdg_surface *pSurface, uint32_t serial)
{
	InternalState *pState = pData;
	xdg_surface_ack_configure(pSurface, serial);

	struct wl_buffer *pBuffer = FrameDraw(pState);
	wl_surface_attach(pState->pSurface, pBuffer, 0, 0);
	wl_surface_commit(pState->pSurface);
}

void
HandleWmBasePing(YMB  void *pData, struct xdg_wm_base *pXdgWmBase, uint32_t serial)
{
	xdg_wm_base_pong(pXdgWmBase, serial);
}

void
HandleTopLevelClose(YMB void *pData, YMB struct xdg_toplevel *pXdgTopLevel)
{
	gRunning = FALSE;
}

void 
PopupDoneListener(YMB void *pData, YMB struct wl_shell_surface *pShellSurface)
{
}

void 
PingListener(YMB void *pData, YMB struct wl_shell_surface *pShellSurface,
		YMB  uint32_t serial)
{

}

void 
ConfigureListener(YMB  void *pData,YMB  struct wl_shell_surface *pShellSurface, 
		YMB  uint32_t edges, YMB  int32_t w, YMB  int32_t h)
{

}

#endif // YPLATFORM_LINUX
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
