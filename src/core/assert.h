#ifndef ASSERTS_H
#define ASSERTS_H
#include "mydefines.h"

#ifdef DEBUG
#define KASSERTIONS_ENABLED

#ifdef KASSERTIONS_ENABLED
#if _MSC_VER
#include <intrin.h>
#define debugBreak() __debugbreak()
#else
#define debugBreak() __builtin_trap()
#endif

#include <stdint.h>

void ReportAssertionFailure(
		const char*							pExpression, 
		const char*							pMessage,
		const char*							pFile,
		int32_t line);

#define KASSERT(expr)														\
{																			\
	if (expr){																\
	}																		\
		else{																\
			ReportAssertionFailure(#expr, "", __FILE__, __LINE__);		\
			debugBreak();													\
		}																	\
}

#define KASSERT_MSG(expr, message)                                        \
    {                                                                     \
        if (expr) {                                                       \
        } else {                                                          \
            ReportAssertionFailure(#expr, message, __FILE__, __LINE__); \
            debugBreak();                                                 \
        }                                                                 \
    }

#ifdef _DEBUG
#define KASSERT_DEBUG(expr)                                          \
    {                                                                \
        if (expr) {                                                  \
        } else {                                                     \
            ReportAssertionFailure(#expr, "", __FILE__, __LINE__); \
            debugBreak();                                            \
        }                                                            \
    }
#else
#define KASSERT_DEBUG(expr)  // Does nothing at all
#endif

#else
#define KASSERT(expr)               // Does nothing at all
#define KASSERT_MSG(expr, message)  // Does nothing at all
#define KASSERT_DEBUG(expr)         // Does nothing at all

#endif
#endif //DEBUG

#endif // ASSERT_H
