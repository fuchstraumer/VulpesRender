#pragma once
#ifndef VULPES_VK_ALLOCATOR_H
#define VULPES_VK_ALLOCATOR_H
#include "vpr_stdafx.h"
#include "ForwardDecl.hpp"
#include <variant>
#include <list>

namespace vpr {

    /** The allocator module encompasses everything required for the operation/use of this project's rather
     *  complex GPU memory management subsystem. In Vulkan, allocating VkDeviceMemory objects for every single 
     *  individual resource requiring GPU memory is wasteful, and not practical: there is a limit in each Vulkan 
     *  implementation to just how many allocations can exist at one time (sometimes as low as 1024). Thus, we will
     *  instead allocate large chunks of memory when a previously unallocated memory type is requested: from there, 
     *  resources (like buffers, images, etc) will bind to subregions of this larger memory object. Binding is much
     *  faster than allocation, and it is also much quicker to de-allocate resources since this only involves registering
     *  a newly freed location in this subsystem. 
     * 
     *  The allocator object is spawned as a member of the LogicalDevice class, but it can be accessed by anyone that can access
     *  a logical device. Its relatively safe to access, and should be thread-safe. Utility methods exist to simplify the creation
     *  and allocation of VkImage and VkBuffer objects as much as possible, as well.
     *  
     *  This module also is minimally complete, at best. It is robust enough to not fail in common usage, and currently has functioned 
     *  wonderfully in most tasks thrown at it. However, the todo list details stuff that still needs to be done (e.g, further splitting pools and 
     *  creating some kind of defragmentation system).
     * 
     *  Lastly, huge credit to GPU-Open as this is primarily just a slightly more object-oriented/"Modern C++" styled implementation of their *excellent*
     *  memory allocator. This would not have been possible without their work: https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator
     * 
     *  \defgroup Allocator Memory Subsystem
     *  \todo Further dividing allocation among three size pools, and then still dividing among type in those pools
     *  \todo Measure costliness of Validate(), possibly use return codes to fix errors when possible.
     */

    enum class SuballocationType : uint8_t {
        Free = 0, // unused entry
        Unknown, // could be various cpu storage objects, or extension objects
        Buffer,
        ImageUnknown, // image memory without defined tiling - possibly related to extensions
        ImageLinear,
        ImageOptimal,
    };

    /** These validation codes are returned by the memory validation routine, giving information on the error encountered. They will also
    *  be printed to the console, if it is enabled, and logged to the log file as well.
    *  \ingroup Allocator
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

    /** This struct is the primary item submitted to allocator methods for resource creation.
     *  \ingroup Allocator
     */
    struct AllocationRequirements {
        /** Defaults to false. If true, no new allocations are created beyond
        * the set created upon initilization of the allocator system. */
        static VkBool32 noNewAllocations;

        /** True if whatever allocation this belongs to should be in its own device memory object. Don't use this too often, of course. */
        VkBool32 privateMemory = false;

        /** The memory properties that are absolutely required by the item you are allocating for. */
        VkMemoryPropertyFlags requiredFlags;
        /** Additional flags that would be nice/useful to have, but are not required. An attempt to meet these will be 
         *  made, but not meeting them won't be considered a failure.*/
        VkMemoryPropertyFlags preferredFlags = VkMemoryPropertyFlags(0);
    };

    struct Suballocation {
        bool operator<(const Suballocation& other) {
            return offset < other.offset;
        }
        VkDeviceSize offset, size;
        SuballocationType type;
    };

    struct suballocOffsetCompare {
        bool operator()(const Suballocation& s0, const Suballocation& s1) const {
            return s0.offset < s1.offset; // true when s0 is before s1
        }
    };

    struct privateSuballocation {
        VkDeviceMemory memory;
        VkDeviceSize size;
        SuballocationType type;
        bool operator==(const privateSuballocation& other) {
            return (memory == other.memory) && (size == other.size) && (type == other.type);
        }
    };

    using suballocationList = std::list<Suballocation>;

    struct SuballocationRequest {
        suballocationList::iterator freeSuballocation; // location of suballoc this request can use.
        VkDeviceSize offset;
    };


