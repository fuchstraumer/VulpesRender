#pragma once
#ifndef VULPES_VK_DESCRIPTOR_POOL_H
#define VULPES_VK_DESCRIPTOR_POOL_H
#include "vpr_stdafx.h"
#include "ForwardDecl.hpp"
#include <map>

namespace vpr {

    /** RAII wrapper around a VkDescriptorPool intended to facilitate sharing of descriptor pools between disparate objects,
    *   thus increasing resource sharing and avoiding allocating single-use descriptor pools (as this is an expensive operation).
    *   \todo Try to set up a better request-based system for deciding how many descriptors of which types to allocate.
    *   \ingroup Resources
    */
    class VPR_API DescriptorPool {
        DescriptorPool(const DescriptorPool&) = delete;
        DescriptorPool& operator=(const DescriptorPool&) = delete;
    public:

        DescriptorPool(const Device* device, const size_t& max_sets);
        ~DescriptorPool();

        /** Counts up the resources required by the given DescriptorSet instance, and adds them to the resourceTypes container for eventual allocation. 
        *   \todo Is this required? */
        void AddDescriptorSet(const DescriptorSet* descriptor_set);
        /** Effectively requests that this pool adds the given quantity of the given descriptor types to its eventual allocation call. If you try to use more
        *    of a given descriptor type than has been allocated, bad things will almost certainly occur. */
        void AddResourceType(const VkDescriptorType& descriptor_type, const uint32_t& descriptor_count);
        /** Make sure to call this AFTER setting how many descriptors you require, otherwise nothing will really be created and attempting to use any descriptor
        *   sets with this pool will fail.  */
        void Create();

        const VkDescriptorPool& vkHandle() const noexcept;

    private:

        VkDescriptorPool handle;
        std::map<VkDescriptorType, size_t> resourceTypes;
        size_t maxSets;
        const Device* device;

    };

}

#endif //!VULPES_VK_DESCRIPTOR_POOL_H