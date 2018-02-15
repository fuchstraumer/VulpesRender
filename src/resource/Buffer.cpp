#include "vpr_stdafx.h"
#include "resource/Buffer.hpp"
#include "core/LogicalDevice.hpp"
#include "command/CommandPool.hpp"
#include "util/easylogging++.h"
#include "alloc/Allocator.hpp"

namespace vpr {

    std::vector<std::pair<VkBuffer, Allocation>> Buffer::stagingBuffers = std::vector<std::pair<VkBuffer, Allocation>>();
    VkBufferViewCreateInfo base_view_info{ VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO, nullptr, 0, VK_NULL_HANDLE, VK_FORMAT_UNDEFINED, 0, 0 };

    Buffer::Buffer(const Device * _parent) : parent(_parent), createInfo(vk_buffer_create_info_base), viewCreateInfo(base_view_info),
        handle(VK_NULL_HANDLE), view(VK_NULL_HANDLE) {}

    Buffer::~Buffer(){
        Destroy();
    }

    Buffer::Buffer(Buffer && other) noexcept : allocators(std::move(other.allocators)), size(std::move(other.size)), createInfo(std::move(other.createInfo)), 
        handle(std::move(other.handle)), memoryAllocation(std::move(other.memoryAllocation)), parent(std::move(other.parent)), 
        MappedMemory(std::move(other.MappedMemory)), viewCreateInfo(std::move(other.viewCreateInfo)), view(std::move(other.view)) {
        // Make sure to nullify so destructor checks safer/more likely to succeed.
        other.handle = VK_NULL_HANDLE;
        other.view = VK_NULL_HANDLE;
        other.MappedMemory = nullptr;
    }

    Buffer & Buffer::operator=(Buffer && other) noexcept {
        allocators = std::move(other.allocators);
        size = std::move(other.size);
        createInfo = std::move(other.createInfo);
        handle = std::move(other.handle);
        memoryAllocation = std::move(other.memoryAllocation);
        parent = std::move(other.parent);
        MappedMemory = std::move(other.MappedMemory);
        view = std::move(other.view);
        viewCreateInfo = std::move(other.viewCreateInfo);
        other.handle = VK_NULL_HANDLE;
        other.MappedMemory = nullptr;
        other.view = VK_NULL_HANDLE;
        return *this;
    }

    void Buffer::CreateBuffer(const VkBufferUsageFlags & usage_flags, const VkMemoryPropertyFlags & memory_flags, const VkDeviceSize & _size) {
        createInfo.usage = usage_flags;
        createInfo.size = _size;
        dataSize = _size;
        createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        AllocationRequirements reqs;
        reqs.preferredFlags = 0;
        reqs.privateMemory = false;
        reqs.requiredFlags = memory_flags;
        VkResult result = parent->vkAllocator->CreateBuffer(&handle, &createInfo, reqs, memoryAllocation);
        VkAssert(result);
        size = memoryAllocation.Size;

    }

    void Buffer::CreateBuffer(const VkBufferCreateInfo& info, const VkMemoryPropertyFlags& memory_flags) {
        createInfo = info;
        dataSize = info.size;
        AllocationRequirements reqs;
        reqs.preferredFlags = 0;
        reqs.privateMemory = false;
        reqs.requiredFlags = memory_flags;
        VkResult result = parent->vkAllocator->CreateBuffer(&handle, &createInfo, reqs, memoryAllocation);
        VkAssert(result);
        size = memoryAllocation.Size;
    }

    void Buffer::Destroy(){
        if (handle != VK_NULL_HANDLE) {
            parent->vkAllocator->DestroyBuffer(handle, memoryAllocation);
            handle = VK_NULL_HANDLE;
        }
        if (view != VK_NULL_HANDLE) {
            vkDestroyBufferView(parent->vkHandle(), view, nullptr);
        }
    }

    void Buffer::CopyToMapped(const void* data, const VkDeviceSize & copy_size, const VkDeviceSize& offset){

        Map(offset);

        if (size == 0) {
            memcpy(MappedMemory, data, static_cast<size_t>(Size()));
        }
        else {
            memcpy(MappedMemory, data, static_cast<size_t>(copy_size));
        }

        Unmap();
    }

    void Buffer::CopyTo(const void* data, const VkCommandBuffer& transfer_cmd, const VkDeviceSize& copy_size, const VkDeviceSize& copy_offset) {

        VkBuffer staging_buffer;
        Allocation staging_alloc;
        createStagingBuffer(copy_size, staging_buffer, staging_alloc);

        void* mapped;
        VkResult result = vkMapMemory(parent->vkHandle(), staging_alloc.Memory(), staging_alloc.Offset(), copy_size, 0, &mapped);
        VkAssert(result);
        memcpy(mapped, data, static_cast<size_t>(copy_size));
        vkUnmapMemory(parent->vkHandle(), staging_alloc.Memory());

        VkBufferCopy copy{};
        copy.size = copy_size;
        copy.dstOffset = copy_offset;
        vkCmdCopyBuffer(transfer_cmd, staging_buffer, handle, 1, &copy);

        auto pair = std::make_pair(std::move(staging_buffer), std::move(staging_alloc));
        stagingBuffers.push_back(std::move(pair));

    }

