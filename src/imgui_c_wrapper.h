#ifndef IMGUI_C_WRAPPER_H
#define IMGUI_C_WRAPPER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

void ImGui_NewFrame();
void ImGui_Render();
void ImGui_ShowDemoWindow(int* p_open);
void ImGui_Text(const char* text);
bool CImGui_ImplVulkan_Init(void* initInfo);
bool CImGui_ImplVulkan_CreateFontsTexture();

#ifdef __cplusplus
}
#endif
#endif
