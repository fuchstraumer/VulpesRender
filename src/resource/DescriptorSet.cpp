#include "vpr_stdafx.h"
#include "resource/DescriptorSet.hpp"
#include "core/LogicalDevice.hpp"
#include "resource/DescriptorPool.hpp"
#include "resource/DescriptorSetLayout.hpp"
namespace vpr {

    DescriptorSet::DescriptorSet(const Device * parent) : device(parent) { }

    DescriptorSet::~DescriptorSet() {
        vkFreeDescriptorSets(device->vkHandle(), descriptorPool->vkHandle(), 1, &descriptorSet);
        vkDestroyDescriptorSetLayout(device->vkHandle(), descriptorSetLayout, nullptr);
    }

    void DescriptorSet::AddDescriptorInfo(const VkDescriptorImageInfo& info, const size_t& item_binding_idx) {

        VkWriteDescriptorSet write_descriptor {
            VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            nullptr,
            descriptorSet,
            static_cast<uint32_t>(item_binding_idx),
            0,
            1,
            bindings.at(item_binding_idx).descriptorType,
            &info,
            nullptr,
            nullptr,
        };

        writeDescriptors.insert(std::make_pair(item_binding_idx, write_descriptor));

    }

    void DescriptorSet::AddDescriptorInfo(const VkDescriptorBufferInfo& info, const size_t& item_binding_idx) {
        
        bufferInfos.insert(std::make_pair(item_binding_idx, info));

        VkWriteDescriptorSet write_descriptor {
            VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            nullptr,
            descriptorSet,
            static_cast<uint32_t>(item_binding_idx),
            0,
            1,
            bindings.at(item_binding_idx).descriptorType,
            nullptr,
            &bufferInfos.at(item_binding_idx),
            nullptr,
        };

        writeDescriptors.insert(std::make_pair(item_binding_idx, write_descriptor));

    }

    void DescriptorSet::Init(const DescriptorPool * parent_pool, const DescriptorSetLayout* set_layout) {

        allocate(parent_pool);
        update();

    }

    void DescriptorSet::allocate(const DescriptorPool* parent_pool, const DescriptorSetLayout* set_layout) {
        
        descriptorPool = parent_pool;

        VkDescriptorSetAllocateInfo alloc_info = vk_descriptor_set_alloc_info_base;
        alloc_info.descriptorPool = descriptorPool->vkHandle();
        alloc_info.descriptorSetCount = 1;
        alloc_info.pSetLayouts = &set_layout->vkHandle();

        VkResult result = vkAllocateDescriptorSets(device->vkHandle(), &alloc_info, &descriptorSet);
        allocated = true;

    }

    void DescriptorSet::update() {

        assert(descriptorPool && allocated && !writeDescriptors.empty());

        std::vector<VkWriteDescriptorSet> write_descriptors;

        for (const auto& entry : writeDescriptors) {
            write_descriptors.push_back(entry.second);
            write_descriptors.back().dstSet = descriptorSet;
            if (entry.second.pBufferInfo != nullptr) {
                write_descriptors.back().pBufferInfo = entry.second.pBufferInfo;
            }
            if (entry.second.pImageInfo != nullptr) {
                write_descriptors.back().pImageInfo = entry.second.pImageInfo;
            }
        }

        vkUpdateDescriptorSets(device->vkHandle(), static_cast<uint32_t>(write_descriptors.size()), write_descriptors.data(), 0, nullptr);

        updated = true;
        
    }

    const VkDescriptorSet & DescriptorSet::vkHandle() const noexcept {
        return descriptorSet;
    }

    const VkDescriptorSetLayout & DescriptorSet::vkLayout() const noexcept {
        return descriptorSetLayout;
    }

     const std::map<size_t, VkDescriptorSetLayoutBinding>& DescriptorSet::GetBindings() const noexcept {
         return bindings;
     }

}