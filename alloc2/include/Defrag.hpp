#pragma once
#ifndef VPR_ALLOC2_DEFRAG_HPP
#define VPR_ALLOC2_DEFRAG_HPP
#include "vpr_stdafx.h"
#include "Allocator.hpp"

namespace vpr
{

    struct DefragmentationContextImpl;
    using DefragmentationContext = DefragmentationContextImpl*;

    struct DefragInfo
    {
        uint32_t flags{ 0x00000000 }; // reserved for eventual use
        uint32_t allocCount{ 0u };
        /*
            \brief Array of allocations that can be defragmented
            Any allocations not passed here are considered immutable and will not be moved
        */
        Allocation* allocations{ nullptr };
        /*
            \brief Optional parameter: when passed, each entry will be updated to reflect if the corresponding entry in #allocations was moved
        */
        VkBool32* changedAllocations{ nullptr };
        uint32_t poolCount{ 0u };
        /*
            \brief Optional parameter: if non-null, each pool passed in will be defragmented (vs singular allocations from individual pools)
            There is no way to retrieve which allocations were moved like when we defragment individual allocs, so you will have to call GetAllocationInfo() on each 
            allocation in the given pool if you are trying to recreate things.
        */
        MemoryPool* pools{ nullptr };
        /*
            \brief Max amount of memory that can be moved with calls to `memcpy()` and `memmove()`, etc. 
            `VK_WHOLE_SIZE` means no limit on the size of memory regions we move.
        */
        VkDeviceSize maxCpuBytesToMove{ VK_WHOLE_SIZE };
        VkDeviceSize maxCpuAllocsToMove{ 0u };
        /*
            \brief Max size of memory that can be moved around using `cmdBuffer` on the GPU side
        */
        VkDeviceSize maxGpuBytesToMove{ VK_WHOLE_SIZE };
        VkDeviceSize maxGpuAllocsToMove{ 0u };
        /*
            \brief Command buffer where GPU copy commands will be posted. Optional. 
            Must be capable of transfer operations, and it would be wisest to use the dedicated transfer
            queue for this when available. It must be recording when passed to defrag functions, and then
            must be submitted by the user - it will not be submitted by this library.

            When null (default), only CPU-side defragmentation will be performed.
        */
        VkCommandBuffer cmdBuffer{ VK_NULL_HANDLE };
    };

    struct DefragStats
    {
        VkDeviceSize bytesMoved{ 0u };
        VkDeviceSize bytesFreed{ 0u };
        uint32_t allocsMoved{ 0u };
        uint32_t numDeviceMemoryBlocksFreed{ 0u };
    };

    /*
    */
    VkResult BeginDefrag(Allocator alloc_handle, const DefragInfo* info, DefragStats* stats, DefragmentationContext* context);
    /*
    */
    VkResult EndDefrag(Allocator alloc_handle, DefragmentationContext context);


}

#endif // !VPR_ALLOC2_DEFRAG_HPP
