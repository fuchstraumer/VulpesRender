#include "vpr_stdafx.h"
#include "render/GraphicsPipeline.h"
#include "core/LogicalDevice.h"

namespace vulpes {

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
