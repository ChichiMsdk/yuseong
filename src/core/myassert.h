#define YASSERTIONS_ENABLED

#ifndef MYASSERT_H
#define MYASSERT_H
#include "mydefines.h"

#ifdef YASSERTIONS_ENABLED

#ifdef _MSC_VER
#include <intrin.h>
#define YDebugBreak() __debugbreak()
#else
#define YDebugBreak() __builtin_trap()
#endif // _MSC_VER

#include <stdint.h>

void ReportAssertionFailure(
		const char*							pExpression, 
		const char*							pMessage,
		const char*							pFile,
		int32_t								line);

#define YASSERT(expr)												\
{																	\
	if (expr){														\
	}																\
		else{														\
			ReportAssertionFailure(#expr, "", __FILE__, __LINE__);	\
			YDebugBreak();											\
		}															\
}

#define YASSERT_MSG(expr, message)										\
    {																	\
        if (expr) {														\
        } else {														\
            ReportAssertionFailure(#expr, message, __FILE__, __LINE__);	\
            YDebugBreak();												\
        }																\
    }

#if defined(_DEBUG) || defined(DEBUG)
#define YASSERT_DEBUG(expr)                                          \
    {                                                                \
        if (expr) {                                                  \
        } else {                                                     \
            ReportAssertionFailure(#expr, "", __FILE__, __LINE__); \
            YDebugBreak();                                            \
        }                                                            \
    }
#else
#define YASSERT_DEBUG(expr)  // Does nothing at all
#endif //_DEBUG

#else
/* NOTE: Does nothing at all */
#define YASSERT(expr)               
#define YASSERT_MSG(expr, message)
#define YASSERT_DEBUG(expr)

#endif //YASSERTIONS_ENABLED

#endif // MYASSERT_H
