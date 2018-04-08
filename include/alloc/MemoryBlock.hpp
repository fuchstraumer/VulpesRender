#pragma once
#ifndef VPR_MEMORY_BLOCK_HPP
#define VPR_MEMORY_BLOCK_HPP
#include "vpr_stdafx.h"
#include "ForwardDecl.hpp"
#include "AllocCommon.hpp"
#include "Suballocation.hpp"
#include <mutex>

namespace vpr {

    /**
    *    A MemoryBlock is a large contiguous region of Vulkan memory of a uniform type (device local, host coherent, host visible, etc) that 
    *   other objects bind to subregions of. This should never be directly accessed by any client code: it is managed and interfaced to by 
    *   other objects, and several of these of each type can exist (occurs when a memory block is fully used, until no more memory available).
    *   \ingroup Allocation
    */
    class VPR_API MemoryBlock {
        MemoryBlock(const MemoryBlock&) = delete;
        MemoryBlock& operator=(const MemoryBlock&) = delete;
    public:

        MemoryBlock(Allocator* alloc = nullptr);
        /** The object should be destroyed via the Destroy method before the destructor is called, but this will call the Destroy method if it hasn't been, and will log a warning that this was done. */
        ~MemoryBlock(); 

        /** \param new_memory: This is the handle that this block will take ownership of. \param new_size: total size of this memory block. */
        void Init(VkDeviceMemory& new_memory, const VkDeviceSize& new_size);

        /** Cleans up resources and prepares object to be safely destroyed. Should be called before the destructor is called, but for safety's sake the destructor will also call this. */
        void Destroy(Allocator* alloc);

        // Used when sorting AllocationCollection
        bool operator<(const MemoryBlock& other);

        VkDeviceSize AvailableMemory() const noexcept;
        const VkDeviceMemory& Memory() const noexcept;

        /** Verifies integrity of memory by checking all contained suballocations for integrity and correctness */
        ValidationCode Validate() const;

        /** Fills the given SuballocationRequest struct with information about where to place the suballocation, and returns whether or not it succeeded in finding a spot to put the requested suballocation. Usually, a failure means we'll just try another memory block (and ultimately, consider creating a new one) */
        bool RequestSuballocation(const VkDeviceSize& buffer_image_granularity, const VkDeviceSize& allocation_size, const VkDeviceSize& allocation_alignment, SuballocationType allocation_type, SuballocationRequest* dest_request);

        /** Verifies that requested suballocation can be added to this object, and sets dest_offset to reflect offset of this now-inserted suballocation. */
        bool VerifySuballocation(const VkDeviceSize& buffer_image_granularity, const VkDeviceSize& allocation_size, const VkDeviceSize& allocation_alignment,
            SuballocationType allocation_type, const suballocationList::const_iterator & dest_suballocation_location, VkDeviceSize* dest_offset) const;

        bool Empty() const;

        /** Performs the actual allocation, once "request" has been checked and made valid. */
        void Allocate(const SuballocationRequest& request, const SuballocationType& allocation_type, const VkDeviceSize& allocation_size);

        /** Frees memory in region specified (i.e frees/destroys a suballocation) */
        void Free(const Allocation* memory_to_free);
        
        /** When we map a suballocation, we are mapping a sub-region of the larger memory object it is bound to. We cannot perform another map until this sub-region
         *  is unmapped. Thus, suballocations actually call their parent's mapping method. This lets us use a mutex to lock off access to this block's VkDeviceMemory object until the memory is unmapped.
         */
        void Map(const Allocation* alloc_being_mapped, const VkDeviceSize& size_of_map, const VkDeviceSize& offset_to_map_at, void* destination_address);
        /** As mentioned for the Map method, due to VkMapMemory functions we must make sure only one sub-region of an object is unmapped at a time, thus requiring the 
         *  parent block to oversee the mapping/unmapping + using a mutex for thread safety.
        */
        void Unmap();
        
        VkDeviceSize LargestAvailRegion() const noexcept;

        suballocation_iterator_t begin();
        suballocation_iterator_t end();
        const_suballocation_iterator_t begin() const;
        const_suballocation_iterator_t end() const;
        const_suballocation_iterator_t cbegin() const;
        const_suballocation_iterator_t cend() const;

        avail_suballocation_iterator_t avail_begin();
        avail_suballocation_iterator_t avail_end();
        const_avail_suballocation_iterator_t avail_begin() const;
        const_avail_suballocation_iterator_t avail_end() const;
        const_avail_suballocation_iterator_t avail_cbegin() const;
        const_avail_suballocation_iterator_t avail_cend() const;

        VkDeviceSize Size;
        suballocationList Suballocations;
        Allocator* allocator;
        uint32_t MemoryTypeIdx;

    protected:

        std::mutex guardMutex;
        VkDeviceSize availSize{ 0 };
        uint32_t freeCount{ 0 };
        VkDeviceMemory memory{ VK_NULL_HANDLE };
        /** Changes the item pointed to by the iterator to be a free type, then adds the now-available size to availSize and increments freeCount. This method
         *  may also call mergeFreeWithNext, insertFreeSuballocation, and removeFreeSuballocation if adjacent suballocations are free or can be merged with the 
         *  now-free allocation passed in.
         */
        void freeSuballocation(const suballocationList::iterator& item_to_free);

        /** Effectively reduces fragmentation by merging two free allocations that are adjacent in memory into one larger allocation. */
        void mergeFreeWithNext(const suballocationList::iterator& item_to_merge);
        
        /** Registers a free suballocation, inserting it into the proper location to keep the sorting intact (via std::lower_bound) */
        void insertFreeSuballocation(const suballocationList::iterator& item_to_insert);
        /** Removes a free suballocation, usually indicating that it is about to be made active and used by an object (or that it has been merged with another free suballocation) */
        void removeFreeSuballocation(const suballocationList::iterator& item_to_remove);

        /** This vector stores iterators that can be used to locate suballocations in this object's suballocationList. Using iterators avoids accidental duplication of objects,
         *  (akin to a pointer), but with more safety and extra convienience when it comes to retrieving, modifying, or even removing the object "pointed" to by the iterator.
        */
        std::vector<suballocationList::iterator> availSuballocations;
    };

    typedef std::vector<MemoryBlock*>::iterator allocation_iterator_t;
    typedef std::vector<MemoryBlock*>::const_iterator const_allocation_iterator_t;

    std::ostream& operator<<(std::ostream& os, const ValidationCode& code);

}

#endif //!VPR_MEMORY_BLOCK_HPP