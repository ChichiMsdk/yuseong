#ifndef MYSTD_H
#define MYSTD_H

/*
 * 199409L (C95)
 * 199901L (C99)
 * 201112L (C11)
 * 201710L (C17)
 * 202311L (C23)
 */

#define C23 202311L
#define C17 201710L 
#define C11 201112L
#define C99 199901L
#define C95 199409L

/*
 * TODO: nothrow attribute ?
 */

// WARN: <attribute.h> might not be found
// #ifdef __GNUC__
// #include <attribute.h>
// #endif // __GNUC__

#if defined(__clang__) || defined(__GNUC__)
#define YND [[nodiscard]]
#define YMB [[maybe_unused]]
#elif defined(_MSC_VER)
#define YMB 
#define YND
#endif //clang ||  gcc

#if __STDC_VERSION__ == C23
#endif // C23

#if __STDC_VERSION__ == C17
#endif // C17

#if __STDC_VERSION__ == C11
#endif // C17

#if __STDC_VERSION__ == C99
#endif // C17

#if __STDC_VERSION__ == C95
#endif // C17

#endif // MYSTD_H
