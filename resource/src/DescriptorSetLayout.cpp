#include "DescriptorSetLayout.hpp"
#include "vkAssert.hpp"
#include "CreateInfoBase.hpp"
#include <vector>
#include <map>

namespace vpr
{

    struct LayoutBindings
    {
        std::map<size_t, VkDescriptorSetLayoutBinding> bindings;
        std::vector<VkDescriptorBindingFlagsEXT> bindingFlags;
    };

    DescriptorSetLayout::DescriptorSetLayout(const VkDevice& _dvc, const VkDescriptorSetLayoutCreateFlags _flags) : 
        device(_dvc), handle(VK_NULL_HANDLE), data(std::make_unique<LayoutBindings>()), creationFlags{ _flags } {}

    DescriptorSetLayout::~DescriptorSetLayout()
    {
        if (handle != VK_NULL_HANDLE)
        {
            vkDestroyDescriptorSetLayout(device, handle, nullptr);
        }
    }

    DescriptorSetLayout::DescriptorSetLayout(DescriptorSetLayout&& other) noexcept : device(std::move(other.device)),
        handle(std::move(other.handle)), ready(std::move(other.ready)), data(std::move(other.data))
    { 
        other.handle = VK_NULL_HANDLE;
    }

    DescriptorSetLayout& DescriptorSetLayout::operator=(DescriptorSetLayout&& other) noexcept
    {
        device = std::move(other.device);
        handle = std::move(other.handle);
        ready = std::move(other.ready);
        data = std::move(other.data);
        other.handle = VK_NULL_HANDLE;
        return *this;
    }
    
    void DescriptorSetLayout::AddDescriptorBinding(const VkDescriptorType descriptor_type, const VkShaderStageFlags shader_stage, const uint32_t descriptor_binding_loc) noexcept
    {
        data->bindings.emplace(descriptor_binding_loc,
            VkDescriptorSetLayoutBinding
            {
                descriptor_binding_loc,
                descriptor_type,
                1,
                VkShaderStageFlags(shader_stage),
                nullptr
            }
        );

    }

    void DescriptorSetLayout::AddDescriptorBinding(const VkDescriptorSetLayoutBinding& binding)
    {
        data->bindings.emplace(binding.binding, binding);
    }
    
    void DescriptorSetLayout::AddDescriptorBindings(const uint32_t num_bindings, const VkDescriptorSetLayoutBinding* bindings_ptr)
    {
        for (uint32_t i = 0; i < num_bindings; ++i)
        {
            data->bindings.emplace(bindings_ptr[i].binding, bindings_ptr[i]);
        }
    }

    void DescriptorSetLayout::SetBindingFlags(const uint32_t binding, const VkDescriptorBindingFlagsEXT flags)
    {
        if (binding >= data->bindingFlags.size())
        {
            data->bindingFlags.resize(static_cast<size_t>(binding), (VkDescriptorBindingFlagsEXT)0);
        }
        data->bindingFlags[binding] = flags;
    }

    const VkDescriptorSetLayout& DescriptorSetLayout::vkHandle() const noexcept
    {
        if(!ready)
        {
            create();
        }
        return handle;
    }

    void DescriptorSetLayout::create() const
    {
        assert(!data->bindings.empty());
        size_t num_bindings = static_cast<uint32_t>(data->bindings.size());
        std::vector<VkDescriptorSetLayoutBinding> bindings_vec;

        for(const auto& entry : data->bindings)
        {
            bindings_vec.emplace_back(entry.second);
        }

        VkDescriptorSetLayoutCreateInfo set_layout_create_info = vk_descriptor_set_layout_create_info_base;
        set_layout_create_info.bindingCount = static_cast<uint32_t>(num_bindings);
        set_layout_create_info.pBindings = bindings_vec.data();
        set_layout_create_info.flags = creationFlags;

        VkDescriptorSetLayoutBindingFlagsCreateInfoEXT flagsInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO_EXT, nullptr, 0u, nullptr };
        if (creationFlags & VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT_EXT)
        {
            flagsInfo.bindingCount = static_cast<uint32_t>(num_bindings);
            // If this is empty, we've really broken things
            assert(!data->bindingFlags.empty());
            if (data->bindingFlags.size() != num_bindings)
            {
                data->bindingFlags.resize(num_bindings, (VkDescriptorBindingFlagsEXT)0);
            }
            flagsInfo.pBindingFlags = data->bindingFlags.data();
            set_layout_create_info.pNext = &flagsInfo;
        }

        VkResult result = vkCreateDescriptorSetLayout(device, &set_layout_create_info, nullptr, &handle);
        VkAssert(result);
        ready = true;
    }

}