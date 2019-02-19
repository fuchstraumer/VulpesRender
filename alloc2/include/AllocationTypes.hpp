#pragma once
#ifndef VPR_ALLOC_2_ALLOCATION_TYPES_HPP
#define VPR_ALLOC_2_ALLOCATION_TYPES_HPP
#include "vpr_stdafx.h"
#include "ForwardDecl.hpp"
#include <cstdint>
#include <limits>

namespace vpr {

class AllocatorImpl;
    using Allocator = AllocatorImpl*;
    class MemoryPoolImpl;
    using MemoryPool = MemoryPoolImpl*;

    /* \struct AllocatorVulkanFns
       \brief Pointers to subset of Vulkan functions used by this system, if dynamically loaded
    */
    struct AllocatorVulkanFns {
        PFN_vkGetPhysicalDeviceProperties vkGetPhysicalDeviceProperties{ nullptr };
        PFN_vkGetPhysicalDeviceMemoryProperties vkGetPhysicalDeviceMemoryProperties{ nullptr };
        PFN_vkAllocateMemory vkAllocateMemory{ nullptr };
        PFN_vkFreeMemory vkFreeMemory{ nullptr };
        PFN_vkMapMemory vkMapMemory{ nullptr };
        PFN_vkUnmapMemory vkUnmapMemory{ nullptr };
        PFN_vkFlushMappedMemoryRanges vkFlushMappedMemoryRanges{ nullptr };
        PFN_vkInvalidateMappedMemoryRanges vkInvalidateMappedMemoryRanges{ nullptr };
        PFN_vkBindBufferMemory vkBindBufferMemory{ nullptr };
        PFN_vkBindImageMemory vkBindImageMemory{ nullptr };
        PFN_vkGetBufferMemoryRequirements vkGetBufferMemoryRequirements{ nullptr };
        PFN_vkGetImageMemoryRequirements vkGetImageMemoryRequirements{ nullptr };
        PFN_vkCreateBuffer vkCreateBuffer{ nullptr };
        PFN_vkDestroyBuffer vkDestroyBuffer{ nullptr };
        PFN_vkCreateImage vkCreateImage{ nullptr };
        PFN_vkDestroyImage vkDestroyImage{ nullptr };
        PFN_vkCmdCopyBuffer vkCmdCopyBuffer{ nullptr };
        // Unlike libraries intended only for desktop use, I won't assume this is enabled and loaded
        PFN_vkGetBufferMemoryRequirements2KHR vkGetBufferMemoryRequirements2KHR{ nullptr };
        PFN_vkGetImageMemoryRequirements2KHR vkGetImageMemoryRequirements2KHR{ nullptr };
    };

    /* \struct DeviceMemoryCallbacks
       \brief Rather brilliant future from GPU-Open: called for gathering stats on device memory allocations
       Should not be used to perform any actions beyond information gathering however, as mutating the
       allocations or doing stuff with them could break things rather disastrously. It does provide
       an easy routing path for gathering debug stats and the like, though.
    */
    struct DeviceMemoryCallbacks {
        void (*AllocateDeviceMemory)(Allocator* allocator, uint32_t memory_type, VkDeviceMemory handle, VkDeviceSize sz);
        void (*FreeDeviceMemory)(Allocator* alloc, uint32_t memory_type, VkDeviceMemory handle, VkDeviceSize sz);
    };

    enum AllocatorCreateFlags {
        AllocatorCreateFlagsBeginRange = 0u,
        /* \brief Disables usage of internal mutexes, which allow allocators to be used across multiple threads. */
        AllocatorCreateFlagsExternallySynchronizedBit = 1u,
        /* \brief Use VK_KHR_dedicated_allocation extension to potentially improve performance of certain memory allocations */
        AllocatorCreateFlagsUseDedicatedAllocExtensions = 2u,
        AllocatorCreateFlagsEndRange = std::numeric_limits<uint32_t>::max()
    };

    struct AllocatorCreateInfo {
        AllocatorCreateFlags Flags{ };
        VkDevice LogicalDevice{ VK_NULL_HANDLE };
        VkPhysicalDevice PhysicalDevice{ VK_NULL_HANDLE };
        /* \brief Used for CPU-side allocations. Can be safely left nullptr, and usually that's the best path */
        const VkAllocationCallbacks* AllocationCallbacks{ nullptr };
        /* \brief Calls supplied functions for statistics tracking of device memory allocations */
        const DeviceMemoryCallbacks* DvcMemoryCallbacks{ nullptr };
        /* \brief If non-null, the library will use these for it's Vulkan functions. Otherwise, it expects static linkage for functions. */
        const AllocatorVulkanFns* VulkanFunctionPtrs{ nullptr };
        VkBool32 EnableRecording{ VK_FALSE };
        const char* RecordingOutputPath;
        VkBool32 UseFrameBufferedStrategy{ VK_FALSE };
    };

