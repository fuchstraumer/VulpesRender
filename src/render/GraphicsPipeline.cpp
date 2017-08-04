#include "vpr_stdafx.h"
#include "render/GraphicsPipeline.h"
#include "core/LogicalDevice.h"

namespace vulpes {

	VkGraphicsPipelineCreateInfo GraphicsPipelineInfo::GetPipelineCreateInfo() const {
			
			VkGraphicsPipelineCreateInfo create_info = vk_graphics_pipeline_create_info_base;
            create_info.pVertexInputState = &VertexInfo;
            create_info.pInputAssemblyState = &AssemblyInfo;
            create_info.pTessellationState = nullptr;
            create_info.pViewportState = &ViewportInfo;
            create_info.pRasterizationState = &RasterizationInfo;
            create_info.pMultisampleState = &MultisampleInfo;
            create_info.pDepthStencilState = &DepthStencilInfo;
            create_info.pColorBlendState = &ColorBlendInfo;
            create_info.pDynamicState = &DynamicStateInfo;
			return create_info;
			
	}

	GraphicsPipeline::GraphicsPipeline(const Device * _parent) : parent(_parent), createInfo(vk_graphics_pipeline_create_info_base) {}

	GraphicsPipeline::~GraphicsPipeline(){
		vkDestroyPipeline(parent->vkHandle(), handle, allocators);
	}

	void GraphicsPipeline::Init(VkGraphicsPipelineCreateInfo & create_info, const VkPipelineCache& cache){
		VkResult result = vkCreateGraphicsPipelines(parent->vkHandle(), cache, 1, &create_info, nullptr, &handle);
		VkAssert(result);
		createInfo = std::move(create_info);
	}

	const VkPipeline & GraphicsPipeline::vkHandle() const noexcept {
		return handle;
	}

}
