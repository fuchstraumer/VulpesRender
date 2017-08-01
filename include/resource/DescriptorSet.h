#ifndef VULPES_VK_DESCRIPTOR_SET_H
#define VULPES_VK_DESCRIPTOR_SET_H

#include "vpr_stdafx.h"
#include "ForwardDecl.h"

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

        // Descriptor type is the resource that will be bound: num_of_resource is the quantity we expect to use.
        void AddDescriptorType(const VkDescriptorType& descriptor_type, const size_t& num_of_resource);

        // shader_stage sets the stage we're accessing the descriptor in, descriptor_binding_loc is the uniform location specified in the shader for
        // this specified resource.
        void AddDescriptorBinding(const VkDescriptorType& descriptor_type, const VkShaderStageFlagBits& shader_stage, const uint32_t& descriptor_binding_loc);

        const VkDescriptorSet& vkHandle() const noexcept;
        const VkDescriptorSetLayout& vkLayout() const noexcept;

        static descriptorLimits DescriptorLimits;
        static descriptorStageLimits descriptorStageLimits;
    private:

        const Device* device;
        const DescriptorPool* descriptorPool;
        VkDescriptorSetLayout descriptorSetLayout;
        VkDescriptorSet descriptorSet;
        bool allocated = false;
        std::vector<VkWriteDescriptorSet> writeDescriptors;
        std::unordered_map<VkDescriptorType, size_t> resourceMap;
    };

}

#endif //!VULPES_VK_DESCRIPTOR_SET_H