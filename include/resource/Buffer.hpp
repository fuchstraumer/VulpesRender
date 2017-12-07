#pragma once
#ifndef VULPES_VK_BUFFER_H
#define VULPES_VK_BUFFER_H
#include "vpr_stdafx.h"
#include "ForwardDecl.hpp"
#include "resource/Allocator.hpp"

namespace vpr {

    /** The resources group/module encompasses objects that especially require Vulkan/GPU resource allocation, lifetime management, and objects
    *   that most benefit from C++ abstraction to remove boilerplate code. It also relates most to the allocator module: this was separated, however,
    *   as it mostly enables much of these classes to be abstract and is complex enough to warrant its own grouping.
    *   \defgroup Resources
    */

    /** RAII wrapper around a VkBuffer object. Can be used for anything from VBOs+EBOs to compute storage buffers to uniform buffers. No initialization
    *   or resource creation is done until the CreateBuffer method is called: at this point, the size of the object must be submitted as this size is fixed
    *   after creating the VkBuffer object.
    *   \ingroup Resources
    */
    class Buffer {
        Buffer(const Buffer&) = delete;
        Buffer& operator=(const Buffer&) = delete;
    public:

        Buffer(const Device* parent);
        ~Buffer();

        Buffer(Buffer&& other) noexcept;
        Buffer& operator=(Buffer&& other) noexcept;

        /** Creates underlying handles, binds a memory region from the allocator system, and prepares everything for use. Does not load data, 
        *    however
        *   \param usage_flags: The intended use of this buffer. If memory_flags is VK_MEMORY_PROPERTY_DEVICE_LOCAL, be sure to | your usage flag 
        *                       with VK_BUFFER_USAGE_TRANFER_SRC_BIT/VK_BUFFER_USAGE_TRANFER_DST_BIT based on which way you'll be passing resources
        *   \param memory_flags: for rendering resources larger than ~512 bytes, use VK_MEMORY_PROPERTY_DEVICE_LOCAL. Otherwise, consider using 
        *                       VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |'d with VK_MEMORY_PROPERTY_HOST_COHERENT bit, especially for things that are frequently
        *                       updated like UBOs.
        */
        void CreateBuffer(const VkBufferUsageFlags& usage_flags, const VkMemoryPropertyFlags& memory_flags, const VkDeviceSize& size);

        void Destroy();

        /** Maps this object, copies data to this object, then ensures to unmap this object.
        *   \param data: whatever you intend to copy. Some objects might have to be const_cast'ed
        *   \param size: size of data being copied, which can be much smaller than the total size
        *   \param offset: destination offset, but can be set to 0 if copying to the front of the buffer or if the copy size is equivalent to the buffer's total size.
        */
        void CopyToMapped(void* data, const VkDeviceSize& size, const VkDeviceSize& offset);
        void CopyTo(void * data, const VkCommandBuffer & transfer_cmd, const VkDeviceSize& copy_size, const VkDeviceSize& copy_offset);
        /** Copies data using a single command buffer submission form the given pool to the given queue. Attempt to use a transfer-specialized queue when available.
        *   \deprecated Use the other CopyTo method that accepts a command buffer instead of this one: using a single command buffer submission like this method does
        *               is wasteful, and is not as safe as the other method: if that method is used with a TransferPool, the transfer is guarded with a VkFence (and this method is not). Additionally, the TransferPool class is thread-safe and protects access to its queue and command pool.
        */
        void CopyTo(void* data, CommandPool* cmd_pool, const VkQueue & transfer_queue, const VkDeviceSize& size, const VkDeviceSize& offset);

        /** Updates the buffer, as it says on the tin. An important note, however, is that this can NOT be called in an active renderpass (part of the Vulkan spec, unfortunately).
        */
        void Update(VkCommandBuffer& cmd, const VkDeviceSize& data_sz, const VkDeviceSize& offset, const void* data);

        void Map(const VkDeviceSize& offset = 0);
        void Unmap();

        const VkBuffer& vkHandle() const noexcept;
        VkBuffer& vkHandle() noexcept;

        const VkDescriptorBufferInfo& GetDescriptor() const noexcept;

        VkDeviceSize Size() const noexcept;

        // Due to alignment requirements, the size reported by Size() might not reflect the size of the data uploaded.
        // Use this when we are checking against data uploaded (for changes)
        VkDeviceSize InitDataSize() const noexcept;

        void* MappedMemory = nullptr;

        /** Creates a staging buffer of given size, using the given (hopefully not used) VkBuffer handle and updating the members of the given Allocation struct.
        *   Adds both of those objects to a static pool, so that they can be cleaned up together independent of individual object scopes and lifetimes.
        */
        static void CreateStagingBuffer(const Device* dvc, const VkDeviceSize& size, VkBuffer& dest, Allocation& dest_memory_range);
        /** Destroys any existing buffers in the static pool, and frees any existing allocations in the very same. This can't throw, and can be safely called
        *   after each frame (so long as you are done with transfers, and make sure they complete before ending a frame via the BaseScene endFrame() method)
        */
        static void DestroyStagingResources(const Device* device);

    protected:

        static std::vector<std::pair<VkBuffer, Allocation>> stagingBuffers;

        void createStagingBuffer(const VkDeviceSize& size, const VkDeviceSize& offset, VkBuffer& staging_buffer, Allocation& dest_memory_range);

        const Device* parent;
        const VkAllocationCallbacks* allocators = nullptr;
        VkBuffer handle;
        VkBufferCreateInfo createInfo;
        Allocation memoryAllocation;
        VkDeviceSize size;
        VkDeviceSize dataSize;

        void setDescriptorInfo() const noexcept;
        mutable bool descriptorInfoSet = false;
        mutable VkDescriptorBufferInfo descriptor;

    };
    
}

#endif // !VULPES_VK_BUFFER_H
