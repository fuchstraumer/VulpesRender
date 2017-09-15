#pragma once
#ifndef VULPES_VK_DESCRIPTOR_SET_H
#define VULPES_VK_DESCRIPTOR_SET_H

#include "vpr_stdafx.h"
#include "ForwardDecl.hpp"

namespace vulpes {

    struct descriptorLimits {
        static constexpr size_t Samplers = 96;
        static constexpr size_t UniformBuffers = 72;
        static constexpr size_t UniformBuffersDynamic = 8;
        static constexpr size_t StorageBuffers = 24;
        static constexpr size_t StorageBuffersDynamic = 4;
        static constexpr size_t SampledImages = 24;
        static constexpr size_t InputAttachments = 4;
    };

    struct descriptorStageLimits {
        static constexpr size_t Samplers = 16;
        // Dynamic count as part of the per-stage uniform buffer limit.
        static constexpr size_t UniformBuffers = 12;
        // Storage count as part of the per-stage uniform buffer limit.
        static constexpr size_t StorageBuffers = 4;
        static constexpr size_t SampledImages = 16;
        static constexpr size_t StorageImages = 4;
        static constexpr size_t InputAttachments = 4;
        static constexpr size_t Total = 128;
    };

    class DescriptorSet {
        DescriptorSet(const DescriptorSet&) = delete;
        DescriptorSet& operator=(const DescriptorSet&) = delete;
    public:

        DescriptorSet(const Device* parent);
        ~DescriptorSet();

        void AddDescriptorBinding(const VkDescriptorType& descriptor_type, const VkShaderStageFlagBits& shader_stage, const uint32_t& descriptor_binding_loc);
        void AddDescriptorInfo(const VkDescriptorImageInfo& info, const size_t& item_binding_idx);
        void AddDescriptorInfo(const VkDescriptorBufferInfo& info, const size_t& item_binding_idx);

		void Init(const DescriptorPool* parent_pool);

        const VkDescriptorSet& vkHandle() const noexcept;
        const VkDescriptorSetLayout& vkLayout() const noexcept;

        static descriptorLimits DescriptorLimits;
        static descriptorStageLimits descriptorStageLimits;

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