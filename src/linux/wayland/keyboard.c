#ifdef PLATFORM_LINUX

/* https://www.x.org/releases/X11R7.6/doc/libX11/specs/XKB/xkblib.html#acknowledgement */
/* https://xkbcommon.org/doc/current/ */

#include <sys/mman.h>
#include <unistd.h>

#include "core/logger.h"
#include "core/myassert.h"

#include "keyboard.h"
#include "internals.h"
#include <errno.h>

void
WlKeyboardKeymap(void *data, YMB struct wl_keyboard *wl_keyboard, uint32_t format, int32_t fd,
		uint32_t size)
{
       InternalState *pState = data;
       YASSERT(format == WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1);

	   YWARN("size: %lu\n", size);
	   YWARN("fd: %d\n", fd);
       char *map_shm = mmap(NULL, size, PROT_READ, MAP_SHARED, fd, 0);
       YASSERT_MSG(map_shm != MAP_FAILED, strerror(errno));

       struct xkb_keymap *pXkbKeymap = xkb_keymap_new_from_string(pState->pXkbContext, map_shm,
                       XKB_KEYMAP_FORMAT_TEXT_V1, XKB_KEYMAP_COMPILE_NO_FLAGS);
       munmap(map_shm, size);
       close(fd);

       struct xkb_state *pXkbState = xkb_state_new(pXkbKeymap);
       xkb_keymap_unref(pState->pXkbKeymap);
       xkb_state_unref(pState->pXkbState);
       pState->pXkbKeymap = pXkbKeymap;
       pState->pXkbState = pXkbState;
}

void
WlKeyboardEnter(void *data, YMB struct wl_keyboard *wl_keyboard, YMB uint32_t serial, 
		YMB struct wl_surface *surface, struct wl_array *keys)
{
       InternalState *pState = data;
       fprintf(stderr, "keyboard enter; keys pressed are:\n");
       uint32_t *key;
       wl_array_for_each(key, keys)
	   {
               char buf[128];
               xkb_keysym_t sym = xkb_state_key_get_one_sym( pState->pXkbState, *key + 8);
               xkb_keysym_get_name(sym, buf, sizeof(buf));
               fprintf(stderr, "sym: %-12s (%d), ", buf, sym);
               xkb_state_key_get_utf8(pState->pXkbState, *key + 8, buf, sizeof(buf));
               fprintf(stderr, "utf8: '%s'\n", buf);
       }
}

void
WlKeyboardKey(void *data, YMB struct wl_keyboard *wl_keyboard, YMB uint32_t serial, 
		YMB uint32_t time, uint32_t key, uint32_t state)
{
       InternalState *pState = data;
       char buf[128];
       uint32_t keycode = key + 8;
       xkb_keysym_t sym = xkb_state_key_get_one_sym( pState->pXkbState, keycode);
       xkb_keysym_get_name(sym, buf, sizeof(buf));
       const char *action = state == WL_KEYBOARD_KEY_STATE_PRESSED ? "press" : "release";
       fprintf(stderr, "key %s: sym: %-12s (%d), ", action, buf, sym);
       xkb_state_key_get_utf8(pState->pXkbState, keycode, buf, sizeof(buf));
       fprintf(stderr, "utf8: '%s'\n", buf);
}

void
WlKeyboardLeave(YMB void *data, YMB struct wl_keyboard *wl_keyboard, 
		YMB uint32_t serial, YMB struct wl_surface *surface)
{
       YDEBUG("keyboard leave\n");
}

void
WlKeyboardModifiers(void *data, YMB struct wl_keyboard *wl_keyboard, YMB uint32_t serial, 
		uint32_t mods_depressed, uint32_t mods_latched, uint32_t mods_locked, uint32_t group)
{
       InternalState *pState = data;
       xkb_state_update_mask(pState->pXkbState, mods_depressed, mods_latched, mods_locked, 0, 0, group);
}

void
WlKeyboardRepeatInfo(YMB void *data, YMB struct wl_keyboard *wl_keyboard, 
		YMB int32_t rate, YMB int32_t delay)
{
       /* Left as an exercise for the reader */
}
#endif //PLATFORM_LINUX
