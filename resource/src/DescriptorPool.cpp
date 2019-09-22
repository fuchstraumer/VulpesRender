#include "vpr_stdafx.h"
#include "DescriptorPool.hpp"
#include "vkAssert.hpp"
#include "CreateInfoBase.hpp"
#include <vector>
#include <array>
#include <map>

namespace vpr
{

    struct ResourceTypeMap
    {
        std::map<VkDescriptorType, size_t> Resources{};
    };

    constexpr static std::array<VkDescriptorType, 11> descriptor_types
    {
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

    DescriptorPool::DescriptorPool(const VkDevice& _device, const size_t max_sets) : device(_device), maxSets(max_sets), handle(VK_NULL_HANDLE), typeMap(std::make_unique<ResourceTypeMap>())
    {
        for (const auto& type : descriptor_types)
        {
            typeMap->Resources[type] = 0;
        }
    }

    DescriptorPool::~DescriptorPool()
    {
        if (handle != VK_NULL_HANDLE)
        {
            vkDestroyDescriptorPool(device, handle, nullptr);
        }
    }

    DescriptorPool::DescriptorPool(DescriptorPool&& other) noexcept : device(std::move(other.device)), handle(std::move(other.handle)),
        maxSets(std::move(other.maxSets)), typeMap(std::move(other.typeMap))
    { 
        other.handle = VK_NULL_HANDLE;
    }

    DescriptorPool& DescriptorPool::operator=(DescriptorPool&& other) noexcept
    {
        device = std::move(other.device);
        handle = std::move(other.handle);
        other.handle = VK_NULL_HANDLE;
        maxSets = std::move(other.maxSets);
        typeMap = std::move(other.typeMap);
        return *this;
    }

    void DescriptorPool::AddResourceType(const VkDescriptorType& descriptor_type, const uint32_t descriptor_count)
    {
        typeMap->Resources[descriptor_type] += descriptor_count;
    }

    void DescriptorPool::Create()
    {

        assert(!typeMap->Resources.empty());
        std::vector<VkDescriptorPoolSize> pool_sizes;
        
        for(const auto& entry : typeMap->Resources)
        {
            if (entry.second > 0)
            {
                pool_sizes.emplace_back(VkDescriptorPoolSize{ entry.first, static_cast<uint32_t>(entry.second) });
            }
        }

        VkDescriptorPoolCreateInfo pool_create_info = vk_descriptor_pool_create_info_base;
        pool_create_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
        pool_create_info.maxSets = static_cast<uint32_t>(maxSets);
        pool_create_info.poolSizeCount = static_cast<uint32_t>(pool_sizes.size());
        pool_create_info.pPoolSizes = pool_sizes.data();

        VkResult result = vkCreateDescriptorPool(device, &pool_create_info, nullptr, &handle);
        VkAssert(result);

    }

    const VkDescriptorPool& DescriptorPool::vkHandle() const noexcept
    {
        return handle;
    }

}
