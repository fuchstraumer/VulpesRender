#include "vpr_stdafx.h"
#include "resource/DescriptorPool.h"
#include "core/LogicalDevice.h"
#include "resource/DescriptorSet.h"

namespace vulpes {

	DescriptorPool::DescriptorPool(const Device * _device, const size_t & max_sets) : device(_device), maxSets(max_sets) {}

	DescriptorPool::~DescriptorPool() {
		vkDestroyDescriptorPool(device->vkHandle(), handle, nullptr);
	}

	void DescriptorPool::AddDescriptorSet(const DescriptorSet* descriptor_set) {
        auto descriptor_bindings = descriptor_set->GetBindings();
        for(const auto& entry : descriptor_bindings) {
            resourceTypes[entry.second.descriptorType] += entry.second.descriptorCount;
        }
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