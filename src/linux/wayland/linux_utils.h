#ifndef LINUX_UTILS_H
#define LINUX_UTILS_H

#ifdef PLATFORM_LINUX

#include <unistd.h>
#include <string.h>

#include <wayland-client.h>

#include "xdg-shell-client-protocol.h"
#include "core/logger.h"

#include "pointers.h"
#include "keyboard.h"

void WlSeatCapabilities(
		void*								pData,
		struct wl_seat*						pWlSeat,
		uint32_t							capabilities);

void WlSeatName(
		void*								pData,
		struct wl_seat*						pWlSeat,
		const char*							pName);

void PopupDoneListener(
		void*								pData,
		struct wl_shell_surface*			pShellSurface);

void PingListener(
		void*								pData,
		struct wl_shell_surface*			pShellSurface,
		uint32_t							serial);

void ConfigureListener(
		void*								pData,
		struct wl_shell_surface*			pShellSurface, 
		uint32_t							edges, 
		int32_t								width, 
		int32_t								height);

void RegistryListener(
		void*								pData,
		struct wl_registry*					pRegistry,
		uint32_t							id,
		const char*							pInterface,
		uint32_t							version);

void HandleWmBasePing(
		void*								pData,
		struct xdg_wm_base*					pBase,
		uint32_t							serial);

void HandleTopLevelClose(
		void*								pData,
		struct xdg_toplevel*				pTopLevel);

void RegistryRemover(
		void*								pData,
		YMB struct wl_registry*	pRegistry,
		YMB uint32_t			id);

void ErrorExit(
		char*								pMsg,
		unsigned long						errcode);

void WlBufferRelease(
		YMB void*				pData,
		struct wl_buffer*					pWlBuffer);

void XdgSurfaceConfigure(
		void*								pData,
		struct xdg_surface*					pSurface,
		uint32_t							serial);

void HandleConfigure(
		YMB void*				pData,
		struct xdg_toplevel*				pTopLevel,
		int32_t								width,
		int32_t								height,
		struct wl_array*					pStates);


int ShmFileAllocate(
		size_t								size);

void LogErrnoExit(
		char*								pFile,
		int									line);

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

#endif //LINUX
#endif  // LINUX_UTILS_H
