#pragma once
#ifndef VULPES_VK_DESCRIPTOR_POOL_H
#define VULPES_VK_DESCRIPTOR_POOL_H
#include "vpr_stdafx.h"
#include <memory>

namespace vpr
{

    struct ResourceTypeMap;

    /** RAII wrapper around a VkDescriptorPool intended to facilitate sharing of descriptor pools between disparate objects,
    *   thus increasing resource sharing and avoiding allocating single-use descriptor pools (as this is an expensive operation).
    *   \ingroup Resources
    */
    class VPR_API DescriptorPool
    {
        DescriptorPool(const DescriptorPool&) = delete;
        DescriptorPool& operator=(const DescriptorPool&) = delete;
    public:

        DescriptorPool(const VkDevice& _device, const size_t max_sets, const VkDescriptorPoolCreateFlags create_flags);
        ~DescriptorPool();

        DescriptorPool(DescriptorPool&& other) noexcept;
        DescriptorPool& operator=(DescriptorPool&& other) noexcept;

        /**Effectively requests that this pool adds the given quantity of the given descriptor types to its eventual allocation call. If you try to use more of a given descriptor type than has been allocated, bad things will almost certainly occur.*/
        void AddResourceType(const VkDescriptorType& descriptor_type, const uint32_t descriptor_count);
        /** Make sure to call this AFTER setting how many descriptors you require, otherwise nothing will really be created and attempting to use any descriptor sets with this pool will fail.*/
        void Create();

        const VkDescriptorPool& vkHandle() const noexcept;

    private:

        VkDevice device{ VK_NULL_HANDLE };
        size_t maxSets{ std::numeric_limits<size_t>::max() };
        VkDescriptorPoolCreateFlags createFlags{ 0u };
        VkDescriptorPool handle{ VK_NULL_HANDLE };
        std::unique_ptr<ResourceTypeMap> typeMap;
    };

}

#endif //!VULPES_VK_DESCRIPTOR_POOL_H
