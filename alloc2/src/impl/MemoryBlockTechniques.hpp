#pragma once
#ifndef VPR_ALLOC2_MEMORY_BLOCK_TECHNIQUES_HPP
#define VPR_ALLOC2_MEMORY_BLOCK_TECHNIQUES_HPP
#include "vpr_stdafx.h"
#include "Allocator.hpp"
#include "Common.hpp"

namespace vpr {

    struct AllocationRequest;

    enum class TechniqueType
    {
        Linear,
        Buddy
    };

    struct BlockAllocTechnique
    {
        BlockAllocTechnique(TechniqueType type) noexcept;
        virtual ~BlockAllocTechnique() {};
        
        virtual void Create(VkDeviceSize size);
        virtual bool Validate() const = 0;
        virtual size_t AllocCount() const noexcept = 0;
        virtual VkDeviceSize TotalAvailSize() const noexcept = 0;
        virtual VkDeviceSize LargedUnusedRegion() const noexcept = 0;
        virtual bool Empty() const noexcept = 0;
        virtual void CalculateStatistics(AllocatorMemoryStats& stats) const = 0;
        virtual void AddPoolStatistics(MemoryPoolStats& stats) const = 0;
        virtual void PrintDetailedMap(AllocatorJsonWriter* writer) const = 0;
        virtual bool CreateAllocationRequest(
            uint32_t curr_frame_idx,
            uint32_t frame_in_use_count,
            VkDeviceSize buffer_image_granularity,
            VkDeviceSize alloc_size,
            VkDeviceSize alloc_align,
            bool upper_address,
            SuballocationType type,
            uint32_t strategy,
            AllocationRequest* alloc_request
        ) = 0;

        TechniqueType Type{ TechniqueType::Linear };
        VkDeviceSize size{ std::numeric_limits<VkDeviceSize>::max() };

    };

}

#endif // !VPR_ALLOC2_MEMORY_BLOCK_TECHNIQUES_HPP