    struct MemoryInfo {
        uint32_t BlockCount{ 0u };
        uint32_t AllocationCount{ 0u };
        uint32_t AvailableSuballocCount{ 0u };
        VkDeviceSize TotalUsedMemoryBytes{ 0u };
        VkDeviceSize AvailUnusedMemoryBytes{ 0u };
        VkDeviceSize SmallestAllocationSize{ 0u };
        VkDeviceSize LargestAllocationSize{ 0u };
        VkDeviceSize AverageAllocationSize{ 0u };
        VkDeviceSize SmallestAvailAllocSize{ 0u };
        VkDeviceSize LargestAvailAllocSize{ 0u };
        VkDeviceSize AverageAvailAllocSize{ 0u };
    };

    /*
    \brief Allocations are segmented by memory type and size range, and these are the enums wrapping the available size ranges
    */
    enum class AllocationSizeGroup : uint32_t {
        Invalid = 0,
        Tiny = 1,
        Small = 2,
        Medium = 3,
        Large = 4,
        XLarge = 5
    };
    
    constexpr static size_t MAX_NUM_ALLOCATION_SIZE_GROUPS = 5;

    struct AllocatorMemoryStats {
        MemoryInfo MemoryTypes[VK_MAX_MEMORY_TYPES];
        MemoryInfo SizeGroups[MAX_NUM_ALLOCATION_SIZE_GROUPS];
        MemoryInfo Total;
    };

    enum class MemoryUsage : uint32_t {
        InvalidOrUnknown = 0u,
        GPU_Only = 1u,
        CPU_Only = 2u,
        CPU_To_GPU = 3u,
        GPU_To_CPU = 4u
    };

    enum AllocationCreateFlagBits {
        ALLOCATION_CREATE_BEGIN_RANGE_BIT = 0u,
        /* 
            \brief Give this allocation it's own unique block (`VkDeviceMemory`) object
            Most helpful with special large resources, or things that need to be uniquely
            isolated for various reasons (e.g, images used as attachments potentially).

            When this flag is used, AllocationCreateInfo::Pool cannot be null
        */
        ALLOCATION_CREATE_USE_DEDICATED_MEMORY_BIT = 0x00000001,
        /* 
            \brief Don't create a new `VkDeviceMemory` object, this object must fit in an existing one 
            If the requested allocation cannot be fit in any existing memory regions, `VK_ERROR_OUT_OF_DEVICE_MEMORY` 
            will be returned.
        */
        ALLOCATION_CREATE_NEVER_ALLOCATE_BIT = 0x00000002,
        /* 
            \brief This allocation will be mapped throughout it's lifetime, and the mapped pointer can be found in the create info struct 
            The mapped pointer is just AllocationInfo::MappedData. This flag can actually be combined with memory types that aren't host-visible - 
            it will just be ignored unless the current platform allows for mapping of `DEVICE_LOCAL` allocations (namely, mobile GPUs and Intel GPUs).
        */
        ALLOCATION_CREATE_PERSISTENTLY_MAPPED_BIT = 0x00000004,
        /*
            Allocations with this flag can be made "lost", effectively meaning they are evicted due to high memory pressure and a need 
            to release some memory for other allocations. Before use, they must be checked for validity. An invalid allocation will
            have it's `VkDeviceMemory` handle set to `VK_NULL_HANDLE`.
        */
        ALLOCATION_CREATE_CAN_BECOME_LOST_BIT = 0x00000008,
        /* 
            Allocations with this flag are able to make *other* allocations lost (ones created with `ALLOCATION_CREATE_CAN_BECOME_LOST_BIT`) if
            we need to make some room to create it. 
        */
        ALLOCATION_CREATE_CAN_MAKE_OTHERS_LOST_BIT = 0x00000010,
        /*
            Interpret the `UserData` pointer as a null-terminated C string and store a copy of it (the actual string data, not the pointer) somewhere. 
            This data will also be used when writing out statistics, to aid visualizations.
        */
        ALLOCATION_USER_DATA_AS_STRING_BIT = 0x00000020,
        /*
            Allocation will be created from the upper stack and address ranges in a stack-type memory pool. This only works with custom memory pools
            created with #POOL_CREATE_LINEAR_ALGORITHM_BIT.
        */
        ALLOCATION_CREATE_UPPER_ADDRESS_BIT = 0x00000040,
        /*
            Allocation strategy that finds the smallest range that the requested allocation can fit in, even it's not the first one we find.
        */
        ALLOCATION_CREATE_STRATEGY_BEST_FIT_BIT = 0x00010000,
        /*
            Allocation strategy that chooses biggest possible range we can fit the requested allocation in.
        */
        ALLOCATION_CREATE_STRATEGY_WORST_FIT_BIT = 0x00020000,
        /*
            Allocation strategy that chooses the easiest and fastest possible fit for our requested allocation, regardless
            of how well (or not well) it fits.
        */
        ALLOCATION_CREATE_STRATEGY_FIRST_FIT_BIT = 0x00040000,
        /*
            Allocation strategy that tries to minimize memory consumption (alias to best fit)
        */
        ALLOCATION_CREATE_STRATEGY_MIN_MEMORY_BIT = ALLOCATION_CREATE_STRATEGY_BEST_FIT_BIT,
        /*
            Allocation strategy that tries to minimize the time required to complete an allocation (alias to first fit)
        */
        ALLOCATION_CREATE_STRATEGY_MIN_TIME_BIT = ALLOCATION_CREATE_STRATEGY_FIRST_FIT_BIT,
        /*
            Allocation strategy that (indirectly) tries to minimize memory fragmentation (alias to worst fit)
        */
        ALLOCATION_CREATE_STRATEGY_MIN_FRAGMENTATION_BIT = ALLOCATION_CREATE_STRATEGY_WORST_FIT_BIT,
        /*
            Bitmask to mask out strategy bits
        */
        ALLOCATION_CREATE_STRATEGY_BITMASK = ALLOCATION_CREATE_STRATEGY_FIRST_FIT_BIT | ALLOCATION_CREATE_STRATEGY_WORST_FIT_BIT | ALLOCATION_CREATE_STRATEGY_BEST_FIT_BIT,
        ALLOCATION_CREATE_FLAGS_END_RANGE = 0x7FFFFFFF
    };

