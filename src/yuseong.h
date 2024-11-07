#ifndef YUSEONG_H
#define YUSEONG_H

#include "renderer/renderer.h"

#include "os.h"
#include "core/event.h"
#include "core/input.h"
#include "core/ymemory.h"
#include "core/logger.h"

extern AppConfig gAppConfig;
extern OsState gOsState;

void AddEventCallbackAndInit(void);

void ArgvCheck(
		int									argc,
		char**								ppArgv,
		RendererType*						pType);

b8 _OnEvent(
		uint16_t							code,
		void*								pSender,
		void*								pListenerInst,
		EventContext						context);

b8 _OnKey(
		uint16_t							code,
		void*								pSender,
		void*								pListenerInst,
		EventContext						context);

b8 _OnResized(
		uint16_t							code,
		void*								pSender,
		void*								pListenerInst,
		EventContext						context);

#define YU_ASSERT(expr) YASSERT(expr == YU_SUCCESS)

#endif // YUSEONG_H
