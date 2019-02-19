#pragma once
#ifndef VPR_ALLOC_2_ALLOCATOR_HPP
#define VPR_ALLOC_2_ALLOCATOR_HPP
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

}

#endif //!VPR_ALLOC_2_ALLOCATOR_HPP
