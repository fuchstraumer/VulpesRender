#include "vpr_stdafx.h"
#include "resource/Buffer.hpp"
#include "core/LogicalDevice.hpp"
#include "command/CommandPool.hpp"
#include "easylogging++.h"
#include "alloc/Allocator.hpp"
#include "alloc/AllocationRequirements.hpp"
#include "common/vkAssert.hpp"
#include "common/CreateInfoBase.hpp"
#include <vector>

namespace vpr {

    static std::vector<std::pair<VkBuffer, Allocation>> stagingBuffers = std::vector<std::pair<VkBuffer, Allocation>>();
    VkBufferViewCreateInfo base_view_info{ VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO, nullptr, 0, VK_NULL_HANDLE, VK_FORMAT_UNDEFINED, 0, 0 };

    Buffer::Buffer(const Device * _parent) : parent(_parent), createInfo(vk_buffer_create_info_base), viewCreateInfo(base_view_info),
        handle(VK_NULL_HANDLE), view(VK_NULL_HANDLE) {}

    Buffer::~Buffer(){
        Destroy();
    }

    Buffer::Buffer(Buffer && other) noexcept : allocators(std::move(other.allocators)), size(std::move(other.size)), createInfo(std::move(other.createInfo)), 
        handle(std::move(other.handle)), memoryAllocation(std::move(other.memoryAllocation)), parent(std::move(other.parent)), 
        viewCreateInfo(std::move(other.viewCreateInfo)), view(std::move(other.view)) {
        // Make sure to nullify so destructor checks safer/more likely to succeed.
        other.handle = VK_NULL_HANDLE;
        other.view = VK_NULL_HANDLE;
    }

    Buffer & Buffer::operator=(Buffer && other) noexcept {
        allocators = std::move(other.allocators);
        size = std::move(other.size);
        createInfo = std::move(other.createInfo);
        handle = std::move(other.handle);
        memoryAllocation = std::move(other.memoryAllocation);
        parent = std::move(other.parent);
        view = std::move(other.view);
        viewCreateInfo = std::move(other.viewCreateInfo);
        other.handle = VK_NULL_HANDLE;
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
        VkResult result = parent->GetAllocator()->CreateBuffer(&handle, &createInfo, reqs, memoryAllocation);
        VkAssert(result);
        size = memoryAllocation.Size;
        setMappedMemoryRange();
    }

    void Buffer::CreateBuffer(const VkBufferCreateInfo& info, const VkMemoryPropertyFlags& memory_flags) {
        createInfo = info;
        dataSize = info.size;
        AllocationRequirements reqs;
        reqs.preferredFlags = 0;
        reqs.privateMemory = false;
        reqs.requiredFlags = memory_flags;
        VkResult result = parent->GetAllocator()->CreateBuffer(&handle, &createInfo, reqs, memoryAllocation);
        VkAssert(result);
        size = memoryAllocation.Size;
        setMappedMemoryRange();
    }

    void Buffer::Destroy(){
        if (handle != VK_NULL_HANDLE) {
            parent->GetAllocator()->DestroyBuffer(handle, memoryAllocation);
            handle = VK_NULL_HANDLE;
        }
        if (view != VK_NULL_HANDLE) {
            vkDestroyBufferView(parent->vkHandle(), view, nullptr);
        }
    }

    void Buffer::CopyToMapped(const void* data, const VkDeviceSize & copy_size, const VkDeviceSize& offset){

        Map(offset);

        if (size == 0) {
            memcpy(mappedMemory, data, static_cast<size_t>(Size()));
        }
        else {
            memcpy(mappedMemory, data, static_cast<size_t>(copy_size));
        }

        Unmap();
    }

    void Buffer::CopyTo(const Buffer* buff, const VkCommandBuffer& cmd, const VkDeviceSize offset) {
        VkBufferCopy copy{};
        copy.size = buff->Size();
        copy.dstOffset = offset;
        vkCmdCopyBuffer(cmd, buff->vkHandle(), handle, 1, &copy);
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

        stagingBuffers.emplace_back(std::make_pair(std::move(staging_buffer), std::move(staging_alloc)));

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

        stagingBuffers.emplace_back(std::make_pair(std::move(staging_buffer), std::move(staging_alloc)));

    }

    void Buffer::Update(const VkCommandBuffer & cmd, const VkDeviceSize & data_sz, const VkDeviceSize & offset, const void * data) {
        vkCmdUpdateBuffer(cmd, handle, offset, data_sz, data);
    }

