#ifndef EVENT_H
#define EVENT_H

#include "mydefines.h"
#include <stdint.h>

typedef struct event_context 
{
    // 128 bytes
    union
	{
        int64_t int64_t[2];
        uint64_t uint64_t[2];
        f64 f64[2];

        int32_t int32_t[4];
        uint32_t uint32_t[4];
        f32 f32[4];

        int16_t int16_t[8];
        uint16_t uint16_t[8];

        int8_t int8_t[16];
        uint8_t uint8_t[16];

        char c[16];
    }data;
}EventContext;

// Should return true if handled.
typedef b8 (*PFN_OnEvent)(
		uint16_t							code,
		void*								sender,
		void*								listener_inst,
		EventContext						data);

void EventInit(void);
void EventShutdown(void);

/**
 * Register to listen for when events are sent with the provided code. Events with duplicate
 * listener/callback combos will not be registered again and will cause this to return FALSE.
 * @param code The event code to listen for.
 * @param listener A pointer to a listener instance. Can be 0/NULL.
 * @param on_event The callback function pointer to be invoked when the event code is fired.
 * @returns TRUE if the event is successfully registered; otherwise false.
 */
b8 EventRegister(
		uint16_t							code,
		void*								pListener,
		PFN_OnEvent							on_event);

/**
 * Unregister from listening for when events are sent with the provided code. If no matching
 * registration is found, this function returns FALSE.
 * @param code The event code to stop listening for.
 * @param listener A pointer to a listener instance. Can be 0/NULL.
 * @param on_event The callback function pointer to be unregistered.
 * @returns TRUE if the event is successfully unregistered; otherwise false.
 */
b8 EventUnregister(
		uint16_t							code,
		void*								pListener,
		PFN_OnEvent							on_event);

/**
 * Fires an event to listeners of the given code. If an event handler returns 
 * TRUE, the event is considered handled and is not passed on to any more listeners.
 * @param code The event code to fire.
 * @param sender A pointer to the sender. Can be 0/NULL.
 * @param data The event data.
 * @returns TRUE if handled, otherwise FALSE.
 */
b8 EventFire(
		uint16_t							code,
		void*								pSender,
		EventContext						context);

// System internal event codes. Application should use codes beyond 255.
typedef enum SystemEventCode 
{
    // Shuts the application down on the next frame.
    EVENT_CODE_APPLICATION_QUIT = 0x01,

    // Keyboard key pressed.
    /* Context usage:
     * uint16_t key_code = data.data.uint16_t[0];
     */
    EVENT_CODE_KEY_PRESSED = 0x02,

    // Keyboard key released.
    /* Context usage:
     * uint16_t key_code = data.data.uint16_t[0];
     */
    EVENT_CODE_KEY_RELEASED = 0x03,

    // Mouse button pressed.
    /* Context usage:
     * uint16_t button = data.data.uint16_t[0];
     */
    EVENT_CODE_BUTTON_PRESSED = 0x04,

    // Mouse button released.
    /* Context usage:
     * uint16_t button = data.data.uint16_t[0];
     */
    EVENT_CODE_BUTTON_RELEASED = 0x05,

    // Mouse moved.
    /* Context usage:
     * uint16_t x = data.data.uint16_t[0];
     * uint16_t y = data.data.uint16_t[1];
     */
    EVENT_CODE_MOUSE_MOVED = 0x06,

    // Mouse moved.
    /* Context usage:
     * u8 z_delta = data.data.u8[0];
     */
    EVENT_CODE_MOUSE_WHEEL = 0x07,

    // Resized/resolution changed from the OS.
    /* Context usage:
     * uint16_t width = data.data.uint16_t[0];
     * uint16_t height = data.data.uint16_t[1];
     */
    EVENT_CODE_RESIZED = 0x08,

    MAX_EVENT_CODE = 0xFF
}SystemEventCode;

#endif // EVENT_H
