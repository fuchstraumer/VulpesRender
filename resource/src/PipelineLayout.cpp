#include "vpr_stdafx.h"
#include "PipelineLayout.hpp"
#include "vkAssert.hpp"
#include "CreateInfoBase.hpp"

namespace vpr {

    PipelineLayout::PipelineLayout(const VkDevice& _device) : device(_device), handle(VK_NULL_HANDLE) { }

    PipelineLayout::~PipelineLayout() {
        Destroy();
    }

    PipelineLayout::PipelineLayout(PipelineLayout && other) noexcept : device(other.device), handle(std::move(other.handle)) {
        other.handle = VK_NULL_HANDLE;
    }

    PipelineLayout & PipelineLayout::operator=(PipelineLayout && other) noexcept {
        device = other.device;
        handle = std::move(other.handle);
        other.handle = VK_NULL_HANDLE;
        return *this;
    }

    void PipelineLayout::Destroy() {
        if (handle != VK_NULL_HANDLE) {
            vkDestroyPipelineLayout(device, handle, nullptr);
            handle = VK_NULL_HANDLE;
        }
    }

    void PipelineLayout::Create(const size_t num_push_constants, const VkPushConstantRange * push_constants) {
        Create(num_push_constants, push_constants, 0, nullptr);
    }

    void PipelineLayout::Create(const size_t num_layouts, const VkDescriptorSetLayout * set_layouts) {
        Create(0, nullptr, num_layouts, set_layouts);
    }

    void PipelineLayout::Create(const size_t num_ranges, const VkPushConstantRange* ranges, const size_t num_layouts, const VkDescriptorSetLayout* set_layouts) {
        VkPipelineLayoutCreateInfo createInfo = vk_pipeline_layout_create_info_base;
        createInfo = vk_pipeline_layout_create_info_base;
        createInfo.setLayoutCount = static_cast<uint32_t>(num_layouts);
        createInfo.pSetLayouts = set_layouts;
        createInfo.pushConstantRangeCount = static_cast<uint32_t>(num_ranges);
        createInfo.pPushConstantRanges = ranges;

        VkResult result = vkCreatePipelineLayout(device, &createInfo, nullptr, &handle);
        VkAssert(result);

    }

    const VkPipelineLayout & PipelineLayout::vkHandle() const noexcept {
        return handle;
    }

}