    void Buffer::Fill(const VkCommandBuffer& cmd, const VkDeviceSize sz, const VkDeviceSize offset, const uint32_t value) {
        #ifdef NDEBUG
        if (offset % 4 != 0) {
            LOG(ERROR) << "Supplied offset value to Buffer fill command is not a multiple of 4, and is as such not valid!";
        }

        if ((sz % 4 != 0) && (sz != Size())) {
            LOG(ERROR) << "Size of fill operation supplied to Buffer fill command is both not a multiple of 4 and wouldn't fill the whole buffer!";
        }
        #endif
        vkCmdFillBuffer(cmd, handle, offset, sz, value);
    }

    VkBufferMemoryBarrier Buffer::CreateMemoryBarrier(VkAccessFlags src, VkAccessFlags dst, uint32_t src_idx, uint32_t dst_idx, VkDeviceSize sz) const {
        
        if ((src_idx != dst_idx) && (createInfo.sharingMode == VK_SHARING_MODE_EXCLUSIVE)) {
            LOG(WARNING) << "Created a memory barrier for a buffer with different source/destination queues, but the buffer has sharing mode VK_SHARING_MODE_EXCLUSIVE!";
        }
        
        return VkBufferMemoryBarrier{ VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER, nullptr, src, dst, src_idx, dst_idx, 
            handle, memoryAllocation.Offset(), sz };
    }

    void Buffer::CreateView(const VkFormat format, const VkDeviceSize range, const VkDeviceSize offset) {
        viewCreateInfo.format = format;
        viewCreateInfo.range = range;
        viewCreateInfo.offset = offset;
        viewCreateInfo.buffer = handle;
        VkResult result = vkCreateBufferView(parent->vkHandle(), &viewCreateInfo, nullptr, &view);
        VkAssert(result);
    }

    void Buffer::Map(const VkDeviceSize& offset) {
        assert(offset < memoryAllocation.Size);
        VkResult result = vkMapMemory(parent->vkHandle(), memoryAllocation.Memory(), memoryAllocation.Offset() + offset, memoryAllocation.Size, 0, &mappedMemory);
        VkAssert(result);
    }

    void Buffer::Unmap() {
        vkUnmapMemory(parent->vkHandle(), memoryAllocation.Memory());
    }

    void Buffer::Flush() {
        VkResult result = vkFlushMappedMemoryRanges(parent->vkHandle(), 1, &mappedMemoryRange);
        VkAssert(result);
    }

    void Buffer::Invalidate() {
        VkResult result = vkInvalidateMappedMemoryRanges(parent->vkHandle(), 1, &mappedMemoryRange);
        VkAssert(result);
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

    const VkMappedMemoryRange& Buffer::MappedMemoryRange() const noexcept {
        return mappedMemoryRange;
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
    
    void Buffer::CreateStagingBuffer(const Device * dvc, const VkDeviceSize & size, VkBuffer & dest, Allocation& dest_memory_alloc){
        
        VkBufferCreateInfo create_info = vk_buffer_create_info_base;
        create_info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        create_info.size = size;

        AllocationRequirements alloc_reqs;
        alloc_reqs.privateMemory = false;
        alloc_reqs.requiredFlags = VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;

        VkResult result = dvc->GetAllocator()->CreateBuffer(&dest, &create_info, alloc_reqs, dest_memory_alloc);
        VkAssert(result);

    }

    std::unique_ptr<Buffer> Buffer::CreateStagingBuffer(const Device* dvc, void* data, const size_t data_size) {
        VkBufferCreateInfo buffer_info = vk_buffer_create_info_base;
        buffer_info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        buffer_info.size = static_cast<VkDeviceSize>(data_size);
        buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        std::unique_ptr<Buffer> result = std::make_unique<Buffer>(dvc);
        result->CreateBuffer(buffer_info, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
        result->CopyToMapped(data, static_cast<VkDeviceSize>(data_size), 0);

        return std::move(result);
    }

    void Buffer::SetMappedMemory(void * mapping_destination) {
        mappedMemory = mapping_destination;
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

        VkResult result = parent->GetAllocator()->CreateBuffer(&staging_buffer, &create_info, alloc_reqs, dest_memory_alloc);

    }

    VkBufferUsageFlags Buffer::Usage() const noexcept {
        return createInfo.usage;
    }

    void Buffer::setMappedMemoryRange() const {
        mappedMemoryRange = VkMappedMemoryRange{ VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE, nullptr, 
            memoryAllocation.Memory(), memoryAllocation.Offset(), memoryAllocation.Size };
    }
}
