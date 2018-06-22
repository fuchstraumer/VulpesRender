#include "vpr_stdafx.h"
#include "resource/DescriptorPool.hpp"
#include "core/LogicalDevice.hpp"
#include "resource/DescriptorSet.hpp"
#include <vector>
#include <array>
#include "common/vkAssert.hpp"
#include "common/CreateInfoBase.hpp"

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

    DescriptorPool::DescriptorPool(const Device * _device, const size_t & max_sets) : device(_device), maxSets(max_sets), handle(VK_NULL_HANDLE) {
        for (const auto& type : descriptor_types) {
            resourceTypes[type] = 0;
        }
    }

    DescriptorPool::~DescriptorPool() {
        if (handle != VK_NULL_HANDLE) {
            vkDestroyDescriptorPool(device->vkHandle(), handle, nullptr);
        }
    }

    DescriptorPool::DescriptorPool(DescriptorPool&& other) noexcept : device(std::move(other.device)), handle(std::move(other.handle)),
        maxSets(std::move(other.maxSets)), resourceTypes(std::move(other.resourceTypes)) { other.handle = VK_NULL_HANDLE; }

    DescriptorPool& DescriptorPool::operator=(DescriptorPool&& other) noexcept {
        device = std::move(other.device);
        handle = std::move(other.handle);
        other.handle = VK_NULL_HANDLE;
        maxSets = std::move(other.maxSets);
        resourceTypes = std::move(other.resourceTypes);
        return *this;
    }

    void DescriptorPool::AddResourceType(const VkDescriptorType & descriptor_type, const uint32_t & descriptor_count) {
        resourceTypes[descriptor_type] += descriptor_count;
    }

    void DescriptorPool::Create() {

        assert(!resourceTypes.empty());
        std::vector<VkDescriptorPoolSize> pool_sizes;
        
        for(const auto& entry : resourceTypes) {
            if (entry.second > 0) {
                pool_sizes.emplace_back(VkDescriptorPoolSize{ entry.first, static_cast<uint32_t>(entry.second) });
            }
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
