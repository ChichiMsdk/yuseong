#ifndef RENDERERIMPL_H
#define RENDERERIMPL_H

#include "renderer/renderer.h"

#include "core/logger.h"
#include "core/ymemory.h"
#include "core/yerror.h"

#include "renderer/vulkan/yvulkan.h"
#include "renderer/vulkan/vulkan_draw.h"
#include "renderer/directx/11/directx11.h"
#include "renderer/metal/metal.h"

typedef b8 GlResult;
typedef b8 D11Result;

/****************************************************************************/
/******************************* Vulkan *************************************/
/****************************************************************************/
YND YuResult vkDraw(
		YMB OsState*							pOsState,
		void*									pCtx);

YND YuResult vkResize(
		YMB OsState*							pOsState,
		YMB uint32_t							width,
		YMB uint32_t							height);

YND YuResult vkErrorToYuseong(
		VkResult								result);

/****************************************************************************/
/******************************* DirectX ************************************/
/****************************************************************************/
YND YuResult D11Draw(
		OsState*							pOsState,
		void*								pCtx);

YND YuResult D11Resize(
		YMB OsState*						pOsState,
		YMB uint32_t						width,
		YMB uint32_t						height);

YND YuResult D11ErrorToYuseong(
		int32_t								result);

/****************************************************************************/
/******************************* OpenGL *************************************/
/****************************************************************************/
YND YuResult glDraw(
		OsState*							pOsState,
		void*								pCtx);

YND YuResult glResize(
		YMB OsState*						pOsState,
		YMB uint32_t						width,
		YMB uint32_t						height);

YND YuResult glErrorToYuseong(
		GlResult							result);

#endif //RENDERERIMPL_H
