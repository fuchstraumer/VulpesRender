#include "vpr_stdafx.h"
#include "resource/PipelineLayout.hpp"
#include "core/LogicalDevice.hpp"

namespace vulpes {

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

    void PipelineLayout::Create(const std::vector<VkPushConstantRange>& push_constants) {
        Create(std::vector<VkDescriptorSetLayout>(), push_constants);
    }

    void PipelineLayout::Create(const std::vector<VkDescriptorSetLayout>& set_layouts) {
        Create(set_layouts, std::vector<VkPushConstantRange>());
    }

    void PipelineLayout::Create(const std::vector<VkDescriptorSetLayout>& set_layouts, const std::vector<VkPushConstantRange>& push_constants) {

        createInfo = vk_pipeline_layout_create_info_base;

        if (!set_layouts.empty()) {
            createInfo.setLayoutCount = static_cast<uint32_t>(set_layouts.size());
            createInfo.pSetLayouts = set_layouts.data();
        }
        else {
            createInfo.setLayoutCount = 0;
            createInfo.pSetLayouts = nullptr;
        }

        if (!push_constants.empty()) {
            createInfo.pushConstantRangeCount = static_cast<uint32_t>(push_constants.size());
            createInfo.pPushConstantRanges = push_constants.data();
        }
        else {
            createInfo.pushConstantRangeCount = 0;
            createInfo.pPushConstantRanges = nullptr;
        }

        VkResult result = vkCreatePipelineLayout(device->vkHandle(), &createInfo, nullptr, &handle);
        VkAssert(result);

    }

    const VkPipelineLayout & PipelineLayout::vkHandle() const noexcept {
        return handle;
    }
}