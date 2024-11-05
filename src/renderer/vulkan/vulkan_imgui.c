#include "vulkan_imgui.h"

#include "core/logger.h"

#include <stdio.h>
#include <assert.h>
#ifdef _MSC_VER
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

typedef struct CImGui_ImplVulkan_InitInfo
{
    VkInstance                      Instance;
    VkPhysicalDevice                PhysicalDevice;
    VkDevice                        Device;
    uint32_t                        QueueFamily;
    VkQueue                         Queue;
    VkDescriptorPool                DescriptorPool;               // See requirements in note above
    VkRenderPass                    RenderPass;                   // Ignored if using dynamic rendering
    uint32_t                        MinImageCount;                // >= 2
    uint32_t                        ImageCount;                   // >= MinImageCount
    VkSampleCountFlagBits           MSAASamples;                  // 0 defaults to VK_SAMPLE_COUNT_1_BIT

    // (Optional)
    VkPipelineCache                 PipelineCache;
    uint32_t                        Subpass;

    // (Optional) Dynamic Rendering
    // Need to explicitly enable VK_KHR_dynamic_rendering extension to use this, even for Vulkan 1.3.
    bool                            UseDynamicRendering;
    VkPipelineRenderingCreateInfoKHR PipelineRenderingCreateInfo;

    // (Optional) Allocation, Debugging
    const VkAllocationCallbacks*    Allocator;
    void                            (*CheckVkResultFn)(VkResult err);
    VkDeviceSize                    MinAllocationSize;      // Minimum allocation size. Set to 1024*1024 to satisfy zealous best practices validation layer and waste a little memory.
}
CImGui_ImplVulkan_InitInfo;

VkResult
ImguiInit(VkContext *pCtx, VkDevice device)
{
    /*
	 * NOTE:
	 * 	1:	Create descriptor pool for IMGUI the size of the pool is very oversize,
	 * 		but it's copied from imgui demo itself.
	 */
	VkDescriptorPoolSize poolSizes[] = {
		{ VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
		{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
		{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
	};
	VkDescriptorPoolCreateInfo descriptorPoolCreateInfo = {
		.sType			= VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
		.flags			= VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
		.maxSets		= 1000 * COUNT_OF(poolSizes),
		.poolSizeCount	= (uint32_t)COUNT_OF(poolSizes),
		.pPoolSizes		= poolSizes,
	};
	VkDescriptorPool imguiPool;
	VK_CHECK(vkCreateDescriptorPool(device, &descriptorPoolCreateInfo, pCtx->pAllocator, &imguiPool));

    /*
	 * NOTE:
	 * 	2:	Initialize Imgui
	 */
	igCreateContext(NULL);
	ImGuiIO* pIo = igGetIO();
	/* NOTE: Enables keyboard control */
	pIo->ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	igStyleColorsDark(NULL);

	YMB struct CImGui_ImplVulkan_InitInfo initInfo = {
		.Instance												= pCtx->instance,
		.PhysicalDevice											= pCtx->device.physicalDev,
		.Device													= device,
		.Queue													= pCtx->device.graphicsQueue,
		.DescriptorPool											= imguiPool,
		.MinImageCount											= 3,
		.ImageCount												= 3,

		/* NOTE: dynamic rendering parameters for imgui to use */
		.UseDynamicRendering									= TRUE,
		.MSAASamples											= VK_SAMPLE_COUNT_1_BIT,

		.PipelineRenderingCreateInfo = {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
			.colorAttachmentCount		= 1,
			.pColorAttachmentFormats	= &pCtx->drawImage.format,
		},
	};

	return VK_SUCCESS;
}

ImDrawData*
ImguiDraw(void)
{

	//set docking
	ImGuiIO* ioptr = igGetIO();
	ioptr->ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;       // Enable Keyboard Controls
	igStyleColorsDark(NULL);

	// Upload Fonts
	// Use any command queue
	bool showDemoWindow = true;
	bool showAnotherWindow = false;
	ImVec4 clearColor;
	clearColor.x = 0.45f;
	clearColor.y = 0.55f;
	clearColor.z = 0.60f;
	clearColor.w = 1.00f;

	// start imgui frame
	igNewFrame();

	if (showDemoWindow)
		igShowDemoWindow(&showDemoWindow);

	// show a simple window that we created ourselves.
	{
		static float f = 0.0f;
		static int counter = 0;

		igBegin("Hello, world!", NULL, 0);
		igText("This is some useful text");
		igCheckbox("Demo window", &showDemoWindow);
		igCheckbox("Another window", &showAnotherWindow);

		igSliderFloat("Float", &f, 0.0f, 1.0f, "%.3f", 0);
		igColorEdit3("clear color", (float*)&clearColor, 0);

		ImVec2 buttonSize;
		buttonSize.x = 0;
		buttonSize.y = 0;
		if (igButton("Button", buttonSize))
			counter++;
		igSameLine(0.0f, -1.0f);
		igText("counter = %d", counter);

		igText("Application average %.3f ms/frame (%.1f FPS)",
				1000.0f / igGetIO()->Framerate, igGetIO()->Framerate);
		igEnd();
	}

	if (showAnotherWindow)
	{
		igBegin("imgui Another Window", &showAnotherWindow, 0);
		igText("Hello from imgui");
		ImVec2 buttonSize;
		buttonSize.x = 0; buttonSize.y = 0;
		if (igButton("Close me", buttonSize))
		{
			showAnotherWindow = false;
		}
		igEnd();
	}

	// render
	igRender();
	return igGetDrawData();
}
/*
 * // Cleanup
 * igDestroyContext(NULL);
 */
