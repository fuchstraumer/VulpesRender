#pragma once
#ifndef VULPES_VK_PIPELINE_LAYOUT_H
#define VULPES_VK_PIPELINE_LAYOUT_H
#include "vpr_stdafx.h"

namespace vpr {

    /** PipelineLayout is an RAII wrapper around a VkPipelineLayout object that also simplifies setting what resources the pipeline layout
    *   will have bound to it. 
    *   \ingroup Resources
    */
    class VPR_API PipelineLayout {
        PipelineLayout(const PipelineLayout&) = delete;
        PipelineLayout& operator=(const PipelineLayout&) = delete;
    public:

        PipelineLayout(const VkDevice& device);
        ~PipelineLayout();
        PipelineLayout(PipelineLayout&& other) noexcept;
        PipelineLayout& operator=(PipelineLayout&& other) noexcept;

        void Destroy();

        /** Creates a pipeline layout that will only use push constants for setting data in shaders */
        void Create(const VkPushConstantRange* push_constants, const size_t num_push_constants);
        /** Creates a pipeline layout that will only use descriptors (of whatever type is in the layout) for data reads/writes in shaders */
        void Create(const VkDescriptorSetLayout* set_layouts, const size_t num_layouts);
        /** Creates a pipeline layout that will utilize both push constants and descriptors in the shaders. */
        void Create(const VkPushConstantRange* push_constants, const size_t num_push_constants,
                const VkDescriptorSetLayout* set_layouts, const size_t num_layouts);

        const VkPipelineLayout& vkHandle() const noexcept;

    private:
        VkDevice device{ VK_NULL_HANDLE };
        VkPipelineLayout handle{ VK_NULL_HANDLE };
    };

}


#endif //!VULPES_VK_PIPELINE_LAYOUT_H
