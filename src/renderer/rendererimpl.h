#ifndef RENDERERIMPL_H
#define RENDERERIMPL_H

#include "renderer/renderer.h"

#include "core/logger.h"
#include "core/ymemory.h"

#include "renderer/vulkan/yvulkan.h"
#include "renderer/opengl/opengl.h"
#include "renderer/directx/11/directx11.h"
#include "renderer/metal/metal.h"

typedef b8 GlResult;
typedef b8 D11Result;

YND YuResult vkErrorToYuseong(VkResult result);
YND YuResult glErrorToYuseong(GlResult result);
YND YuResult D11ErrorToYuseong(int32_t result);

YND YuResult vkDraw(YMB OsState* pOsState);
YND YuResult glDraw(OsState* pOsState);
YND YuResult D11Draw(OsState* pOsState);

YND YuResult vkResize(YMB OsState* pOsState, YMB uint32_t width,YMB  uint32_t height);
YND YuResult glResize(YMB OsState* pOsState, YMB uint32_t width,YMB  uint32_t height);
YND YuResult D11Resize(YMB OsState* pOsState, YMB uint32_t width,YMB  uint32_t height);

#endif //RENDERERIMPL_H