    struct suballocIterCompare {
        bool operator()(const suballocationList::iterator& iter0, const suballocationList::iterator& iter1) const {
            return iter0->size < iter1->size;
        }
    };

    using avail_suballocation_iterator_t = std::vector<suballocationList::iterator>::iterator;
    using const_avail_suballocation_iterator_t = std::vector<suballocationList::iterator>::const_iterator;
    using suballocation_iterator_t = suballocationList::iterator;
    using const_suballocation_iterator_t = suballocationList::const_iterator;


    /*
    
        Main allocation classes and objects
    
    */
    
    /**    
    *    Allocation class represents a singular allocation: can be a private allocation (i.e, only user
    *    of attached DeviceMemory) or a block allocation (bound to sub-region of device memory)
    *   \ingroup Allocator
    */
    struct Allocation {

        /** If this is an allocation bound to a smaller region of a larger object, it is a block allocation. 
         *  Otherwise, it has it's own VkDeviceMemory object and is a "PRIVATE_ALLOCATION" type.
         */
        enum class allocType {
            BLOCK_ALLOCATION,
            PRIVATE_ALLOCATION,
            INVALID_TYPE,
        };


        Allocation() = default;
        ~Allocation() = default;
        Allocation(const Allocation&) = default;
        Allocation& operator=(const Allocation&) = default;
        Allocation(Allocation&& other) noexcept;
        Allocation& operator=(Allocation&& other) noexcept;

        void Init(MemoryBlock* parent_block, const VkDeviceSize& offset, const VkDeviceSize& alignment, const VkDeviceSize& alloc_size, const SuballocationType& suballoc_type);
        void Update(MemoryBlock* new_parent_block, const VkDeviceSize& new_offset);
        /** \param persistently_mapped: If set, this object will be considered to be always mapped. This will remove any worries about mapping/unmapping the object. */
        void InitPrivate(const uint32_t& type_idx, VkDeviceMemory& dvc_memory, const SuballocationType& suballoc_type, bool persistently_mapped, void* mapped_data, const VkDeviceSize& data_size);
        void Map(const VkDeviceSize& size_to_map, const VkDeviceSize& offset_to_map_at, void* address_to_map_to) const;
        void Unmap() const noexcept;

        const VkDeviceMemory& Memory() const;
        VkDeviceSize Offset() const noexcept;
        uint32_t MemoryTypeIdx() const;

        allocType Type = allocType::INVALID_TYPE;
        SuballocationType SuballocType;
        VkDeviceSize Size, Alignment;

        struct blockAllocation {
            MemoryBlock* ParentBlock;
            VkDeviceSize Offset;
        };

        struct privateAllocation {
            uint32_t MemoryTypeIdx;
            VkDeviceMemory DvcMemory;
            bool PersistentlyMapped;
            void* MappedData;
        };

        std::variant<blockAllocation, privateAllocation> typeData;
    };

    /**
    *    A MemoryBlock is a large contiguous region of Vulkan memory of a uniform type (device local, host coherent, host visible, etc) that 
    *   other objects bind to subregions of. This should never be directly accessed by any client code: it is managed and interfaced to by 
    *   other objects, and several of these of each type can exist (occurs when a memory block is fully used, until no more memory available).
    *   \ingroup Allocator
    */
    class MemoryBlock {
    public:

        MemoryBlock(Allocator* alloc);
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

        /** Used to protect access to the VkDeviceMemory handle, so that two threads don't attempt to map or use this handle at the same time. */
        std::mutex memoryMutex; 
        VkDeviceSize availSize;
        uint32_t freeCount;
        VkDeviceMemory memory;
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

    /** An allocation collection is just a vector of MemoryBlocks of the same type. With commonly used memory types we wil quite easily fill one block up (e.g, device-local memory) 
     *  so we will need to create a new block. In order to keep some organization among memory types, though, we store these similar memory blocks in this object.
     *  \ingroup Allocator
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

    /** The primary interface and class of this subsystem. This object is responsible for creating resources when requested, managing memory,
     *  checking integrity of memory, and cleaning up after itself and when deallocation has been requested.
     *  \ingroup Allocator
     */
    class Allocator {
        Allocator(const Allocator&) = delete;
        Allocator(Allocator&&) = delete;
        Allocator& operator=(const Allocator&) = delete;
        Allocator& operator=(Allocator&&) = delete;
    public:

