#ifndef YVEC4_H
#define YVEC4_H

#include <stdalign.h>
#include <mydefines.h>

#include "cglm/vec4.h"

#if defined(_MSC_VER)
#  define Y_INLINE __forceinline
#else
#  define Y_INLINE static inline __attribute((always_inline))
#endif


#if defined(_MSC_VER)
/* do not use alignment for older visual studio versions */
#  if _MSC_VER < 1913 /*  Visual Studio 2017 version 15.6  */
#    define Y_ALL_UNALIGNED
#    define Y_ALIGN(X) /* no alignment */
#  else
#    define Y_ALIGN(X) __declspec(align(X))
#  endif
#else
#  define Y_ALIGN(X) __attribute((aligned(X)))
#endif

#undef Y_ALIGN
#define Y_ALIGN(X) 

typedef Y_ALIGN(16) float y_vec4[4];

typedef struct ComputePushConstant
{
	alignas(8) float data1[4];
	alignas(8) float data2[4];
	alignas(8) float data3[4];
	alignas(8) float data4[4];

    /*
	 * vec4 data1;
	 * vec4 data2;
	 * vec4 data3;
	 * vec4 data4;
     */
} ComputePushConstant;

/*!
 * @brief Create vec4 with all members of [x] to [w]
 *
 * @param[in]  x, y, z, w    source
 * @param[out] v destination
 */
CGLM_INLINE
void
Vec4Fill(f32 x, f32 y, f32 z, f32 w, vec4 vOut)
{
	vOut[0] = x;
	vOut[1] = y;
	vOut[2] = z;
	vOut[3] = w;
}

/*!
 * @brief copy all members of [a] to [dest]
 *
 * @param[in]  v    source
 * @param[out] dest destination
 */
CGLM_INLINE
void
Vec4Copy(y_vec4 v, vec4 dest)
{
	dest[0] = v[0];
	dest[1] = v[1];
	dest[2] = v[2];
	dest[3] = v[3];
}
#endif // VEC4_H
