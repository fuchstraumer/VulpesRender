#include "vpr_stdafx.h"
#include "resource/PipelineLayout.hpp"
#include "core/LogicalDevice.hpp"
#include "common/vkAssert.hpp"
#include "common/CreateInfoBase.hpp"

namespace vpr {

    PipelineLayout::PipelineLayout(const Device * _device) : device(_device), handle(VK_NULL_HANDLE) { }

    PipelineLayout::~PipelineLayout() {
        Destroy();
    }

    PipelineLayout::PipelineLayout(PipelineLayout && other) noexcept : device(other.device), handle(std::move(other.handle)), createInfo(std::move(other.createInfo)) {
        other.handle = VK_NULL_HANDLE;
    }

    PipelineLayout & PipelineLayout::operator=(PipelineLayout && other) noexcept {
        device = other.device;
        handle = std::move(other.handle);
        createInfo = std::move(other.createInfo);
        other.handle = VK_NULL_HANDLE;
        return *this;
    }

    void PipelineLayout::Destroy() {
        if (handle != VK_NULL_HANDLE) {
            vkDestroyPipelineLayout(device->vkHandle(), handle, nullptr);
            handle = VK_NULL_HANDLE;
        }
    }

    void PipelineLayout::Create(const VkPushConstantRange * push_constants, const size_t num_push_constants) {
        Create(push_constants, num_push_constants, nullptr, 0);
    }

    void PipelineLayout::Create(const VkDescriptorSetLayout * set_layouts, const size_t num_layouts) {
        Create(nullptr, 0, set_layouts, num_layouts);
    }

    void PipelineLayout::Create(const VkPushConstantRange* ranges, const size_t num_ranges, const VkDescriptorSetLayout* set_layouts, const size_t num_layouts) {

        createInfo = vk_pipeline_layout_create_info_base;
        createInfo.setLayoutCount = static_cast<uint32_t>(num_layouts);
        createInfo.pSetLayouts = set_layouts;
        createInfo.pushConstantRangeCount = static_cast<uint32_t>(num_ranges);
        createInfo.pPushConstantRanges = ranges;

        VkResult result = vkCreatePipelineLayout(device->vkHandle(), &createInfo, nullptr, &handle);
        VkAssert(result);

    }

    const VkPipelineLayout & PipelineLayout::vkHandle() const noexcept {
        return handle;
    }

}
