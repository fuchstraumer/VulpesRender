#pragma once
#ifndef VULPES_VK_DESCRIPTOR_SET_H
#define VULPES_VK_DESCRIPTOR_SET_H
#include "vpr_stdafx.h"
#include "ForwardDecl.hpp"

namespace vpr {

    /** RAII wrapper around a descriptor set, simplifying adding individual descriptor bindings for whatever stage they're required at.
    *   \ingroup Resources
    */
    class DescriptorSet {
        DescriptorSet(const DescriptorSet&) = delete;
        DescriptorSet& operator=(const DescriptorSet&) = delete;
    public:

        DescriptorSet(const Device* parent);
        ~DescriptorSet();

        /** Specifies that a descriptor of the given type be accessible from the given stage, and sets the index it will be accessed at in the shader.
        */
        void AddDescriptorBinding(const VkDescriptorType& descriptor_type, const VkShaderStageFlagBits& shader_stage, const uint32_t& descriptor_binding_loc);
        /** Sets the required image info for the descriptor at the given index. Make sure this is called appropriately for each call to AddDescriptorBinding
        */
        void AddDescriptorInfo(const VkDescriptorImageInfo& info, const size_t& item_binding_idx);
        /** Functionally the same as AddDescriptorInfo with VkDescriptorImageInfo.  
        */
        void AddDescriptorInfo(const VkDescriptorBufferInfo& info, const size_t& item_binding_idx);

        /** Call after all descriptor bindings and infos required have been added, and make sure you have enough space in the given pool for all of these resources. */
        void Init(const DescriptorPool* parent_pool);

        const VkDescriptorSet& vkHandle() const noexcept;
        /** Returns the VkDescriptorSetLayout object attached to this class, which is required when creating a VkPipelineLayout that will utilize this DescriptorSet */
        const VkDescriptorSetLayout& vkLayout() const noexcept;

        const std::map<size_t, VkDescriptorSetLayoutBinding>& GetBindings() const noexcept;
        
    private:

        void createLayout();
        void allocate(const DescriptorPool* parent_pool);
        void update();

        const Device* device;
        const DescriptorPool* descriptorPool;
        VkDescriptorSetLayout descriptorSetLayout;
        VkDescriptorSet descriptorSet;
        bool updated = false;
        bool allocated = false;
        std::map<size_t, VkWriteDescriptorSet> writeDescriptors;
        std::map<size_t, VkDescriptorSetLayoutBinding> bindings;
        std::map<size_t, VkDescriptorBufferInfo> bufferInfos;
    };

}

#endif //!VULPES_VK_DESCRIPTOR_SET_H