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
    class AllocationCollection {
        AllocationCollection(const AllocationCollection&) = delete;
        AllocationCollection& operator=(const AllocationCollection&) = delete;
    public:

        AllocationCollection() = default;
        AllocationCollection(Allocator* allocator);
        AllocationCollection(AllocationCollection&& other) noexcept;
        AllocationCollection& operator=(AllocationCollection&& other) noexcept;

        ~AllocationCollection();

        MemoryBlock* operator[](const size_t& idx);
        const MemoryBlock* operator[](const size_t& idx) const;

        size_t AddMemoryBlock(std::unique_ptr<MemoryBlock>&& new_block) noexcept;
        bool Empty() const;

        /** Removes only the particular memory block from the internal vector, and re-sorts the blocks once complete. */
        void RemoveBlock(MemoryBlock * block_to_erase);
        void SortAllocations();

        typedef std::vector<std::unique_ptr<MemoryBlock>>::iterator iterator;
        typedef std::vector<std::unique_ptr<MemoryBlock>>::const_iterator const_iterator;

        iterator begin() noexcept;
        iterator end() noexcept;
        const_iterator begin() const noexcept;
        const_iterator end() const noexcept;
        const_iterator cbegin() const noexcept;
        const_iterator cend() const noexcept;
        
    private:  
        std::mutex containerMutex;
        std::vector<std::unique_ptr<MemoryBlock>> allocations;
        Allocator* allocator;
    };

}

#endif //!VPR_ALLOCATION_COLLECTION_HPP