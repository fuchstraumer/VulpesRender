#include "vpr_stdafx.h"
#include "resource/DescriptorSet.hpp"
#include "core/LogicalDevice.hpp"
#include "resource/DescriptorPool.hpp"
#include "resource/DescriptorSetLayout.hpp"
namespace vpr {

    DescriptorSet::DescriptorSet(const Device * parent) : device(parent), handle(VK_NULL_HANDLE) { }

    DescriptorSet::~DescriptorSet() {
        if (handle != VK_NULL_HANDLE) {
            vkFreeDescriptorSets(device->vkHandle(), descriptorPool->vkHandle(), 1, &handle);
        }
    }

    DescriptorSet::DescriptorSet(DescriptorSet&& other) noexcept : device(std::move(other.device)), handle(std::move(other.handle)),
        descriptorPool(std::move(other.descriptorPool)), updated(std::move(other.updated)), allocated(std::move(other.allocated)),
        writeDescriptors(std::move(other.writeDescriptors)), bufferInfos(std::move(other.bufferInfos)) { other.handle = VK_NULL_HANDLE; }

    DescriptorSet& DescriptorSet::operator=(DescriptorSet&& other) noexcept {
        device = std::move(other.device);
        descriptorPool = std::move(other.descriptorPool);
        handle = std::move(other.handle);
        other.handle = VK_NULL_HANDLE;
        updated = std::move(other.updated);
        allocated = std::move(other.allocated);
        writeDescriptors = std::move(other.writeDescriptors);
        bufferInfos = std::move(other.bufferInfos);
        return *this;
    }
    void DescriptorSet::AddDescriptorInfo(VkDescriptorImageInfo info, const VkDescriptorType& type, const size_t& item_binding_idx) {

        imageInfos.emplace(item_binding_idx, std::move(info));
        writeDescriptors.emplace(item_binding_idx, VkWriteDescriptorSet{
            VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            nullptr,
            handle,
            static_cast<uint32_t>(item_binding_idx),
            0,
            1,
            type,
            &imageInfos.at(item_binding_idx),
            nullptr,
            nullptr,
        });

    }

    void DescriptorSet::AddDescriptorInfo(VkDescriptorBufferInfo info, const VkDescriptorType& descr_type, const size_t& item_binding_idx) {
        
        bufferInfos.emplace(item_binding_idx, std::move(info));

        writeDescriptors.emplace(item_binding_idx, VkWriteDescriptorSet{
            VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            nullptr,
            handle,
            static_cast<uint32_t>(item_binding_idx),
            0,
            1,
            descr_type,
            nullptr,
            &bufferInfos.at(item_binding_idx),
            nullptr,
        });
    }

    void DescriptorSet::AddDescriptorInfo(VkDescriptorBufferInfo info, const VkBufferView & view, const VkDescriptorType & type, const size_t & idx) {

        bufferInfos.emplace(idx, std::move(info));
        bufferViews.emplace(idx, view);

        writeDescriptors.emplace(idx, VkWriteDescriptorSet{
            VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            nullptr,
            handle,
            static_cast<uint32_t>(idx),
            0,
            1,
            type,
            nullptr,
            &bufferInfos.at(idx),
            &bufferViews.at(idx)
        });
    }

    void DescriptorSet::Init(const DescriptorPool * parent_pool, const DescriptorSetLayout* set_layout) {

        allocate(parent_pool, set_layout);
        update();

    }

    void DescriptorSet::allocate(const DescriptorPool* parent_pool, const DescriptorSetLayout* set_layout) const {
        
        descriptorPool = parent_pool;
        setLayout = set_layout;

        VkDescriptorSetAllocateInfo alloc_info = vk_descriptor_set_alloc_info_base;
        alloc_info.descriptorPool = descriptorPool->vkHandle();
        alloc_info.descriptorSetCount = 1;
        alloc_info.pSetLayouts = &set_layout->vkHandle();

        VkResult result = vkAllocateDescriptorSets(device->vkHandle(), &alloc_info, &handle);
        VkAssert(result);
        allocated = true;

    }

    void DescriptorSet::Update() const {
        update();
    }

    void DescriptorSet::update() const {

        assert(descriptorPool && allocated && !writeDescriptors.empty());

        std::vector<VkWriteDescriptorSet> write_descriptors;

        for (const auto& entry : writeDescriptors) {
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

        vkUpdateDescriptorSets(device->vkHandle(), static_cast<uint32_t>(write_descriptors.size()), write_descriptors.data(), 0, nullptr);

        updated = true;
        
    }

    const VkDescriptorSet & DescriptorSet::vkHandle() const noexcept {
        if (!allocated) {
            allocate(descriptorPool, setLayout);
        }
        if (!updated) {
            update();
        }
        return handle;
    }
    
    void DescriptorSet::Reset() {
        writeDescriptors.clear();
        bufferInfos.clear();
        bufferViews.clear();
        imageInfos.clear();
        updated = false;
    }

}