#pragma once
#ifndef VPR_ALLOCATOR_COMMON_ITEMS_HPP
#define VPR_ALLOCATOR_COMMON_ITEMS_HPP
#include "vpr_stdafx.h"
#include "ForwardDecl.hpp"
#include <utility>
#include <stdexcept>
#include <cassert>
namespace vpr {
    
    constexpr static size_t vkMaxMemoryTypes = 32;
    constexpr static VkDeviceSize MinSuballocationSizeToRegister = 16;

    constexpr static VkDeviceSize SmallHeapMaxSize = 64 * 1024 * 1024;

    constexpr static VkDeviceSize DefaultLargeHeapBlockSize = 256 * 1024 * 1024;
    constexpr static VkDeviceSize LargeBlockSingleAllocSize = 512 * 16384; // ~8mb
    constexpr static VkDeviceSize DefaultMediumHeapBlockSize = 16777216; 
    constexpr static VkDeviceSize MediumBlockSingleAllocSize = 262144 * 2; // ~0.512mb
    constexpr static VkDeviceSize DefaultSmallHeapBlockSize = 16 * 4096;
    constexpr static VkDeviceSize SmallBlockSingleAllocSize = 4096;

#if !defined NDEBUG || defined FORCE_ALLOCATOR_VALIDATION
    constexpr bool VALIDATE_MEMORY = true;
#else 
    constexpr bool VALIDATE_MEMORY = false;
#endif // !NDEBUG

    /** These validation codes are returned by the memory validation routine, giving information on the error encountered. They will also
    *  be printed to the console, if it is enabled, and logged to the log file as well.
    *  \ingroup Allocation
    */
    enum class ValidationCode : uint8_t {
        VALIDATION_PASSED = 0,
        /** Suballocation's memory handle is invalid */
        NULL_MEMORY_HANDLE,
        /** Suballocation's memory size is zero */
        ZERO_MEMORY_SIZE,
        /** Offset of suballocation is incorrect: it may overlap with another, or it may be placed beyond the range of the allocation */
        INCORRECT_SUBALLOC_OFFSET,
        /** Two adjacent free suballoctions: merge them into one bigger suballocation */
        NEED_MERGE_SUBALLOCS,
        /** We found more free suballocations while validating than there are in the free suballocation list */
        FREE_SUBALLOC_COUNT_MISMATCH,
        /** Non-free suballocation in free suballocation list */
        USED_SUBALLOC_IN_FREE_LIST,
        /** Free suballocation list is sorted by available space descending: sorting is incorrect and smaller free region is before larger free region. */
        FREE_SUBALLOC_SORT_INCORRECT,
        /** Calculated offset as sum of all suballoc sizes is not equal to allocations total size */
        FINAL_SIZE_MISMATCH,
        /** Calculated total available size doesn't match stored available size */
        FINAL_FREE_SIZE_MISMATCH,
    };
    
    template<typename T>
    constexpr static T AlignUp(const T& offset, const T& alignment) {
        return (offset + alignment - 1) / alignment * alignment;
    } 
    

    /**
    *    Taken from the Vulkan specification, section 11.6
    *    Essentially, we need to ensure that linear and non-linear resources are properly placed on separate memory pages so that
    *    they avoid any accidental aliasing. Linear resources are just those that could be read like any other memory region, without
    *    any particular optimization for size or access speed. Optimally tiled resources are those that are tiled either by the hardware drivers,
    *    or the Vulkan implementation. Think of things like Z-Order curve encoding for texture data, or block-based compression for DDS/KTX texture formats.
    *    \param item_a_offset: non-linear object's offset
    *    \param item_a_size: non-linear object's size
    *    \param item_b_offset: linear object's offset
    *    \param item_b_size: linear object's size
    *    \param page_size: almost universally tends to be the bufferImageGranularity value retrieved by the parent Allocator class.
    *    \ingroup Allocation
    */
    constexpr static inline bool CheckBlocksOnSamePage(const VkDeviceSize& item_a_offset, const VkDeviceSize& item_a_size, const VkDeviceSize& item_b_offset, const VkDeviceSize& page_size) {
        assert(item_a_offset + item_a_size <= item_b_offset && item_a_size > 0 && page_size > 0);
        VkDeviceSize item_a_end = item_a_offset + item_a_size - 1;
        VkDeviceSize item_a_end_page = item_a_end & ~(page_size - 1);
        VkDeviceSize item_b_start_Page = item_b_offset & ~(page_size - 1);
        return item_a_end_page == item_b_start_Page;
    }

    constexpr static inline uint32_t countBitsSet(const uint32_t& val) {
        uint32_t count = val - ((val >> 1) & 0x55555555);
        count = ((count >> 2) & 0x33333333) + (count & 0x33333333);
        count = ((count >> 4) + count) & 0x0F0F0F0F;
        count = ((count >> 8) + count) & 0x00FF00FF;
        count = ((count >> 16) + count) & 0x0000FFFF;
        return count;
    }

}

#endif //!VPR_ALLOCATOR_COMMON_ITEMS_HPP
