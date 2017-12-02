#include "vpr_stdafx.h"
#include "resource/DescriptorPool.hpp"
#include "core/LogicalDevice.hpp"
#include "resource/DescriptorSet.hpp"

namespace vpr {

    constexpr static std::array<VkDescriptorType, 11> descriptor_types{
        VK_DESCRIPTOR_TYPE_SAMPLER,
        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
        VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
        VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER,
        VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER,
        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
        VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC,
        VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT
    };

    DescriptorPool::DescriptorPool(const Device * _device, const size_t & max_sets) : device(_device), maxSets(max_sets) {
        for (const auto& type : descriptor_types) {
            resourceTypes[type] = 0;
        }
    }

    DescriptorPool::~DescriptorPool() {
        vkDestroyDescriptorPool(device->vkHandle(), handle, nullptr);
    }

    void DescriptorPool::AddDescriptorSet(const DescriptorSet* descriptor_set) {
        auto descriptor_bindings = descriptor_set->GetBindings();
        for(const auto& entry : descriptor_bindings) {
            resourceTypes[entry.second.descriptorType] += entry.second.descriptorCount;
        }
    }

    void DescriptorPool::AddResourceType(const VkDescriptorType & descriptor_type, const uint32_t & descriptor_count) {
        resourceTypes[descriptor_type] += descriptor_count;
    }

    void DescriptorPool::Create() {

        assert(!resourceTypes.empty());
        std::vector<VkDescriptorPoolSize> pool_sizes;
        
        for(const auto& entry : resourceTypes) {
            pool_sizes.push_back(VkDescriptorPoolSize{ entry.first, static_cast<uint32_t>(entry.second) });
        }

        VkDescriptorPoolCreateInfo pool_create_info = vk_descriptor_pool_create_info_base;
        pool_create_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
        pool_create_info.maxSets = static_cast<uint32_t>(maxSets);
        pool_create_info.poolSizeCount = static_cast<uint32_t>(pool_sizes.size());
        pool_create_info.pPoolSizes = pool_sizes.data();

        VkResult result = vkCreateDescriptorPool(device->vkHandle(), &pool_create_info, nullptr, &handle);
        VkAssert(result);

    }

    const VkDescriptorPool& DescriptorPool::vkHandle() const noexcept {
        return handle;
    }

}