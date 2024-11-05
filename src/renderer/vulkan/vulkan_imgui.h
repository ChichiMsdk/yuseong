#ifndef VULKAN_IMGUI_H
#define VULKAN_IMGUI_H

#include <stdbool.h>

#include "yvulkan.h"

#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#include "cimgui.h"
#include "cimgui_impl.h"
#include "imgui_c_wrapper.h"
/* #include "imgui/backends/imgui_impl_vulkan.h" */

#ifdef IMGUI_HAS_IMSTR
#define igBegin igBegin_Str
#define igSliderFloat igSliderFloat_Str
#define igCheckbox igCheckbox_Str
#define igColorEdit3 igColorEdit3_Str
#define igButton igButton_Str
#endif


VkResult ImguiInit(VkContext *pCtx, VkDevice device);

ImDrawData* ImguiDraw(void);

#endif // VULKAN_IMGUI_H

