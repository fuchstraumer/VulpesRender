#include "stdafx.h"
#include "GraphicsPipeline.h"
#include "engine/renderer/core/LogicalDevice.h"
#include "engine/renderer/render/Swapchain.h"
#include "engine/renderer/resource/ShaderModule.h"
#include "..\resource\PipelineCache.h"

namespace vulpes {
	GraphicsPipeline::GraphicsPipeline(const Device * _parent, const GraphicsPipelineInfo & _info) : parent(_parent), info(_info) {}

	GraphicsPipeline::GraphicsPipeline(const Device * _parent) : parent(_parent), createInfo(vk_graphics_pipeline_create_info_base) {}

	GraphicsPipeline::~GraphicsPipeline(){
		vkDestroyPipeline(parent->vkHandle(), handle, allocators);
	}

	void GraphicsPipeline::Init(VkGraphicsPipelineCreateInfo & create_info, const VkPipelineCache& cache){
		VkResult result = vkCreateGraphicsPipelines(parent->vkHandle(), cache, 1, &create_info, nullptr, &handle);
		VkAssert(result);
		createInfo = std::move(create_info);
	}


	void GraphicsPipeline::SetRenderpass(const VkRenderPass & renderpass){
		createInfo.renderPass = renderpass;
	}

	const VkPipeline & GraphicsPipeline::vkHandle() const noexcept {
		return handle;
	}

}
