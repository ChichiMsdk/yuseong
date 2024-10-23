#ifndef YSTRING_H
#define YSTRING_H

#ifdef PLATFORM_WINDOWS
#define PlatStrDup(str) _strdup(str);
#elif PLATFORM_LINUX
#define PlatStrDup(str) strdup(str);
#else
#define PlatStrDup(str) strdup(str);
#endif

#define StrDup(str) PlatStrDup(str)

#endif  //YSTRING_H