    void Buffer::CopyTo(const void* data, CommandPool* cmd_pool, const VkQueue & transfer_queue, const VkDeviceSize & copy_size, const VkDeviceSize & copy_offset){
        
        VkBuffer staging_buffer;
        Allocation staging_alloc;
        createStagingBuffer(copy_size, staging_buffer, staging_alloc);

        void* mapped;
        VkResult result = vkMapMemory(parent->vkHandle(), staging_alloc.Memory(), staging_alloc.Offset(), copy_size, 0, &mapped);
        VkAssert(result);
        memcpy(mapped, data, static_cast<size_t>(copy_size));
        vkUnmapMemory(parent->vkHandle(), staging_alloc.Memory());

        VkCommandBuffer copy_cmd = cmd_pool->StartSingleCmdBuffer();
            VkBufferCopy copy{};
            copy.size = copy_size;
            copy.dstOffset = copy_offset;
            vkCmdCopyBuffer(copy_cmd, staging_buffer, handle, 1, &copy);
        cmd_pool->EndSingleCmdBuffer(copy_cmd, transfer_queue);

        auto pair = std::make_pair(std::move(staging_buffer), std::move(staging_alloc));
        stagingBuffers.push_back(std::move(pair));

    }

    void Buffer::Update(const VkCommandBuffer & cmd, const VkDeviceSize & data_sz, const VkDeviceSize & offset, const void * data) {
        vkCmdUpdateBuffer(cmd, handle, memoryAllocation.Offset() + offset, data_sz, data);
    }

    VkBufferMemoryBarrier Buffer::CreateMemoryBarrier(VkAccessFlags src, VkAccessFlags dst, uint32_t src_idx, uint32_t dst_idx, VkDeviceSize sz) const {
        return VkBufferMemoryBarrier{ VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER, nullptr, src, dst, src_idx, dst_idx, 
            handle, memoryAllocation.Offset(), sz };
    }

    void Buffer::CreateView(const VkFormat format, const uint32_t range, const uint32_t offset) {
        viewCreateInfo.format = format;
        viewCreateInfo.range = range;
        viewCreateInfo.offset = offset;
        viewCreateInfo.buffer = handle;
        VkResult result = vkCreateBufferView(parent->vkHandle(), &viewCreateInfo, nullptr, &view);
        VkAssert(result);
    }

    void Buffer::Map(const VkDeviceSize& offset){
        assert(offset < memoryAllocation.Size);
        VkResult result = vkMapMemory(parent->vkHandle(), memoryAllocation.Memory(), memoryAllocation.Offset() + offset, memoryAllocation.Size, 0, &MappedMemory);
        VkAssert(result);
    }

    void Buffer::Unmap(){
        vkUnmapMemory(parent->vkHandle(), memoryAllocation.Memory());
    }

    const VkBuffer & Buffer::vkHandle() const noexcept{
        return handle;
    }

    VkBuffer & Buffer::vkHandle() noexcept{
        return handle;
    }

    const VkBufferView& Buffer::View() const noexcept {
        return view;
    }

    void Buffer::setDescriptorInfo() const noexcept {
        descriptor = VkDescriptorBufferInfo{ handle, 0, dataSize };
        descriptorInfoSet = true;
    }

    const VkDescriptorBufferInfo& Buffer::GetDescriptor() const noexcept{
        if (!descriptorInfoSet) {
            setDescriptorInfo();
        }
        return descriptor;
    }

    VkDeviceSize Buffer::Size() const noexcept{
        return size;
    }

    VkDeviceSize Buffer::InitDataSize() const noexcept {
        return dataSize;
    }

    void Buffer::CreateStagingBuffer(const Device * dvc, const VkDeviceSize & size, VkBuffer & dest, Allocation& dest_memory_alloc){
        
        VkBufferCreateInfo create_info = vk_buffer_create_info_base;
        create_info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        create_info.size = size;

        AllocationRequirements alloc_reqs;
        alloc_reqs.privateMemory = false;
        alloc_reqs.requiredFlags = VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;

        VkResult result = dvc->vkAllocator->CreateBuffer(&dest, &create_info, alloc_reqs, dest_memory_alloc);
        VkAssert(result);

        auto pair = std::make_pair(std::move(dest), dest_memory_alloc);
        stagingBuffers.push_back(std::move(pair));
        
        LOG(INFO) << "Created a staging buffer. Currently have " << std::to_string(stagingBuffers.size()) << " staging buffers in the pool.";
    }

    void Buffer::DestroyStagingResources(const Device* device){

        if (stagingBuffers.empty()) {
            return;
        }

        for (auto& buff : stagingBuffers) {
            device->vkAllocator->DestroyBuffer(buff.first, buff.second);
        }

        stagingBuffers.clear(); 
        stagingBuffers.shrink_to_fit();
    }

    void Buffer::createStagingBuffer(const VkDeviceSize & staging_size, VkBuffer & staging_buffer, Allocation& dest_memory_alloc){
        
        VkBufferCreateInfo create_info = vk_buffer_create_info_base;
        create_info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        create_info.size = staging_size;
        size = staging_size;

        AllocationRequirements alloc_reqs;
        alloc_reqs.privateMemory = false;
        alloc_reqs.requiredFlags = VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;

        VkResult result = parent->vkAllocator->CreateBuffer(&staging_buffer, &create_info, alloc_reqs, dest_memory_alloc);
        VkAssert(result);
        LOG_IF(stagingBuffers.size() > 100, WARNING) << "Warning! More than 100 staging buffers currently in the staging buffer pool!";

    }

    VkBufferUsageFlags Buffer::Usage() const noexcept {
        return createInfo.usage;
    }
}
