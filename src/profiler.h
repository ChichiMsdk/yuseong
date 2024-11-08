#ifndef PROFILER_H
#define PROFILER_H

#ifdef TRACY_ENABLE
#	include "TracyC.h"
#else
#	define TracyCFrameMark
#	define TracyCZoneN(a, b, c)
#	define TracyCZoneEnd(a)
#endif

#endif // PROFILER_H
