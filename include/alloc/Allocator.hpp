#pragma once
#ifndef VULPES_VK_ALLOCATOR_H
#define VULPES_VK_ALLOCATOR_H
#include "vpr_stdafx.h"
#include "ForwardDecl.hpp"
#include <memory>

namespace vpr {

    /** The allocator module encompasses everything required for the operation/use of this project's rather
     *  complex GPU memory management subsystem. In Vulkan, allocating VkDeviceMemory objects for every single 
     *  individual resource requiring GPU memory is wasteful, and not practical: there is a limit in each Vulkan 
     *  implementation to just how many allocations can exist at one time (sometimes as low as 1024). Thus, we will
     *  instead allocate large chunks of memory when a previously unallocated memory type is requested: from there, 
     *  resources (like buffers, images, etc) will bind to subregions of this larger memory object. Binding is much
     *  faster than allocation, and it is also much quicker to de-allocate resources since this only involves registering
     *  a newly freed location in this subsystem. 
     * 
     *  The allocator object is spawned as a member of the LogicalDevice class, but it can be accessed by anyone that can access
     *  a logical device. Its relatively safe to access, and should be thread-safe. Utility methods exist to simplify the creation
     *  and allocation of VkImage and VkBuffer objects as much as possible, as well.
     *  
     *  This module also is minimally complete, at best. It is robust enough to not fail in common usage, and currently has functioned 
     *  wonderfully in most tasks thrown at it. However, the todo list details stuff that still needs to be done (e.g, further splitting pools and 
     *  creating some kind of defragmentation system).
     * 
     *  Lastly, huge credit to GPU-Open as this is primarily just a slightly more object-oriented/"Modern C++" styled implementation of their *excellent*
     *  memory allocator. This would not have been possible without their work: https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator
     * 
     *  \defgroup Allocation
     *  \todo Further dividing allocation among three size pools, and then still dividing among type in those pools
     *  \todo Measure costliness of Validate(), possibly use return codes to fix errors when possible.
     */

    struct AllocatorImpl;
    struct AllocationRequirements;

    /**
    * Suballocations bound to a single memory block can represent different objects,
    * though one will usually find that they end up grouped together.
    * \ingroup Allocation
    */
    enum class SuballocationType : uint8_t;

    /** The primary interface and class of this subsystem. This object is responsible for creating resources when requested, managing memory,
     *  checking integrity of memory, and cleaning up after itself and when deallocation has been requested.
     *  \ingroup Allocation
     */
    class VPR_API Allocator {
        Allocator(const Allocator&) = delete;
        Allocator(Allocator&&) = delete;
        Allocator& operator=(const Allocator&) = delete;
        Allocator& operator=(Allocator&&) = delete;
    public:

        enum class allocation_extensions {
            None = 0,
            DedicatedAllocations = 1
        };

        Allocator(const Device* parent_dvc, allocation_extensions dedicated_alloc_enabled);
        ~Allocator();

        void Recreate();

        const VkDevice& DeviceHandle() const noexcept;

        VkResult AllocateMemory(const VkMemoryRequirements& memory_reqs, const AllocationRequirements& alloc_details, const SuballocationType& suballoc_type, Allocation& dest_allocation);

        void FreeMemory(const Allocation* memory_to_free);

        // Allocates memory for an image, using given handle to get requirements. Allocation information is written to dest_memory_range, so it can then be used to bind the resources together.
        VkResult AllocateForImage(VkImage& image_handle, const AllocationRequirements& details, const SuballocationType& alloc_type, Allocation& dest_allocation);

        // Much like AllocateForImage: uses given handle to get requirements, writes details of allocation ot given range, making memory valid for binding.
        VkResult AllocateForBuffer(VkBuffer& buffer_handle, const AllocationRequirements& details, const SuballocationType& alloc_type, Allocation& dest_allocation);

        // Creates an image object using given info. When finished, given handle is a valid image object (so long as the result value is VkSuccess). Also writes details to 
        // dest_memory_range, but this method will try to bind the memory and image together too
        VkResult CreateImage(VkImage* image_handle, const VkImageCreateInfo* img_create_info, const AllocationRequirements& alloc_reqs, Allocation& dest_allocation);

        // Creates a buffer object using given info. Given handle is valid for use if method returns VK_SUCCESS, and memory will also have been bound to the object. Details of the 
        // memory used for this particular object are also written to dest_memory_range, however.
        VkResult CreateBuffer(VkBuffer* buffer_handle, const VkBufferCreateInfo* buffer_create_info, const AllocationRequirements& alloc_reqs, Allocation& dest_allocation);

        // Destroys image/buffer specified by given handle.
        void DestroyImage(const VkImage& image_handle, Allocation& allocation_to_free);
        void DestroyBuffer(const VkBuffer& buffer_handle, Allocation& allocation_to_free);

    private:
        std::unique_ptr<AllocatorImpl> impl;
    };

}

#endif // !VULPES_VK_ALLOCATOR_H
