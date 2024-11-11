#include "vulkan_pipeline.h"

#include "renderer/rendererimpl.h"

#include "core/filesystem.h"
#include "core/darray.h"
#include "core/darray_debug.h"
#include "core/logger.h"
#include "core/myassert.h"
#include "core/yerror.h"

#include "core/yvec4.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>

#define IO_CHECK(expr) \
	do { \
		int errcode = expr; \
		if (errcode != 0) { \
			char buffer[1024]; \
			OsStrError(buffer, 1024, errno); \
			YERROR("%s", buffer); \
			return VK_ERROR_UNKNOWN; \
		} \
	} while (0);

/*
 * TODO: Change error handling here
 * Needs to take into account fopen failures
 *
 * TODO: Keep those files loaded in memory ?
 */
YND VkResult
vkLoadShaderModule(VkContext *pCtx, const char* pFilePath, VkDevice device, VkShaderModule* pOutShaderModule)
{
	uint32_t*	pBuffer;
	FILE*		pStream;
	size_t		fileSize;
	size_t		count;
	size_t		elemCount = 1;

	IO_CHECK(OsFopen(&pStream, pFilePath, "rb"));
	IO_CHECK(fseek(pStream, 0, SEEK_END));
	fileSize = ftell(pStream);

	pBuffer = DarrayReserve(uint32_t, fileSize / sizeof(uint32_t));
	fseek(pStream, 0, SEEK_SET);

	/* WARN: What should happen when this fails ? */
	count = OsFread(pBuffer, fileSize, fileSize, elemCount, pStream);
	if (count != elemCount)
	{
		char msg[100];
		sprintf(msg, "OsFread: %zu", count);
		perror(msg);
	}
	IO_CHECK(OsFclose(pStream));

	VkShaderModuleCreateInfo shaderModuleCreateInfo = {
		.sType		= VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
		.pNext		= VK_NULL_HANDLE,
		.codeSize	= fileSize,
		.pCode		= pBuffer,
	};
	VK_CHECK(vkCreateShaderModule(device, &shaderModuleCreateInfo, pCtx->pAllocator, pOutShaderModule));

	DarrayDestroy(pBuffer);
	return VK_SUCCESS;
}

