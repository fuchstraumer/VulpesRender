#pragma once
#ifndef VPR_ALLOCATION_COLLECTION_HPP
#define VPR_ALLOCATION_COLLECTION_HPP
#include "vpr_stdafx.h"
#include "ForwardDecl.hpp"

namespace vpr {
  
    /** An allocation collection is just a vector of MemoryBlocks of the same type. With commonly used memory types we wil quite easily fill one block up (e.g, device-local memory) 
     *  so we will need to create a new block. In order to keep some organization among memory types, though, we store these similar memory blocks in this object.
     *  \ingroup Allocation
     */
    struct AllocationCollection {
        std::vector<std::unique_ptr<MemoryBlock>> allocations;

        AllocationCollection() = default;
        AllocationCollection(Allocator* allocator);

        ~AllocationCollection();

        MemoryBlock* operator[](const size_t& idx);
        const MemoryBlock* operator[](const size_t& idx) const;

        bool Empty() const;

        /** Removes only the particular memory block from the internal vector, and re-sorts the blocks once complete. */
        void RemoveBlock(MemoryBlock * block_to_erase);

        void SortAllocations();

        
    private:
        Allocator* allocator;
    };

}

#endif //!VPR_ALLOCATION_COLLECTION_HPP