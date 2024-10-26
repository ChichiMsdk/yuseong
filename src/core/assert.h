#ifndef ASSERTS_H
#define ASSERTS_H
#include "mydefines.h"

#define YASSERTIONS_ENABLED

#ifdef YASSERTIONS_ENABLED
#ifdef _MSC_VER
#include <intrin.h>
#define YDebugBreak() __debugbreak()
#else
#define YDebugBreak() __builtin_trap()
#endif // YASSERTIONS_ENABLED

#include <stdint.h>

void ReportAssertionFailure(
		const char*							pExpression, 
		const char*							pMessage,
		const char*							pFile,
		int32_t								line);

#define KASSERT(expr)														\
{																			\
	if (expr){																\
	}																		\
		else{																\
			ReportAssertionFailure(#expr, "", __FILE__, __LINE__);		\
			YDebugBreak();													\
		}																	\
}

#define KASSERT_MSG(expr, message)                                        \
    {                                                                     \
        if (expr) {                                                       \
        } else {                                                          \
            ReportAssertionFailure(#expr, message, __FILE__, __LINE__); \
            YDebugBreak();                                                 \
        }                                                                 \
    }

#if defined(_DEBUG) || defined(DEBUG)
#define KASSERT_DEBUG(expr)                                          \
    {                                                                \
        if (expr) {                                                  \
        } else {                                                     \
            ReportAssertionFailure(#expr, "", __FILE__, __LINE__); \
            YDebugBreak();                                            \
        }                                                            \
    }
#else
#define KASSERT_DEBUG(expr)  // Does nothing at all
#endif //_DEBUG

#else
#define KASSERT(expr)               // Does nothing at all
#define KASSERT_MSG(expr, message)  // Does nothing at all
#define KASSERT_DEBUG(expr)         // Does nothing at all

#endif //DEBUG

#endif // ASSERT_H