        Allocator(const Device* parent_dvc);
        ~Allocator();

        void Recreate();

        VkDeviceSize GetPreferredBlockSize(const uint32_t& memory_type_idx) const noexcept;
        VkDeviceSize GetBufferImageGranularity() const noexcept;

        uint32_t GetMemoryHeapCount() const noexcept;
        uint32_t GetMemoryTypeCount() const noexcept;

        const VkDevice& DeviceHandle() const noexcept;

        VkResult AllocateMemory(const VkMemoryRequirements& memory_reqs, const AllocationRequirements& alloc_details, const SuballocationType& suballoc_type, Allocation& dest_allocation);

        void FreeMemory(const Allocation* memory_to_free);

        // Allocates memory for an image, using given handle to get requirements. Allocation information is written to dest_memory_range, so it can then be used to bind the resources together.
        VkResult AllocateForImage(VkImage& image_handle, const AllocationRequirements& details, const SuballocationType& alloc_type, Allocation& dest_allocation);

        // Much like AllocateForImage: uses given handle to get requirements, writes details of allocation ot given range, making memory valid for binding.
        VkResult AllocateForBuffer(VkBuffer& buffer_handle, const AllocationRequirements& details, const SuballocationType& alloc_type, Allocation& dest_allocation);

        // Creates an image object using given info. When finished, given handle is a valid image object (so long as the result value is VkSuccess). Also writes details to 
        // dest_memory_range, but this method will try to bind the memory and image together too
        VkResult CreateImage(VkImage* image_handle, const VkImageCreateInfo* img_create_info, const AllocationRequirements& alloc_reqs, Allocation& dest_allocation);

        // Creates a buffer object using given info. Given handle is valid for use if method returns VK_SUCCESS, and memory will also have been bound to the object. Details of the 
        // memory used for this particular object are also written to dest_memory_range, however.
        VkResult CreateBuffer(VkBuffer* buffer_handle, const VkBufferCreateInfo* buffer_create_info, const AllocationRequirements& alloc_reqs, Allocation& dest_allocation);

        // Destroys image/buffer specified by given handle.
        void DestroyImage(const VkImage& image_handle, Allocation& allocation_to_free);
        void DestroyBuffer(const VkBuffer& buffer_handle, Allocation& allocation_to_free);

    private:

        // Won't throw: but can return invalid indices. Make sure to handle this.
        uint32_t findMemoryTypeIdx(const VkMemoryRequirements& mem_reqs, const AllocationRequirements& details) const noexcept;

        // These allocation methods return VkResult's so that we can try different parameters (based partially on return code) in main allocation method.
        VkResult allocateMemoryType(const VkMemoryRequirements& memory_reqs, const AllocationRequirements& alloc_details, const uint32_t& memory_type_idx, const SuballocationType& type, Allocation& dest_allocation);
        VkResult allocatePrivateMemory(const VkDeviceSize& size, const SuballocationType& type, const uint32_t& memory_type_idx, Allocation& dest_allocation);

        // called from "FreeMemory" if memory to free isn't found in the allocation vectors for any of our active memory types.
        bool freePrivateMemory(const Allocation* memory_to_free);

        std::vector<std::unique_ptr<AllocationCollection>> allocations;
        std::vector<std::unique_ptr<AllocationCollection>> privateAllocations;
        std::vector<bool> emptyAllocations;

        const Device* parent;

        VkPhysicalDeviceProperties deviceProperties;
        VkPhysicalDeviceMemoryProperties deviceMemoryProperties;

        VkDeviceSize preferredLargeHeapBlockSize;
        VkDeviceSize preferredSmallHeapBlockSize;
        const VkAllocationCallbacks* pAllocationCallbacks = nullptr;
        
    };

    

}

#endif // !VULPES_VK_ALLOCATOR_H
