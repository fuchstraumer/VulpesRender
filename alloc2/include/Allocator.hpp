#pragma once
#ifndef VPR_ALLOC_2_ALLOCATOR_HPP
#define VPR_ALLOC_2_ALLOCATOR_HPP
#include "vpr_stdafx.h"
#include "AllocationTypes.hpp"

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

}

#endif //!VPR_ALLOC_2_ALLOCATOR_HPP
