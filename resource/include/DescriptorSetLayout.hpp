#pragma once
#ifndef VPR_DESCRIPTOR_SET_LAYOUT_HPP
#define VPR_DESCRIPTOR_SET_LAYOUT_HPP
#include "vpr_stdafx.h"
#include <memory>

namespace vpr
{

    struct LayoutBindings;
    /**Simple wrapper around the VkDescriptorSetLayout object. No explicit construction function: it is created
     * when it is first used by calling vkHandle(). Make sure to add bindings before calling vkHandle(), however, 
     * as I believe an empty set won't break anything but the validation layers should give warnings or errors 
     * if it's not done for valid reasons.
     * 
     * Empty sets can be required if you have shaders that use descriptors at bindings 1 and 3, for example, as
     * you will still then need to pass an array of 3 set layouts to the function for binding descriptor sets.
     * \ingroup Resources
     */
    class VPR_API DescriptorSetLayout
    {
        DescriptorSetLayout(const DescriptorSetLayout&) = delete;
        DescriptorSetLayout& operator=(const DescriptorSetLayout&) = delete;
    public:

        DescriptorSetLayout(const VkDevice& _dvc, const VkDescriptorSetLayoutCreateFlags _flags = VkDescriptorSetLayoutCreateFlags(0));
        ~DescriptorSetLayout();
        DescriptorSetLayout(DescriptorSetLayout&& other) noexcept;
        DescriptorSetLayout& operator=(DescriptorSetLayout&& other) noexcept;

        /**Specifies that a descriptor of the given type be accessible from the given stage, and sets the index it will be accessed at in the shader. */
        void AddDescriptorBinding(const VkDescriptorType descriptor_type, const VkShaderStageFlags shader_stage, const uint32_t descriptor_binding_loc) noexcept;
        void AddDescriptorBinding(const VkDescriptorSetLayoutBinding& binding);
        void AddDescriptorBindings(const uint32_t num_bindings, const VkDescriptorSetLayoutBinding* bindings);

        /**Calling vkHandle() on this object will create the object if it is not already ready for use.*/
        const VkDescriptorSetLayout& vkHandle() const noexcept;

    private:
        void create() const;
        mutable bool ready{ false };
        VkDevice device{ VK_NULL_HANDLE };
        mutable VkDescriptorSetLayout handle{ VK_NULL_HANDLE };
        VkDescriptorSetLayoutCreateFlags creationFlags{ 0 };
        std::unique_ptr<LayoutBindings> data;
    };

}

#endif