YND VkResult
vkComputePipelineInit(VkContext *pCtx, VkDevice device, const char** ppShaderPaths)
{
	pCtx->pComputeShaders = DarrayReserve(ComputeShaderFx, 3);

	/* NOTE: Init pipelineLayout */
	VkPushConstantRange pushConstantRange = {
		.offset = 0,
		.size = sizeof(ComputePushConstant),
		.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
	};
	VkPipelineLayoutCreateInfo computePipelineLayoutCreateInfo = {
		.sType					= VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		.pNext					= VK_NULL_HANDLE,
		.pSetLayouts			= &pCtx->drawImageDescriptorSetLayout,
		.setLayoutCount			= 1,
		.pPushConstantRanges	= &pushConstantRange,
		.pushConstantRangeCount	= 1,
	};
	VK_CHECK(vkCreatePipelineLayout(
				device,
				&computePipelineLayoutCreateInfo,
				pCtx->pAllocator,
				&pCtx->gradientComputePipelineLayout));

	/* NOTE: Setup PipelineInfo */
	uint32_t createInfoCount = 1;
	VkPipelineCache pipelineCache = VK_NULL_HANDLE;

	VkPipelineShaderStageCreateInfo pipelineShaderStageInfo = {
		.sType	= VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
		.pNext	= VK_NULL_HANDLE,
		.stage	= VK_SHADER_STAGE_COMPUTE_BIT,
		.pName	= "main",
	};
	VkComputePipelineCreateInfo computePipelineCreateInfo = {
		.sType	= VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
		.pNext	= VK_NULL_HANDLE,
		.layout	= pCtx->gradientComputePipelineLayout,
		.stage	= pipelineShaderStageInfo,
	};

	/* NOTE: Load and init pipeline of gradientColorDrawShader */
	VkShaderModule gradientDrawShader;
	VkShaderModule skyDrawShader;
	VkShaderModule gradientColorDrawShader;
	VK_CHECK(vkLoadShaderModule(pCtx, ppShaderPaths[0], device, &gradientColorDrawShader));
	VK_CHECK(vkLoadShaderModule(pCtx, ppShaderPaths[1], device, &gradientDrawShader));
	VK_CHECK(vkLoadShaderModule(pCtx, ppShaderPaths[2], device, &skyDrawShader));

	/* NOTE: Load and init pipeline of gradientDrawShader */
	pCtx->pComputeShaders[0].pFilePath = ppShaderPaths[0];
	pCtx->pComputeShaders[0].pipelineLayout = pCtx->gradientComputePipelineLayout;
	Vec4Fill(1.0f, 0.0f, 0.0f, 1.0f, pCtx->pComputeShaders[0].pushConstant.data1);
	Vec4Fill(0.0f, 0.0f, 1.0f, 1.0f, pCtx->pComputeShaders[0].pushConstant.data2);
	computePipelineCreateInfo.stage.module = gradientColorDrawShader;
	VK_CHECK(vkCreateComputePipelines(
				device,
				pipelineCache,
				createInfoCount,
				&computePipelineCreateInfo,
				pCtx->pAllocator,
				&pCtx->pComputeShaders[0].pipeline));

	/* NOTE: Load and init pipeline of gradientDrawShader */
	pCtx->pComputeShaders[1].pFilePath = ppShaderPaths[1];
	pCtx->pComputeShaders[1].pipelineLayout = pCtx->gradientComputePipelineLayout;
	Vec4Fill(1.0f, 0.0f, 0.0f, 1.0f, pCtx->pComputeShaders[1].pushConstant.data1);
	Vec4Fill(0.0f, 0.0f, 1.0f, 1.0f, pCtx->pComputeShaders[1].pushConstant.data2);
	computePipelineCreateInfo.stage.module = gradientDrawShader;
	VK_CHECK(vkCreateComputePipelines(
				device,
				pipelineCache,
				createInfoCount,
				&computePipelineCreateInfo,
				pCtx->pAllocator,
				&pCtx->pComputeShaders[1].pipeline));

	/* NOTE: Load and init pipeline of skyDrawShader */
	pCtx->pComputeShaders[2].pFilePath = ppShaderPaths[2];
	pCtx->pComputeShaders[2].pipelineLayout = pCtx->gradientComputePipelineLayout;
	Vec4Fill(0.1f, 0.2f, 0.4f, 0.97f, pCtx->pComputeShaders[2].pushConstant.data1);
	computePipelineCreateInfo.stage.module = skyDrawShader;
	VK_CHECK(vkCreateComputePipelines(
				device,
				pipelineCache,
				createInfoCount,
				&computePipelineCreateInfo,
				pCtx->pAllocator,
				&pCtx->pComputeShaders[2].pipeline));

	/* NOTE: Cleanup */
	vkDestroyShaderModule(device, skyDrawShader, pCtx->pAllocator);
	vkDestroyShaderModule(device, gradientColorDrawShader, pCtx->pAllocator);
	vkDestroyShaderModule(device, gradientDrawShader, pCtx->pAllocator);

	return VK_SUCCESS;
}

YND VkResult
vkPipelineInit(VkContext *pCtx, VkDevice device, const char* pFilePath)
{
	VkPushConstantRange pushConstantRange = {
		.offset = 0,
		.size = sizeof(ComputePushConstant),
		.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
	};
	VkPipelineLayoutCreateInfo computePipelineLayoutCreateInfo = {
		.sType					= VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		.pNext					= VK_NULL_HANDLE,
		.pSetLayouts			= &pCtx->drawImageDescriptorSetLayout,
		.setLayoutCount			= 1,
		.pPushConstantRanges	= &pushConstantRange,
		.pushConstantRangeCount	= 1,
	};
	VK_CHECK(vkCreatePipelineLayout(
				device,
				&computePipelineLayoutCreateInfo,
				pCtx->pAllocator,
				&pCtx->gradientComputePipelineLayout));

	VkShaderModule computeDrawShader;
	VK_CHECK(vkLoadShaderModule(pCtx, pFilePath, device, &computeDrawShader));

	VkPipelineShaderStageCreateInfo pipelineShaderStageInfo = {
		.sType	= VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
		.pNext	= VK_NULL_HANDLE,
		.stage	= VK_SHADER_STAGE_COMPUTE_BIT,
		.module	= computeDrawShader,
		.pName	= "main",
	};
	VkComputePipelineCreateInfo computePipelineCreateInfo = {
		.sType	= VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
		.pNext	= VK_NULL_HANDLE,
		.layout	= pCtx->gradientComputePipelineLayout,
		.stage	= pipelineShaderStageInfo,
	};

	uint32_t createInfoCount = 1;
	VkPipelineCache pipelineCache = VK_NULL_HANDLE;

	VK_CHECK(vkCreateComputePipelines(
				device,
				pipelineCache,
				createInfoCount,
				&computePipelineCreateInfo,
				pCtx->pAllocator,
				&pCtx->gradientComputePipeline));

	vkDestroyShaderModule(device, computeDrawShader, pCtx->pAllocator);

	return VK_SUCCESS;
}

