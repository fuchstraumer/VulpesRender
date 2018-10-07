#include "vpr_stdafx.h"
#include "DescriptorSet.hpp"
#include "DescriptorPool.hpp"
#include "DescriptorSetLayout.hpp"
#include "vkAssert.hpp"
#include "CreateInfoBase.hpp"
#include <vector>
#include <map>

namespace vpr {

    struct DescriptorSetImpl {
        VkDescriptorPool pool{ VK_NULL_HANDLE };
        VkDescriptorSetLayout setLayout{ VK_NULL_HANDLE };
        bool updated{ false };
        bool allocated{ false };
        std::map<size_t, VkWriteDescriptorSet> writeDescriptors;
        std::map<size_t, VkDescriptorBufferInfo> bufferInfos;
        std::map<size_t, VkDescriptorImageInfo> imageInfos;
        std::map<size_t, VkBufferView> bufferViews;
    };

    DescriptorSet::DescriptorSet(const VkDevice& parent) : device(parent), handle(VK_NULL_HANDLE), impl(std::make_unique<DescriptorSetImpl>()) { }

    DescriptorSet::~DescriptorSet() {
        if (handle != VK_NULL_HANDLE) {
            vkFreeDescriptorSets(device, impl->pool, 1, &handle);
        }
    }

    DescriptorSet::DescriptorSet(DescriptorSet&& other) noexcept : device(std::move(other.device)), handle(std::move(other.handle)),
        impl(std::move(other.impl)) { other.handle = VK_NULL_HANDLE; }

    DescriptorSet& DescriptorSet::operator=(DescriptorSet&& other) noexcept {
        device = std::move(other.device);
        handle = std::move(other.handle);
        impl = std::move(other.impl);
        other.handle = VK_NULL_HANDLE;
        return *this;
    }

    void DescriptorSet::AddDescriptorInfo(VkDescriptorImageInfo info, const VkDescriptorType& type, const size_t& item_binding_idx) {
        impl->imageInfos.emplace(item_binding_idx, std::move(info));
        impl->writeDescriptors.emplace(item_binding_idx, VkWriteDescriptorSet{
            VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            nullptr,
            handle,
            static_cast<uint32_t>(item_binding_idx),
            0,
            1,
            type,
            &impl->imageInfos.at(item_binding_idx),
            nullptr,
            nullptr,
        });

    }

    void DescriptorSet::AddDescriptorInfo(VkDescriptorBufferInfo info, const VkDescriptorType& descr_type, const size_t& item_binding_idx) {
        impl->bufferInfos.emplace(item_binding_idx, std::move(info));
        impl->writeDescriptors.emplace(item_binding_idx, VkWriteDescriptorSet{
            VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            nullptr,
            handle,
            static_cast<uint32_t>(item_binding_idx),
            0,
            1,
            descr_type,
            nullptr,
            &impl->bufferInfos.at(item_binding_idx),
            nullptr,
        });
    }

    void DescriptorSet::AddDescriptorInfo(VkDescriptorBufferInfo info, const VkBufferView & view, const VkDescriptorType & type, const size_t & idx) {
        impl->bufferInfos.emplace(idx, std::move(info));
        impl->bufferViews.emplace(idx, view);
        impl->writeDescriptors.emplace(idx, VkWriteDescriptorSet{
            VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            nullptr,
            handle,
            static_cast<uint32_t>(idx),
            0,
            1,
            type,
            nullptr,
            &impl->bufferInfos.at(idx),
            &impl->bufferViews.at(idx)
        });
    }

    void DescriptorSet::AddSamplerBinding(const size_t & idx) {
        impl->writeDescriptors.emplace(idx, VkWriteDescriptorSet{
            VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            nullptr,
            handle, 
            static_cast<uint32_t>(idx),
            0,
            1,
            VK_DESCRIPTOR_TYPE_SAMPLER,
            nullptr,
            nullptr,
            nullptr
        });
    }

    void DescriptorSet::Init(const VkDescriptorPool& parent_pool, const VkDescriptorSetLayout& set_layout) {
        allocate(parent_pool, set_layout);
        update();
    }

    void DescriptorSet::allocate(const VkDescriptorPool& parent_pool, const VkDescriptorSetLayout& set_layout) const {
        
        impl->pool = parent_pool;
        impl->setLayout = set_layout;

        VkDescriptorSetAllocateInfo alloc_info = vk_descriptor_set_alloc_info_base;
        alloc_info.descriptorPool = impl->pool;
        alloc_info.descriptorSetCount = 1;
        alloc_info.pSetLayouts = &impl->setLayout;

        VkResult result = vkAllocateDescriptorSets(device, &alloc_info, &handle);
        VkAssert(result);
        impl->allocated = true;

    }

    void DescriptorSet::Update() const {
        update();
    }

    void DescriptorSet::update() const {

        assert(impl->pool && impl->allocated && !impl->writeDescriptors.empty());

        std::vector<VkWriteDescriptorSet> write_descriptors;

        for (const auto& entry : impl->writeDescriptors) {
            write_descriptors.emplace_back(entry.second);
            write_descriptors.back().dstSet = handle;
            if (entry.second.pBufferInfo != nullptr) {
                write_descriptors.back().pBufferInfo = entry.second.pBufferInfo;
            }
            if (entry.second.pImageInfo != nullptr) {
                write_descriptors.back().pImageInfo = entry.second.pImageInfo;
            }
            if (entry.second.pTexelBufferView != nullptr) {
                write_descriptors.back().pTexelBufferView = entry.second.pTexelBufferView;
            }
        }

        vkUpdateDescriptorSets(device, static_cast<uint32_t>(write_descriptors.size()), write_descriptors.data(), 0, nullptr);

        impl->updated = true;
        
    }

    const VkDescriptorSet & DescriptorSet::vkHandle() const noexcept {
        if (!impl->allocated) {
            allocate(impl->pool, impl->setLayout);
        }
        if (!impl->updated) {
            update();
        }
        return handle;
    }
    
    void DescriptorSet::Reset() {
        VkDescriptorPool pool = impl->pool;
        VkDescriptorSetLayout layout = impl->setLayout;
        impl.reset();
        impl = std::make_unique<DescriptorSetImpl>();
        impl->pool = pool;
        impl->setLayout = layout;
    }

}
