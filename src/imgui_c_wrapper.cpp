#include "imgui.h"
#include "backends/imgui_impl_vulkan.h"
#include "imgui_c_wrapper.h"

void ImGui_NewFrame()
{
    ImGui::NewFrame();
}

void ImGui_Render() 
{
    ImGui::Render();
}

void ImGui_ShowDemoWindow(int* p_open)
{
    ImGui::ShowDemoWindow((bool*)p_open);
}

void ImGui_Text(const char* text)
{
    ImGui::Text("%s", text);
}
bool CImGui_ImplVulkan_Init(void* initInfo)
{
	ImGui_ImplVulkan_InitInfo* pute = reinterpret_cast<ImGui_ImplVulkan_InitInfo*>(initInfo);
	return ImGui_ImplVulkan_Init(pute);
}
bool CImGui_ImplVulkan_CreateFontsTexture()
{
	return ImGui_ImplVulkan_CreateFontsTexture();
}