YND VkResult
vkPipelineReset(VkContext *pCtx, VkDevice device, const char* pFilePath)
{
	VK_ASSERT(vkDeviceWaitIdle(device));
	vkDestroyPipeline(device, pCtx->gradientComputePipeline, pCtx->pAllocator);
	vkDestroyPipelineLayout(device, pCtx->gradientComputePipelineLayout, pCtx->pAllocator);

	VK_CHECK(vkPipelineInit(pCtx, device, pFilePath));
	YINFO("Pipeline was successfully reset.");
	return VK_SUCCESS;
}

inline void
vkRasterizerSetModes(
		VkPipelineRasterizationStateCreateInfo*	pRasterizer,
		VkPolygonMode							polygonMode,
		VkCullModeFlags							cullModeFlags,
		VkFrontFace								frontFace,
		f32										lineWidth)
{
	pRasterizer->sType			= VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	pRasterizer->cullMode		= cullModeFlags;
	pRasterizer->frontFace		= frontFace;
	pRasterizer->polygonMode	= polygonMode;
	pRasterizer->lineWidth		= lineWidth;
}

inline void
vkSetMultisamplingNone(VkPipelineMultisampleStateCreateInfo* pMultisamplingCreateInfo)
{
	pMultisamplingCreateInfo->sType					= VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	pMultisamplingCreateInfo->sampleShadingEnable	= VK_FALSE;
	/* NOTE: multisampling defaulted to no multisampling (1 sample per pixel) */
	pMultisamplingCreateInfo->rasterizationSamples	= VK_SAMPLE_COUNT_1_BIT;
	pMultisamplingCreateInfo->minSampleShading		= 1.0f;
	pMultisamplingCreateInfo->pSampleMask			= VK_NULL_HANDLE;
	/* NOTE: no alpha to coverage either */
	pMultisamplingCreateInfo->alphaToCoverageEnable	= VK_FALSE;
	pMultisamplingCreateInfo->alphaToOneEnable		= VK_FALSE;
}

inline void
vkDisableBlending(VkPipelineColorBlendAttachmentState* pColorBlendAttachment)
{
	/* NOTE: default write mask */
	pColorBlendAttachment->colorWriteMask	= VK_COLOR_COMPONENT_R_BIT
											  | VK_COLOR_COMPONENT_G_BIT
											  | VK_COLOR_COMPONENT_B_BIT
											  | VK_COLOR_COMPONENT_A_BIT;
	/* NOTE: No sampling */
	pColorBlendAttachment->blendEnable		= VK_FALSE;
}

inline void
vkDepthTestDisable(VkPipelineDepthStencilStateCreateInfo* pDepthStencil)
{
    pDepthStencil->sType					= VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    pDepthStencil->depthTestEnable			= VK_FALSE;
    pDepthStencil->depthWriteEnable			= VK_FALSE;
    pDepthStencil->depthCompareOp			= VK_COMPARE_OP_NEVER;
    pDepthStencil->depthBoundsTestEnable	= VK_FALSE;
    pDepthStencil->stencilTestEnable		= VK_FALSE;
    pDepthStencil->front					= (VkStencilOpState){0};
    pDepthStencil->back						= (VkStencilOpState){0};
    pDepthStencil->minDepthBounds			= 0.0f;
    pDepthStencil->maxDepthBounds			= 1.0f;
}

