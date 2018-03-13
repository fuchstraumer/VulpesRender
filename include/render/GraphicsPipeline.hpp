#pragma once
#ifndef VULPES_VK_GRAPHICS_PIPELINE_H
#define VULPES_VK_GRAPHICS_PIPELINE_H

#include "vpr_stdafx.h"
#include "ForwardDecl.hpp"

namespace vpr {

    /** The rendering group is used for objects that are directly related to how things appear on screen, including those classes
    *   that are minimum requirements for making things display like Framebuffer, Swapchain, Renderpass, and DepthStencil. Other classes
    *   exist to make using advanced features like MSAA (Multisampling) easier by handling setup, or by pre-defining advanced renderpass
    *   setups as OffscreenFramebuffers does.
    *   \defgroup Rendering
    */

    /** This struct is used to define most of the pipeline state for a Vulkan vkGraphicsPipeline object. All members have default
    *   values that are reasonable chosen with an eye towards stability, but make sure to update the following:
    *   - DynamicStateInfo
    *   - VertexInfo
    *   - MultisampleInfo (whether or not MSAA is on, sample count)
    *   And be aware of the default values and useful values for:
    *   - AssemblyInfo: update if not using triangle lists
    *   - RasterizationInfo: Set culling mode, winding direction, and polygon rendering mode.
    *   \ingroup Rendering
    */
    struct VPR_API GraphicsPipelineInfo {
        VkPipelineVertexInputStateCreateInfo VertexInfo = vk_pipeline_vertex_input_state_create_info_base;
        VkPipelineInputAssemblyStateCreateInfo AssemblyInfo = vk_pipeline_input_assembly_create_info_base;
        VkPipelineTessellationStateCreateInfo TesselationInfo = vk_pipeline_tesselation_state_create_info_base;
        VkPipelineViewportStateCreateInfo ViewportInfo = vk_pipeline_viewport_create_info_base;
        VkPipelineRasterizationStateCreateInfo RasterizationInfo = vk_pipeline_rasterization_create_info_base;
        VkPipelineMultisampleStateCreateInfo MultisampleInfo = vk_pipeline_multisample_create_info_base;
        VkPipelineDepthStencilStateCreateInfo DepthStencilInfo = vk_pipeline_depth_stencil_create_info_base;
        VkPipelineColorBlendStateCreateInfo ColorBlendInfo = vk_pipeline_color_blend_create_info_base;
        VkPipelineDynamicStateCreateInfo DynamicStateInfo = vk_pipeline_dynamic_state_create_info_base;

        GraphicsPipelineInfo() {}

        /** This method returns a VkGraphicsPipelineCreateInfo struct with its internal state object pointers set
        *   to point to the members of this class. This is useful for short-cutting having to set them all yourself,
        *   but be aware of object lifetime and make sure the class instance being pointed to exists when using the
        *   returned VkGraphicsPipelineCreateInfo struct! Also, note that not all fields are filled: you MUST fill 
        *   the following fields yourself:
        *   - renderPass
        *   - subpass
        *   - layout
        *   - stageCount
        *   - pStages
        */
        VkGraphicsPipelineCreateInfo GetPipelineCreateInfo() const;
    };

    /** The GraphicsPipeline object is an RAII wrapper around a vkGraphicsPipeline object, handling construction and destruction
    *   alone. The GraphicsPipelineInfo structure is, truthfully, much more important and much complex than this class is.
    *
    *   \ingroup Rendering
    */
    class VPR_API GraphicsPipeline {
        GraphicsPipeline(const GraphicsPipeline&) = delete;
        GraphicsPipeline& operator=(const GraphicsPipeline&) = delete;
    public:

        GraphicsPipeline(const Device* parent, VkGraphicsPipelineCreateInfo create_info, VkPipeline handle);
        GraphicsPipeline(const Device* parent);
        ~GraphicsPipeline();
        
        GraphicsPipeline(GraphicsPipeline&& other) noexcept;
        GraphicsPipeline& operator=(GraphicsPipeline&& other) noexcept;
        
        void Init(VkGraphicsPipelineCreateInfo& create_info, const VkPipelineCache& cache = VK_NULL_HANDLE);
        void Destroy();
        const VkPipeline& vkHandle() const noexcept;

    private:

        const VkAllocationCallbacks* allocators = nullptr;
        const Device* parent;
        VkPipeline handle;
        VkGraphicsPipelineCreateInfo createInfo;

    };

    void CreateMultiple(const Device* dvc, const VkGraphicsPipelineCreateInfo* infos, const size_t  num_infos, VkPipelineCache cache, GraphicsPipeline** dest_pipelines_array);

}
#endif // !VULPES_VK_GRAPHICS_PIPELINE_H
