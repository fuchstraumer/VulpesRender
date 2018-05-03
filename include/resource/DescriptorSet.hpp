#pragma once
#ifndef VULPES_VK_DESCRIPTOR_SET_H
#define VULPES_VK_DESCRIPTOR_SET_H
#include "vpr_stdafx.h"
#include "ForwardDecl.hpp"
#include <map>
namespace vpr {

    /** RAII wrapper around a descriptor set, simplifying adding individual descriptor bindings for whatever stage they're required at.
    *   \ingroup Resources
    */
    class VPR_API DescriptorSet {
        DescriptorSet(const DescriptorSet&) = delete;
        DescriptorSet& operator=(const DescriptorSet&) = delete;
    public:

        DescriptorSet(const Device* parent);
        ~DescriptorSet();

        DescriptorSet(DescriptorSet&& other) noexcept;
        DescriptorSet& operator=(DescriptorSet&& other) noexcept;

        /** Sets the required image info for the descriptor at the given index */
        void AddDescriptorInfo(VkDescriptorImageInfo info, const VkDescriptorType& type, const size_t& item_binding_idx);
        /** Functionally the same as AddDescriptorInfo with VkDescriptorImageInfo. */
        void AddDescriptorInfo(VkDescriptorBufferInfo info, const VkDescriptorType& descr_type, const size_t& item_binding_idx);
        /* Add info for a texel buffer. */
        void AddDescriptorInfo(VkDescriptorBufferInfo info, const VkBufferView& view, const VkDescriptorType& type, const size_t& idx);

        /**Call after all descriptor bindings and infos required have been added, 
        *  and make sure you have enough space in the given pool for all of these resources. 
        */
        void Init(const DescriptorPool* parent_pool, const DescriptorSetLayout* set_layout);
        const VkDescriptorSet& vkHandle() const noexcept;

        // Calls vkUpdateDescriptorSets, updating the bindings with potentially new handles representing different buffers
        void Update() const;

        // Clears the write descriptors and info vectors, requiring an update call after re-adding the descriptors
        void Reset();
        
    private:

        void allocate(const DescriptorPool* parent_pool, const DescriptorSetLayout* set_layout) const;
        void update() const;

        const Device* device;
        mutable const DescriptorPool* descriptorPool;
        mutable const DescriptorSetLayout* setLayout;
        mutable VkDescriptorSet handle;
        mutable bool updated = false;
        mutable bool allocated = false;
        mutable std::map<size_t, VkWriteDescriptorSet> writeDescriptors;
        mutable std::map<size_t, VkDescriptorBufferInfo> bufferInfos;
        mutable std::map<size_t, VkDescriptorImageInfo> imageInfos;
        mutable std::map<size_t, VkBufferView> bufferViews;
    };

}

#endif //!VULPES_VK_DESCRIPTOR_SET_H