inline void
vkPipelineRenderingSetFormatAndDepthFormat(
		VkPipelineRenderingCreateInfo*		pRenderingCreateInfo,
		VkFormat*							pOutColorAttachmentFormat,
		VkFormat							depthAttachmentFormat,
		VkFormat							colorAttachmentformat)
{
	*pOutColorAttachmentFormat						= colorAttachmentformat;

	pRenderingCreateInfo->sType						= VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
	/* NOTE: Connect the format to the renderingCreateInfo structure */
	pRenderingCreateInfo->colorAttachmentCount		= 1;
	pRenderingCreateInfo->pColorAttachmentFormats	= pOutColorAttachmentFormat;
	pRenderingCreateInfo->depthAttachmentFormat		= depthAttachmentFormat;
}

inline void
vkInputAssemblySetTopology(VkPipelineInputAssemblyStateCreateInfo* pInputAssembly, VkPrimitiveTopology topology)
{
	pInputAssembly->sType					= VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	pInputAssembly->topology				= topology;
	/* NOTE: we are not going to use primitive restart on the entire tutorial so leave it on false */
	pInputAssembly->primitiveRestartEnable	= VK_FALSE;
}

YND VkResult
vkGraphicsPipelineCreate(
		VkDevice							device,
		GraphicsPipeline*					pPipeline,
		VkAllocationCallbacks*				pAllocator,
		VkPipeline*							pOutPipeline)
{
    /*
     * NOTE: make viewport state from our stored viewport and scissor.
     * at the moment we wont support multiple viewports or scissors
     */
	VkPipelineViewportStateCreateInfo viewportState	=	{
		.sType			=	VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
		.viewportCount	=	1,
		.scissorCount	=	1,
	};
    /*
	 * NOTE: setup dummy color blending. We arent using transparent objects yet
     * the blending is just "no blend", but we do write to the color attachment
     */
	VkPipelineColorBlendStateCreateInfo colorBlending	=	{
		.sType				=	VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
		.logicOpEnable		=	VK_FALSE,
		.logicOp			=	VK_LOGIC_OP_COPY,
		.attachmentCount	=	1,
		.pAttachments		=	&pPipeline->colorBlendAttachment,
	};

	/* NOTE: completely clear VertexInputStateCreateInfo, as we have no need for it */
	VkPipelineVertexInputStateCreateInfo vertexInputInfo	=	{
		.sType	=	VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
	};

    /*
	 * NOTE: build the actual pipeline.
     * we now use all of the info structs we have been writing into into this one
     * to create the pipeline
     */
    VkGraphicsPipelineCreateInfo pipelineInfo = {
		.sType					=	VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
		/* NOTE: connect the renderInfo to the pNext extension mechanism */
		.pNext					=	&pPipeline->renderingCreateInfo,

		.stageCount				=	(uint32_t) DarrayCapacity(pPipeline->pShaderStages),
		.pStages				=	pPipeline->pShaderStages,
		.pVertexInputState		=	&vertexInputInfo,
		.pInputAssemblyState	=	&pPipeline->inputAssembly,
		.pViewportState			=	&viewportState,
		.pRasterizationState	=	&pPipeline->rasterizer,
		.pMultisampleState		=	&pPipeline->multisampling,
		.pColorBlendState		=	&colorBlending,
		.pDepthStencilState		=	&pPipeline->depthStencil,
		.layout					=	pPipeline->pipelineLayout,
	};

	VkDynamicState state[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };

	VkPipelineDynamicStateCreateInfo dynamicInfo = { 
		.sType				=	VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
		.pDynamicStates		=	&state[0],
		.dynamicStateCount	=	COUNT_OF(state),
	};

	pipelineInfo.pDynamicState		=	&dynamicInfo;
	VkPipelineCache	pipelineCache	=	VK_NULL_HANDLE;
	uint32_t		createInfoCount	=	1;

	VK_CHECK(vkCreateGraphicsPipelines(
				device,
				pipelineCache,
				createInfoCount,
				&pipelineInfo,
				pAllocator,
				pOutPipeline));

	return VK_SUCCESS;
}

