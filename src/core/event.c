#include "darray.h"
#include "event.h"

typedef struct RegisteredEvent 
{
	void* pListener;
	PFN_OnEvent callback;
} RegisteredEvent;

typedef struct EventCodeEntry 
{
	RegisteredEvent* pEvents;
} EventCodeEntry;

#define MAX_MESSAGE_CODES 16384

typedef struct EventSystemState
{
	EventCodeEntry pRegistered[MAX_MESSAGE_CODES];
} EventSystemState;

/**
 * Event system internal state.
 */
static b8 bInitialized = FALSE;
static EventSystemState state = {0};

void
EventInit(void)
{
	bInitialized = TRUE;
}

void
EventShutdown(void)
{
    // Free the events arrays. And objects pointed to should be destroyed on their own.
    for(uint16_t i = 0; i < MAX_MESSAGE_CODES; ++i)
	{
        if(state.pRegistered[i].pEvents != 0)
		{
            darray_destroy(state.pRegistered[i].pEvents);
            state.pRegistered[i].pEvents = 0;
        }
    }
}

b8
EventRegister(uint16_t code, void* pListener, PFN_OnEvent onEvent)
{
    if(bInitialized == FALSE)
	{
        return FALSE;
    }

    if(state.pRegistered[code].pEvents == 0)
	{
        state.pRegistered[code].pEvents = darray_create(RegisteredEvent);
    }

    uint64_t registered_count = darray_length(state.pRegistered[code].pEvents);
    for(uint64_t i = 0; i < registered_count; ++i)
	{
        if(state.pRegistered[code].pEvents[i].pListener == pListener)
		{
            // TODO: warn
            return FALSE;
        }
    }
    // If at this point, no duplicate was found. Proceed with registration.
    RegisteredEvent event;
    event.pListener = pListener;
    event.callback = onEvent;
    darray_push(state.pRegistered[code].pEvents, event);

    return TRUE;
}

b8 
EventUnregister(uint16_t code, void* pListener, PFN_OnEvent onEvent)
{
    if(bInitialized == FALSE)
	{
        return FALSE;
    }

    // On nothing is registered for the code, boot out.
    if(state.pRegistered[code].pEvents == 0)
	{
        // TODO: warn
        return FALSE;
    }

    uint64_t registeredCount = darray_length(state.pRegistered[code].pEvents);
    for(uint64_t i = 0; i < registeredCount; ++i)
	{
        RegisteredEvent e = state.pRegistered[code].pEvents[i];
        if(e.pListener == pListener && e.callback == onEvent)
		{
            // Found one, remove it
            RegisteredEvent popped_event;
            darray_pop_at(state.pRegistered[code].pEvents, i, &popped_event);
            return TRUE;
        }
    }

    // Not found.
    return FALSE;
}

b8 
EventFire(uint16_t code, void* pSender, EventContext context)
{
    if(bInitialized == FALSE) 
	{
        return FALSE;
    }

    // If nothing is registered for the code, boot out.
    if(state.pRegistered[code].pEvents == 0)
	{
        return FALSE;
    }

    uint64_t registeredCount = darray_length(state.pRegistered[code].pEvents);
    for(uint64_t i = 0; i < registeredCount; ++i) 
	{
        RegisteredEvent e = state.pRegistered[code].pEvents[i];
        if(e.callback(code, pSender, e.pListener, context))
		{
            // Message has been handled, do not send to other pListeners.
            return TRUE;
        }
    }
    // Not found.
    return FALSE;
}
