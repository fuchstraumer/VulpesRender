#pragma once
#ifndef VULPES_VK_ALLOCATOR_H
#define VULPES_VK_ALLOCATOR_H
#include "vpr_stdafx.h"
#include "ForwardDecl.hpp"
#include "AllocCommon.hpp"
#include "AllocationCollection.hpp"
#include <unordered_set>

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

        Allocator(const Device* parent_dvc, bool dedicated_alloc_enabled);
        ~Allocator();

        void Recreate();

        VkDeviceSize GetPreferredBlockSize(const uint32_t& memory_type_idx) const noexcept;
        VkDeviceSize GetBufferImageGranularity() const noexcept;

        uint32_t GetMemoryHeapCount() const noexcept;
        uint32_t GetMemoryTypeCount() const noexcept;

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

        // Won't throw: but can return invalid indices. Make sure to handle this.
        uint32_t findMemoryTypeIdx(const VkMemoryRequirements& mem_reqs, const AllocationRequirements& details) const noexcept;

        // These allocation methods return VkResult's so that we can try different parameters (based partially on return code) in main allocation method.
        VkResult allocateMemoryType(const VkMemoryRequirements& memory_reqs, const AllocationRequirements& alloc_details, const uint32_t& memory_type_idx, const SuballocationType& type, Allocation& dest_allocation);
        VkResult allocatePrivateMemory(const VkDeviceSize& size, const uint32_t& memory_type_idx, Allocation& dest_allocation);

        // called from "FreeMemory" if memory to free isn't found in the allocation vectors for any of our active memory types.
        bool freePrivateMemory(const Allocation* memory_to_free);

        std::vector<std::unique_ptr<AllocationCollection>> allocations;
        std::unordered_set<std::unique_ptr<Allocation>> privateAllocations;
        std::vector<bool> emptyAllocations;
        /**Guards the private allocations set, since it's a different object entirely than the main one.
        */
        std::mutex privateMutex;
        const Device* parent;

        VkPhysicalDeviceProperties deviceProperties;
        VkPhysicalDeviceMemoryProperties deviceMemoryProperties;

        VkDeviceSize preferredLargeHeapBlockSize;
        VkDeviceSize preferredSmallHeapBlockSize;
        const VkAllocationCallbacks* pAllocationCallbacks = nullptr;
        bool usingAllocationKHR;

        /*
            Used GPU Open allocator impl. hints and this:
            http://asawicki.info/articles/VK_KHR_dedicated_allocation.php5
            blogpost to implement support for this extension.
        */
        void fetchAllocFunctionPointersKHR();
        
        PFN_vkGetBufferMemoryRequirements2KHR pVkGetBufferMemoryRequirements2KHR;
        PFN_vkGetImageMemoryRequirements2KHR pVkGetImageMemoryRequirements2KHR;
        PFN_vkGetImageSparseMemoryRequirements2KHR pVkGetImageSparseMemoryRequirements2KHR;
    };

    

}

#endif // !VULPES_VK_ALLOCATOR_H