YND VkResult
vkTrianglePipelineInit(VkContext *pCtx, VkDevice device, TrianglePipeline* pTrianglePipeline)
{
	VkShaderModule	triangleFragmentShaderModule;
	VK_CHECK(vkLoadShaderModule(pCtx, pTrianglePipeline->pFragmentShaderFilePath, device, &triangleFragmentShaderModule));
	YINFO("Triangle fragment shader loaded.");

	VkShaderModule	triangleVertexShaderModule;
	VK_CHECK(vkLoadShaderModule(pCtx, pTrianglePipeline->pVertexShaderFilePath, device, &triangleVertexShaderModule));
	YINFO("Triangle vertex shader loaded.");
    /*
	 * NOTE: build the pipeline layout that controls the inputs/outputs of the shader
	 * we are not using descriptor sets or other systems yet, so no need to use anything other than empty default
     */
	VkPipelineLayoutCreateInfo pipelineLayoutInfo = {
		.sType	=	VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
	};

	VK_CHECK(vkCreatePipelineLayout(device, &pipelineLayoutInfo, pCtx->pAllocator, &pTrianglePipeline->pipelineLayout));
	pTrianglePipeline->graphicsPipeline.pipelineLayout	= pTrianglePipeline->pipelineLayout;

	uint32_t	shaderCreateInfoCount	= 2 ;
	pTrianglePipeline->graphicsPipeline.pShaderStages = DarrayReserve(VkPipelineShaderStageCreateInfo, 
			shaderCreateInfoCount);

	VkPipelineShaderStageCreateInfo triFragShaderStage = {
		.sType	= VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
		.stage	= VK_SHADER_STAGE_FRAGMENT_BIT,
		.module = triangleFragmentShaderModule,
		.pName	= "main",
	};
	DarrayPush(pTrianglePipeline->graphicsPipeline.pShaderStages, triFragShaderStage);

	VkPipelineShaderStageCreateInfo triVertexShaderStage = {
		.sType	= VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
		.stage	= VK_SHADER_STAGE_VERTEX_BIT,
		.module = triangleVertexShaderModule,
		.pName	= "main",
	};
	DarrayPush(pTrianglePipeline->graphicsPipeline.pShaderStages, triVertexShaderStage);

	vkInputAssemblySetTopology(&pTrianglePipeline->graphicsPipeline.inputAssembly, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);

	f32	lineWidth = 1.0f;
	vkRasterizerSetModes(
			&pTrianglePipeline->graphicsPipeline.rasterizer,
			VK_POLYGON_MODE_FILL,
			VK_CULL_MODE_NONE,
			VK_FRONT_FACE_CLOCKWISE,
			lineWidth);

	vkSetMultisamplingNone(&pTrianglePipeline->graphicsPipeline.multisampling);

	vkDisableBlending(&pTrianglePipeline->graphicsPipeline.colorBlendAttachment);
	vkDepthTestDisable(&pTrianglePipeline->graphicsPipeline.depthStencil);

	VkFormat	depthFormat = VK_FORMAT_UNDEFINED;
	vkPipelineRenderingSetFormatAndDepthFormat(
			&pTrianglePipeline->graphicsPipeline.renderingCreateInfo,
			&pCtx->drawImage.format,
			depthFormat,
			pCtx->drawImage.format);

	VkPipeline newPipeline;
	VK_CHECK(vkGraphicsPipelineCreate(
				device,
				&pTrianglePipeline->graphicsPipeline,
				pCtx->pAllocator,
				&newPipeline));

	pTrianglePipeline->pipeline = newPipeline;
	vkDestroyShaderModule(device, triangleFragmentShaderModule, pCtx->pAllocator);
	vkDestroyShaderModule(device, triangleVertexShaderModule, pCtx->pAllocator);
	DarrayDestroy(pTrianglePipeline->graphicsPipeline.pShaderStages);
	exit(1);

	return VK_SUCCESS;
}

void
vkGraphicsPipelineClear(GraphicsPipeline* pPipeline)
{
	pPipeline->inputAssembly		= (VkPipelineInputAssemblyStateCreateInfo) {
		.sType =	VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO
	};
	pPipeline->rasterizer			= (VkPipelineRasterizationStateCreateInfo) {
		.sType =	VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO
	};
	pPipeline->multisampling		= (VkPipelineMultisampleStateCreateInfo) {
		.sType =	VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO
	};
	pPipeline->depthStencil			= (VkPipelineDepthStencilStateCreateInfo) {
		.sType =	VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO
	};
	pPipeline->renderingCreateInfo			= (VkPipelineRenderingCreateInfo) {
		.sType =	VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO
	};
	pPipeline->colorBlendAttachment	= (VkPipelineColorBlendAttachmentState) {0};
	pPipeline->pipelineLayout		= (VkPipelineLayout) {0};
	DarrayClear(pPipeline->pShaderStages);
}
