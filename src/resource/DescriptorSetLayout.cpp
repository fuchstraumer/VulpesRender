#include "resource/DescriptorSetLayout.hpp"
#include "core/LogicalDevice.hpp"
#include <vector>
namespace vpr {

    DescriptorSetLayout::DescriptorSetLayout(const Device* _dvc) : device(_dvc), handle(VK_NULL_HANDLE) {}

    DescriptorSetLayout::~DescriptorSetLayout() {
        if (handle != VK_NULL_HANDLE) {
            vkDestroyDescriptorSetLayout(device->vkHandle(), handle, nullptr);
        }
    }

    DescriptorSetLayout::DescriptorSetLayout(DescriptorSetLayout&& other) noexcept : device(std::move(other.device)),
        handle(std::move(other.handle)), ready(std::move(other.ready)), bindings(std::move(other.bindings)) { other.handle = VK_NULL_HANDLE; }

    DescriptorSetLayout& DescriptorSetLayout::operator=(DescriptorSetLayout&& other) noexcept {
        device = std::move(other.device);
        handle = std::move(other.handle);
        ready = std::move(other.ready);
        bindings = std::move(other.bindings);
        other.handle = VK_NULL_HANDLE;
        return *this;
    }
    
    void DescriptorSetLayout::AddDescriptorBinding(const VkDescriptorType& descriptor_type, const VkShaderStageFlags& shader_stage, const uint32_t& descriptor_binding_loc) noexcept {
        
        

        bindings.emplace(descriptor_binding_loc, VkDescriptorSetLayoutBinding{
            descriptor_binding_loc,
            descriptor_type,
            1,
            VkShaderStageFlags(shader_stage),
            nullptr
        });

    }

    void DescriptorSetLayout::AddDescriptorBinding(const VkDescriptorSetLayoutBinding& binding) {
        bindings.emplace(binding.binding, binding);
    }

    const VkDescriptorSetLayout& DescriptorSetLayout::vkHandle() const noexcept {
        if(!ready) {
            create();
        }
        return handle;
    }

    void DescriptorSetLayout::create() const {
        size_t num_bindings = bindings.size();
        assert(!bindings.empty());
        std::vector<VkDescriptorSetLayoutBinding> bindings_vec;
        for(const auto& entry : bindings) {
            bindings_vec.push_back(entry.second);
        }

        VkDescriptorSetLayoutCreateInfo set_layout_create_info = vk_descriptor_set_layout_create_info_base;
        set_layout_create_info.bindingCount = static_cast<uint32_t>(num_bindings);
        set_layout_create_info.pBindings = bindings_vec.data();

        VkResult result = vkCreateDescriptorSetLayout(device->vkHandle(), &set_layout_create_info, nullptr, &handle);
        VkAssert(result);
        ready = true;
    }

}