#pragma once
#ifndef VULPES_VK_GRAPHICS_PIPELINE_H
#define VULPES_VK_GRAPHICS_PIPELINE_H

#include "vpr_stdafx.h"
#include "../ForwardDecl.hpp"

namespace vulpes {

	struct GraphicsPipelineInfo {
		VkPipelineInputAssemblyStateCreateInfo AssemblyInfo = vk_pipeline_input_assembly_create_info_base;
		VkPipelineRasterizationStateCreateInfo RasterizationInfo = vk_pipeline_rasterization_create_info_base;
		VkPipelineColorBlendStateCreateInfo ColorBlendInfo = vk_pipeline_color_blend_create_info_base;
		VkPipelineDepthStencilStateCreateInfo DepthStencilInfo = vk_pipeline_depth_stencil_create_info_base;
		VkPipelineViewportStateCreateInfo ViewportInfo = vk_pipeline_viewport_create_info_base;
		VkPipelineMultisampleStateCreateInfo MultisampleInfo = vk_pipeline_multisample_create_info_base;
		VkPipelineDynamicStateCreateInfo DynamicStateInfo = vk_pipeline_dynamic_state_create_info_base;
		VkPipelineVertexInputStateCreateInfo VertexInfo = vk_pipeline_vertex_input_state_create_info_base;
		GraphicsPipelineInfo() {}

		VkGraphicsPipelineCreateInfo GetPipelineCreateInfo() const;
	};


	class GraphicsPipeline {
		GraphicsPipeline(const GraphicsPipeline&) = delete;
		GraphicsPipeline& operator=(const GraphicsPipeline&) = delete;
	public:

		GraphicsPipeline(const Device* parent);
		~GraphicsPipeline();

		void Init(VkGraphicsPipelineCreateInfo& create_info, const VkPipelineCache& cache = VK_NULL_HANDLE);

		const VkPipeline& vkHandle() const noexcept;

	private:

		const VkAllocationCallbacks* allocators = nullptr;
		const Device* parent;
		VkPipeline handle;
		VkGraphicsPipelineCreateInfo createInfo;

	};

}
#endif // !VULPES_VK_GRAPHICS_PIPELINE_H