    using AllocationCreateFlags = uint32_t;

    struct AllocationCreateInfo {
        // A #AllocationCreateFlagBits enum (or combination of the component bits, usually)
        AllocationCreateFlags Flags{ 0 };
        MemoryUsage Usage{ MemoryUsage::InvalidOrUnknown };
        /*
            \brief Flags that MUST be present for the allocation to be created
            If the flags can't be satisfied, the allocation WILL fail to be created and an error should be returned.
        */
        VkMemoryPropertyFlags RequiredFlags{ VK_MEMORY_PROPERTY_FLAG_BITS_MAX_ENUM };
        /*
            \brief Flags that would be nice to have, but aren't required. No issues/penalties for being unable to satisfy these.
        */
        VkMemoryPropertyFlags PreferredFlags{ 0 };
        /*
            \brief Bitmask with one bit set for each acceptable memory type for this allocation
            Leaving it initialized to zero is okay: it means any memory type can work so long as it meets the other requirements
            from this structure's fields.
        */
        uint32_t MemoryTypeBits{ 0u };
        /*
            \brief Pool the allocation should belong to. If not null, Usage, RequiredFlags, PreferredFlags, and MemoryTypeBits are all ignored.
        */
        MemoryPool Pool{ VK_NULL_HANDLE };
        void* UserData;
    };


    enum MemoryPoolCreateFlagBits {
        MEMORY_POOL_CREATE_IGNORE_BUFFER_IMAGE_GRANULARITY_BIT = 0x00000002,
        MEMORY_POOL_CREATE_LINEAR_ALGORITHM_BIT,
        MEMORY_POOL_CREATE_BUDDY_ALGORITHM_BIT,
        MEMORY_POOL_CREATE_END_RANGE_BIT = 0x7FFFFFFF
    };

    using MemoryPoolCreateFlags = uint32_t;

    struct MemoryPoolCreateInfo {
        /* \brief Memory type index this will be allocated from */
        uint32_t MemoryTypeIndex{ 0u };
        /* \brief Subset of #MemoryPoolCreateFlagBits */
        MemoryPoolCreateFlags Flags{ 0 };
        /* \brief Size of a single `VkDeviceMemory` object to be allocated from this pool 
            Specify nonzero to set this explicitly, which will make the size of blocks static. Otherwise,
            when left to zero the library will manage the size of this block as it sees fit.

            Usually, this will be based on the four size categories (see AllocationSizeGroup)
        */
        VkDeviceSize BlockSize{ 0u };
        /*
            \brief Number of blocks to pre-allocate in this pool upon construction
            If left to zero, no initial blocks will be created.
        */
        size_t MinBlockCount;
        /*
            \brief Maximum number of blocks allowed in this pool - optional
            The default is no limit, 0. Set to the same as PoolCreateInfo::MinBlockCount
            to have a fixed size pool that won't grow or shrink in use.
        */
        size_t MaxBlockCount;
        /*
            \brief Maximum number of frames that are in use at the same time as the current frame.
        */
        uint32_t FrameInUseCount;
    };

    struct MemoryPoolStats {
        VkDeviceSize Size;
        VkDeviceSize UnusedSize;
        size_t AllocationCount;
        size_t UnusedRangeCount;
        VkDeviceSize UnusedRangeSizeMax;
        size_t BlockCount;
    };

}

#endif //!VPR_ALLOC_2_ALLOCATION_TYPES_HPP
