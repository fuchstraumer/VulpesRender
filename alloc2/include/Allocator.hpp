#pragma once
#ifndef VPR_ALLOC_2_ALLOCATOR_HPP
#define VPR_ALLOC_2_ALLOCATOR_HPP
#include "vpr_stdafx.h"
#include "AllocationTypes.hpp"
#include "Allocation.hpp"

namespace vpr {

    /*The allocation group is designed for efficiently managing Vulkan memory allocation, handling
    * the more complicated logic of allocation (primarily by allocating large VkDeviceMemory objects, 
    * to which we bind and use small regions of). 
    *
    * With version 2.0, I've made this system handle more like Vulkan in many ways (see: opaque handles) and tried to increase
    * the thread-safety, debug functionality, utility, and general robustness of the system. It's now
    * even more ready to easily plug into larger graphics engines and systems.
    * 
    * As with previous versions, this is really just my interpretation of GPU-Open's amazing Vulkan
    * memory allocator; I won't claim it's an improvement (it probably isn't), but rewriting it is a 
    * great learning exercise in tons of ways.
    * 
    * Original repository can be found at https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator
    * \defgroup Allocation
    */

    VPR_API VkResult MakeAllocator(AllocatorCreateInfo* create_info, Allocator* alloc) noexcept;
    VPR_API void DestroyAllocator(Allocator* alloc) noexcept;

    VPR_API void CalculateStats(Allocator alloc, AllocatorMemoryStats* stats);
    VPR_API void BuildStatsString(Allocator alloc, char** stats_string, VkBool32 detailed_output);
    VPR_API void FreeStatsString(Allocator alloc, char** stats_string, VkBool32 detailed_output);

    VPR_API VkResult FindMemoryTypeIndex(Allocator alloc, uint32_t type_bits, const AllocationCreateInfo* alloc_create_info, uint32_t* result_memory_idx) noexcept;
    VPR_API VkResult FindMemoryTypeIndexForBuffer(Allocator alloc, const VkBufferCreateInfo* buffer_info, const AllocationCreateInfo* alloc_info, uint32_t* result_memory_idx) noexcept;
    VPR_API VkResult FindMemoryTypeIndexForImage(Allocator alloc, const VkImageCreateInfo* image_info, const AllocationCreateInfo* alloc_info, uint32_t* result_memory_idx) noexcept;

    /*
        \brief Creates a new #MemoryPool: potentially allocating device memory in the process
        \param alloc Allocator instance this pool will be a child of
        \param create_info Creation parameters of the pool
        \param[out] pool Handle of created pool object
    */
    VPR_API VkResult CreateMemoryPool(Allocator alloc, const MemoryPoolCreateInfo* create_info, MemoryPool* pool) noexcept;
    /*
        \brief Destroys a #MemoryPool, and frees any device memory it allocated
        \param alloc Allocator instance that the pool was a child of
        \param pool Handle of the MemoryPool to be destroyed
    */
    VPR_API void DestroyMemoryPool(Allocator alloc, MemoryPool pool) noexcept;
    /*
        \brief Calculates memory statistics for a #MemoryPool instance
        \param alloc_handle Handle to allocator instance that pool_handle was created from
        \param pool_handle Handle to MemoryPool to calculate stats for
        \param dest_stats_ptr Destination #MemoryPoolStats struct to write values into
    */
    VPR_API void CalculateMemoryPoolStats(Allocator alloc_handle, MemoryPool pool_handle, MemoryPoolStats* dest_stats_ptr) noexcept;
    /*
        \brief Marks all allocations in the given pool as lost, as if they are not used in the frame or for MemoryPoolCreateInfo::FrameInUseCount frames from now
        \param alloc_handle Allocator object handle
        \param pool_handle Pool for which we're marking everything cleared
        \param[out] lost_alloc_count_ptr Optional pointer to which we'll write (if not `nullptr`) how many allocations were just "lost"
    */
   VPR_API void MarkAllMemoryPoolAllocationsLost(Allocator alloc_handle, MemoryPool pool_handle, size_t* lost_alloc_count_ptr);

