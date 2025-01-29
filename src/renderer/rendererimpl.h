#ifndef RENDERERIMPL_H
#define RENDERERIMPL_H

#include "renderer/renderer.h"

#include "core/logger.h"
#include "core/ymemory.h"
#include "core/yerror.h"

#include "renderer/vulkan/yvulkan.h"
#include "renderer/vulkan/vulkan_draw.h"
#include "renderer/metal/metal.h"

/****************************************************************************/
/******************************* Vulkan *************************************/
/****************************************************************************/
YND YuResult vkDraw(
		YMB OsState*		    pOsState,
		void*			    pCtx);

YND YuResult vkResize(
		YMB OsState*		    pOsState,
		YMB uint32_t		    width,
		YMB uint32_t		    height);

YND YuResult vkErrorToYuseong(
		VkResult		    result);
#endif
