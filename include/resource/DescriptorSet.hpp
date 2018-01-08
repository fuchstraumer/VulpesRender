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

        /** Sets the required image info for the descriptor at the given index. Make sure this is called appropriately for each call to AddDescriptorBinding
        */
        void AddDescriptorInfo(const VkDescriptorImageInfo& info, const VkDescriptorType& type, const size_t& item_binding_idx);
        /** Functionally the same as AddDescriptorInfo with VkDescriptorImageInfo.  
        */
        void AddDescriptorInfo(const VkDescriptorBufferInfo& info, const VkDescriptorType& descr_type, const size_t& item_binding_idx);
        /** Call after all descriptor bindings and infos required have been added, and make sure you have enough space in the given pool for all of these resources. */
        void Init(const DescriptorPool* parent_pool, const DescriptorSetLayout* set_layout);
        const VkDescriptorSet& vkHandle() const noexcept;
        
    private:

        void allocate(const DescriptorPool* parent_pool, const DescriptorSetLayout* set_layout);
        void update();

        const Device* device;
        const DescriptorPool* descriptorPool;
        VkDescriptorSet descriptorSet;
        bool updated = false;
        bool allocated = false;
        std::map<size_t, VkWriteDescriptorSet> writeDescriptors;
        std::map<size_t, VkDescriptorBufferInfo> bufferInfos;
    };

}

#endif //!VULPES_VK_DESCRIPTOR_SET_H