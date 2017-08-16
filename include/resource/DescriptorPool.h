#ifndef VULPES_VK_DESCRIPTOR_POOL_H
#define VULPES_VK_DESCRIPTOR_POOL_H

#include "vpr_stdafx.h"
#include "ForwardDecl.h"

namespace vulpes {

    class DescriptorPool {
        DescriptorPool(const DescriptorPool&) = delete;
        DescriptorPool& operator=(const DescriptorPool&) = delete;
    public:

        DescriptorPool(const Device* device, const size_t& max_sets);
        ~DescriptorPool();

        // Counts up the resources in descriptor_set, updates resourceTypes appropriately.
        void AddDescriptorSet(const DescriptorSet* descriptor_set);
		void AddResourceType(const VkDescriptorType& descriptor_type, const uint32_t& descriptor_count);
        // Allocates based on added descriptor sets.
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