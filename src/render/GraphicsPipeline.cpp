#include "vpr_stdafx.h"
#include "render/GraphicsPipeline.hpp"
#include "core/LogicalDevice.hpp"
#include "common/vkAssert.hpp"
#include "common/CreateInfoBase.hpp"
#include <vector>

namespace vpr {

    GraphicsPipelineInfo::GraphicsPipelineInfo() : VertexInfo(vk_pipeline_vertex_input_state_create_info_base), AssemblyInfo(vk_pipeline_input_assembly_create_info_base), 
        TesselationInfo(vk_pipeline_tesselation_state_create_info_base), ViewportInfo(vk_pipeline_viewport_create_info_base), RasterizationInfo(vk_pipeline_rasterization_create_info_base),
        MultisampleInfo(vk_pipeline_multisample_create_info_base), DepthStencilInfo(vk_pipeline_depth_stencil_create_info_base), ColorBlendInfo(vk_pipeline_color_blend_create_info_base),
        DynamicStateInfo(vk_pipeline_dynamic_state_create_info_base) {}

    VkGraphicsPipelineCreateInfo GraphicsPipelineInfo::GetPipelineCreateInfo() const {
        VkGraphicsPipelineCreateInfo create_info = vk_graphics_pipeline_create_info_base;
        create_info.pVertexInputState = &VertexInfo;
        create_info.pInputAssemblyState = &AssemblyInfo;
        create_info.pTessellationState = &TesselationInfo;
        create_info.pViewportState = &ViewportInfo;
        create_info.pRasterizationState = &RasterizationInfo;
        create_info.pMultisampleState = &MultisampleInfo;
        create_info.pDepthStencilState = &DepthStencilInfo;
        create_info.pColorBlendState = &ColorBlendInfo;
        create_info.pDynamicState = &DynamicStateInfo;
        return create_info;        
    }

    GraphicsPipeline::GraphicsPipeline(const Device* _d, VkGraphicsPipelineCreateInfo info, VkPipeline _handle) : parent(_d), createInfo(std::move(info)), handle(_handle) {}

    GraphicsPipeline::GraphicsPipeline(const Device * _parent) : parent(_parent), createInfo(vk_graphics_pipeline_create_info_base), handle(VK_NULL_HANDLE) {}

    GraphicsPipeline::GraphicsPipeline(GraphicsPipeline&& other) noexcept : parent(std::move(other.parent)), createInfo(other.createInfo), handle(std::move(other.handle)) {
        other.handle = VK_NULL_HANDLE;
    }

    GraphicsPipeline& GraphicsPipeline::operator=(GraphicsPipeline&& other) noexcept {
        parent = std::move(other.parent);
        createInfo = other.createInfo;
        handle = std::move(other.handle);
        other.handle = VK_NULL_HANDLE;
        return *this;
    }

    GraphicsPipeline::~GraphicsPipeline(){
        Destroy();
    }

    void GraphicsPipeline::Init(VkGraphicsPipelineCreateInfo & create_info, const VkPipelineCache& cache){
        VkResult result = vkCreateGraphicsPipelines(parent->vkHandle(), cache, 1, &create_info, nullptr, &handle);
        VkAssert(result);
        createInfo = std::move(create_info);
    }
    
    void GraphicsPipeline::Destroy() {
        if(handle != VK_NULL_HANDLE) {
            vkDestroyPipeline(parent->vkHandle(), handle, allocators);
        }
    }

    const VkPipeline & GraphicsPipeline::vkHandle() const noexcept {
        return handle;
    }

    void GraphicsPipeline::CreateMultiple(const Device* dvc, const VkGraphicsPipelineCreateInfo* infos, const size_t num_infos, VkPipelineCache cache, GraphicsPipeline** results) {
        std::vector<VkPipeline> handles(num_infos);
        VkResult result = vkCreateGraphicsPipelines(dvc->vkHandle(), cache, static_cast<uint32_t>(num_infos), infos, nullptr, handles.data());
        VkAssert(result);
        for (size_t i = 0; i < handles.size(); ++i) {
            results[i] = new GraphicsPipeline(dvc, infos[i], handles[i]);
        }
    }

}