    /*
        \brief Most general-purpose allocation function.
        \param[out] dest_handle Handle to the allocated memory
        \param[out] dest_alloc_info Optional pointer to the information about the allocated memory

        #AllocationInfo can be retrieved later by calling GetAllocationInfo(), as well.
    */
    VPR_API VkResult AllocateMemory(Allocator alloc_handle, const VkMemoryRequirements* memory_reqs, const AllocationCreateInfo* alloc_info, Allocation* dest_handle, AllocationInfo* dest_alloc_info);
    /*
        \brief Allocate several allocations at once (potentially more efficient than singular). All from same memory type and pool.
        \param alloc_handle Allocator object handle
        \param memory_reqs Pointer to VkMemoryRequirements structure
        \param alloc_info AllocationCreateInfo structure to use to define properties of all the items allocated
        \param num_allocations Number of allocations being performed (and size of output arrays)
        \param[out] dest_handles_array Array of #Allocation handles that will be written to 
        \param[out] alloc_info_array Optional array of #AllocationInfo objects that can also be written to when creating the object
    */
    VPR_API VkResult AllocateMemoryPages(Allocator alloc_handle, const VkMemoryRequirements* memory_reqs, const AllocationCreateInfo* alloc_info, size_t num_allocations,
        Allocation* dest_handles_array, AllocationInfo* alloc_info_array);
    /*
        \brief Frees memory previously allocated through this interface for the given allocator and handle
        Passing `VK_NULL_HANDLE` as `allocation` is valid, and such calls will just be skipped (similar to calling `delete` on `nullptr`)
    */
    VPR_API void FreeMemory(Allocator allocator_handle, Allocation alloc_to_free) noexcept;
    /*
        \brief Frees and destroys several memory allocations at once.

        Pages is just a suggestion, like the other function. It's good for use with sparse binding, or for freeing several
        similar allocations at once (or, emulating things like a single-clear memory arena). It can potentially be internally
        optimized to be more efficient than calling #FreeMemory `alloc_count` times.

        Passing `VK_NULL_HANDLE` for some of the entries of the input allocation array is valid - these ones will just be 
        skipped.
    */
    VPR_API void FreeMemoryPages(Allocator alloc_handle, size_t alloc_count, Allocation* allocations_to_free);
    /*
        \brief Tries to do an in-place resize of an allocation if these is enough free memory after it in it's parent block
        You can both shrink and grow an allocation through this interface
    */
    VPR_API void ResizeAllocation(Allocator alloc_handle, Allocation alloc, AllocationInfo* alloc_info);
    /*
        \brief atomically "touches" the given allocation to mark it as used in the current frame
        Won't throw, and is of course thread-safe.
    */
    VPR_API VkResult TouchAllocation(Allocator alloc_handle, Allocation alloc) noexcept;
    /*
        \brief Maps the memory and returns the pointer to this mapped region
    */
    VPR_API VkResult MapMemory(Allocator alloc_handle, Allocation alloc, void** p_mapped_data);
    /*
        \brief Unmaps memory attached to the specified allocation
    */
    VPR_API VkResult UnmapMemory(Allocator alloc_handle, Allocation alloc);
    /*
        \brief Calls vkFlushMappedMemoryRanges on given allocation, in the given region specified by (offset, size) if non-zero
    */
    VPR_API void FlushAllocation(Allocator alloc_handle, Allocation alloc, VkDeviceSize offset, VkDeviceSize size);
    /*
        \brief Calls vkInvalidateMemoryRanges on given allocation, in the region specified by (offset, size)
    */
    VPR_API void InvalidateAllocation(Allocator alloc_handle, Allocation alloc, VkDeviceSize offset, VkDeviceSize size);
    /*
        \brief Checks metadata values padded around allocations of given memory types to see if they have been corrupted (overwritten, incorrectly sized, etc)
    */
    VkResult CheckCorruption(Allocator alloc_handle, uint32_t memory_type_bits) noexcept;
    
}

#endif //!VPR_ALLOC_2_ALLOCATOR_HPP
