#include "os.h"

#ifdef YPLATFORM_LINUX
#	include "core/event.h"
#	include "core/input.h"
#	include "core/logger.h"
#	include "core/assert.h"
#	include "xdg-shell-client-protocol.h"

#	include <unistd.h>
#	include <errno.h>
#	include <sys/types.h>
#	include <sys/mman.h>
#	include <sys/stat.h>					/* For mode constants */
#	include <fcntl.h>						/* For O_* constants */
#	include <wayland-client.h>
#	include <vulkan/vulkan_wayland.h>
#	include <stdio.h>
#	include <stdlib.h>
#	include <string.h>
#	include <vulkan/vulkan.h>
#	include <vulkan/vk_enum_string_helper.h>

typedef struct internal_state
{
	struct wl_display *pDisplay;
	struct wl_compositor *pCompositor;
	struct wl_surface *pSurface;
	struct wl_shm *pShm;

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
void RegistryRemover(void *pData, [[maybe_unused]]struct wl_registry *pRegistry, [[maybe_unused]]uint32_t id);
void ErrorExit(char *pMsg, unsigned long dw);

void HandleConfigure([[maybe_unused]]void *pData, struct xdg_toplevel *pTopLevel, int32_t w, int32_t h,
		struct wl_array *pStates);

static struct xdg_wm_base_listener gWMBaseListener = {
	.ping = HandleWMBasePing,
};

static struct xdg_toplevel_listener gTopLevelListener = {
	.configure = HandleConfigure, // resize here
	.close = HandleTopLevelClose
};

void
LogErrnoExit(char *pFile, int line)
{
	YFATAL("%s:%d||%s",  pFile, line, strerror(errno));
	exit(1);
}

#define ShmExitClose(shmName)			\
		do { \
			YFATAL("%s:%d||%s",  __FILE__, __LINE__, strerror(errno)); \
			int errcode2 = shm_unlink(shmName); \
			if (errcode2 < 0) \
			{ \
				YFATAL("%s:%d||%s",  __FILE__, __LINE__, strerror(errno)); \
			} \
			exit(1); \
		} while (0)

[[nodiscard]] b8 
OS_Init(OS_State *pState, const char *pAppName, int32_t x, int32_t y, int32_t w, int32_t h)
{
	// TODO: Check errors !!

	(void)pAppName;

	pState->pInternalState = calloc(1, sizeof(InternalState));
	InternalState *state = (InternalState *)pState->pInternalState;

	state->pDisplay = wl_display_connect(NULL);
	if (!state->pDisplay) ErrorExit("wl_display_connect", 1);

    struct wl_registry *pRegistry = wl_display_get_registry(state->pDisplay);

	struct wl_registry_listener registryListener = {
		.global = RegistryListener,
		.global_remove = RegistryRemover
	};
	wl_registry_add_listener(pRegistry, &registryListener, state);

	wl_display_dispatch(state->pDisplay);
	wl_display_roundtrip(state->pDisplay);

	state->pSurface = wl_compositor_create_surface(state->pCompositor);
	if (!state->pSurface) 
		ErrorExit("wl_compositor_create_surface", 1);
	YINFO("Surface created !");

	state->pXDGSurface = xdg_wm_base_get_xdg_surface(state->pWMBase, state->pSurface);
	state->pXDGTopLevel = xdg_surface_get_toplevel(state->pXDGSurface);

	xdg_toplevel_add_listener(state->pXDGTopLevel, &gTopLevelListener, state);
	xdg_wm_base_add_listener(state->pWMBase, &gWMBaseListener, state);


	wl_surface_commit(state->pSurface);
	YDEBUG("Surface commited !");

	[[maybe_unused]]struct wl_shm_pool *pPool;
	[[maybe_unused]]struct wl_buffer *pBuffer;
	[[maybe_unused]] void *pData;

	int errcode = 0;
	int stride = w * 4;
	int size = stride * h;
	int fd = -1;
	char *pShmName = "/ChichiTmp";

	fd = shm_open(pShmName, O_RDWR | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR);
	if (fd < 0)
	{
		if (errno == EEXIST)
			ShmExitClose(pShmName);
		LogErrnoExit(__FILE__, __LINE__);
	}

	errcode = ftruncate(fd, size);
	if (errcode < 0)
		ShmExitClose(pShmName);
	YDEBUG("Shared memory buffer opened !");

	/* TODO: Don't forget to unmap and unlink ! */

    /*
	 * pData = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	 * if (pData == MAP_FAILED)
	 * 	ShmExitClose(pShmName);
     */

	if (!state->pShm)
		ErrorExit("Could not get wl_shm interface on time", 1);

	YDEBUG("Wl_shm interface available !");
	pPool = wl_shm_create_pool(state->pShm, fd, size);
	YDEBUG("Wl_shm pool created !");
	pBuffer = wl_shm_pool_create_buffer(pPool, 0, w, h, stride, WL_SHM_FORMAT_ABGR8888);
	YDEBUG("Wl_shm pool buffer created !");

	wl_surface_attach(state->pSurface, pBuffer, x, y);
	YDEBUG("Wl_shm surface attached !");
	wl_surface_damage(state->pSurface, x, y, UINT32_MAX, UINT32_MAX);
	YDEBUG("Wl_shm surface damaged !");
	wl_surface_commit(state->pSurface);
	YDEBUG("Wl_shm surface committed !");

	return TRUE;
}
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

#include <time.h>
[[nodiscard]] f64
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
RegistryListener(void *pData, struct wl_registry *pRegistry, uint32_t id, const char *pInterface,
		[[maybe_unused]] uint32_t version)
{
	InternalState *state = (InternalState *)pData;
	if (strcmp(pInterface, wl_compositor_interface.name) == 0)
		state->pCompositor = wl_registry_bind(pRegistry, id, &wl_compositor_interface, 4);
	else if (strcmp(pInterface, xdg_wm_base_interface.name) == 0) 
		state->pWMBase = wl_registry_bind(pRegistry, id, &xdg_wm_base_interface, 1);
	else if (strcmp(pInterface, wl_shm_interface.name) == 0) 
		state->pShm = wl_registry_bind(pRegistry, id, &wl_shm_interface, 1);
}

void
RegistryRemover(void *pData, [[maybe_unused]]struct wl_registry *pRegistry, [[maybe_unused]]uint32_t id)
{
	[[maybe_unused]]InternalState *state = (InternalState *)pData;
}

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
	xdg_wm_base_pong(pBase, serial);
}

void
HandleTopLevelClose([[maybe_unused]]void *pData, [[maybe_unused]]struct xdg_toplevel *pTopLevel)
{
	gRunning = FALSE;
}

void PopupDoneListener([[maybe_unused]]void *pData, [[maybe_unused]]struct wl_shell_surface *pShellSurface)
{
}

void PingListener([[maybe_unused]]void *pData, [[maybe_unused]]struct wl_shell_surface *pShellSurface,
		[[maybe_unused]] uint32_t serial)
{

}

void ConfigureListener([[maybe_unused]] void *pData,[[maybe_unused]] struct wl_shell_surface *pShellSurface, 
		[[maybe_unused]] uint32_t edges, [[maybe_unused]] int32_t w, [[maybe_unused]] int32_t h)
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
