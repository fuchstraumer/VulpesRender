#pragma once
#ifndef VPR_DESCRIPTOR_SET_LAYOUT_HPP
#define VPR_DESCRIPTOR_SET_LAYOUT_HPP
#include "vpr_stdafx.h"
#include "ForwardDecl.hpp"
#include <vulkan/vulkan.h>
#include <map>

namespace vpr {

    /**Simple wrapper around the VkDescriptorSetLayout object. No explicit construction function: it is created
     * when it is first used by calling vkHandle(). This class has been left copyable as it doesn't represent any
     * underlying resources or allocations like the other descriptor set related objects.
     * 
     * Make sure to add bindings before calling vkHandle(), however, as I believe an empty set won't break anything
     * but the validation layers should give warnings or errors regardless.
     * \ingroup Resources
     */
    class VPR_API DescriptorSetLayout {
    public:

        DescriptorSetLayout(const Device* _dvc);
        ~DescriptorSetLayout();

        /** Specifies that a descriptor of the given type be accessible from the given stage, and sets the index it will be accessed at in the shader.
        */
        void AddDescriptorBinding(const VkDescriptorType& descriptor_type, const VkShaderStageFlags& shader_stage, const uint32_t& descriptor_binding_loc) noexcept;
        
        const VkDescriptorSetLayout& vkHandle() const noexcept;
    private:
        mutable bool ready = false;
        void create() const;
        const Device* device;
        mutable VkDescriptorSetLayout handle;
        std::map<size_t, VkDescriptorSetLayoutBinding> bindings;
    };

}

#endif