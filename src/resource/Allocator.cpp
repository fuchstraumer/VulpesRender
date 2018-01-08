#include "vpr_stdafx.h"
#include "resource/Allocator.hpp"
#include "core/Instance.hpp"
#include "core/LogicalDevice.hpp"
#include "core/PhysicalDevice.hpp"
#include "util/easylogging++.h"
#include <mutex>

namespace vpr {

    static std::mutex memoryMutex;

    VkBool32 AllocationRequirements::noNewAllocations = false;

    // padding to inject at end of allocations ot test allocation system
    static constexpr size_t DEBUG_PADDING = 0;



    /*
    This sets the calling/usage of the memory validation routine.
    This can be costly, but it also returns the error present
    and could be used  (in some cases) to fix the error instead
    of reporting a likely critical failure
    */

#if !defined NDEBUG || defined FORCE_ALLOCATOR_VALIDATION
    constexpr bool VALIDATE_MEMORY = true;
#else 
    constexpr bool VALIDATE_MEMORY = false;
#endif // !NDEBUG

    constexpr static size_t vkMaxMemoryTypes = 32;
    // mininum size of suballoction objects to bother registering in our allocation's list's
    constexpr static VkDeviceSize MinSuballocationSizeToRegister = 16;

    constexpr static VkDeviceSize SmallHeapMaxSize = 512 * 1024 * 1024;
    constexpr static VkDeviceSize DefaultLargeHeapBlockSize = 256 * 1024 * 1024;
    constexpr static VkDeviceSize DefaultSmallHeapBlockSize = 64 * 1024 * 1024;

    /*

    Utility functions for performing various allocation tasks.

    */

    template<typename T>
    constexpr static T AlignUp(const T& offset, const T& alignment) {
        return (offset + alignment - 1) / alignment * alignment;
    }

    /**
    *    Taken from the Vulkan specification, section 11.6
    *    Essentially, we need to ensure that linear and non-linear resources are properly placed on separate memory pages so that
    *    they avoid any accidental aliasing. Linear resources are just those that could be read like any other memory region, without
    *   any particular optimization for size or access speed. Optimally tiled resources are those that are tiled either by the hardware drivers,
    *   or the Vulkan implementation. Think of things like Z-Order curve encoding for texture data, or block-based compression for DDS/KTX texture formats.
    *   \param item_a_offset: non-linear object's offset
    *   \param item_a_size: non-linear object's size
    *   \param item_b_offset: linear object's offset
    *   \param item_b_size: linear object's size
    *   \param page_size: almost universally tends to be the bufferImageGranularity value retrieved by the parent Allocator class.
    *   \ingroup Allocator
    */
    constexpr static inline bool CheckBlocksOnSamePage(const VkDeviceSize& item_a_offset, const VkDeviceSize& item_a_size, const VkDeviceSize& item_b_offset, const VkDeviceSize& page_size) {
        assert(item_a_offset + item_a_size <= item_b_offset && item_a_size > 0 && page_size > 0);
        VkDeviceSize item_a_end = item_a_offset + item_a_size - 1;
        VkDeviceSize item_a_end_page = item_a_end & ~(page_size - 1);
        VkDeviceSize item_b_start_Page = item_b_offset & ~(page_size - 1);
        return item_a_end_page == item_b_start_Page;
    }

    /**
    *    Checks to make sure the two objects of type "type_a" and "type_b" wouldn't cause a conflict with the buffer-image granularity values. Returns true if
    *    conflict, false if no conflict. This is unlike the CheckBlocksOnSamePage method, in that it doesn't check memory location and alignment values, merely
    *   comparing the resource types for incompatabilities. This is used to avoid the more detailed checks like CheckBlocksOnSamePage (and the corrections required
    *   if this also fails)
    *
    *    BufferImageGranularity specifies interactions between linear and non-linear resources, so we check based on those.
    *   \ingroup Allocator
    */
    constexpr static inline bool CheckBufferImageGranularityConflict(SuballocationType type_a, SuballocationType type_b) {
        if (type_a > type_b) {
            std::swap(type_a, type_b);
        }

        switch (type_a) {
        case SuballocationType::Free:
            return false;
        case SuballocationType::Unknown:
            // best be conservative and play it safe: return true
            return true;
        case SuballocationType::Buffer:
            // unknown return is playing it safe again, optimal return is because optimal tiling and linear buffers don't mix
            return type_b == SuballocationType::ImageUnknown || type_b == SuballocationType::ImageOptimal;
        case SuballocationType::ImageUnknown:
            return type_b == SuballocationType::ImageUnknown || type_b == SuballocationType::ImageOptimal || type_b == SuballocationType::ImageLinear;
        case SuballocationType::ImageLinear:
            return type_b == SuballocationType::ImageOptimal;
        case SuballocationType::ImageOptimal:
            return false;
        default:
            LOG(WARNING) << "Reached default case in CheckBufferImageGranularity switch statement: this should NOT occur";
            return true;
        }
    }

    constexpr static inline uint32_t countBitsSet(const uint32_t& val) {
        uint32_t count = val - ((val >> 1) & 0x55555555);
        count = ((count >> 2) & 0x33333333) + (count & 0x33333333);
        count = ((count >> 4) + count) & 0x0F0F0F0F;
        count = ((count >> 8) + count) & 0x00FF00FF;
        count = ((count >> 16) + count) & 0x0000FFFF;
        return count;
    }

    /** This is a simple and common overload to print enum info to any stream (this also works, FYI, with easylogging++). A note to make, however,
    *  is that compilers running at W4/Wall warning levels will warn that this method is unreferenced in release mode, if validation is not forced.
    *  As the memory verification routine is not used in this case, much of that code will fold away but should still be left in for debug builds.
    * \ingroup Allocator
    */
    std::ostream& operator<<(std::ostream& os, const ValidationCode& code)  {
        switch (code) {
        case ValidationCode::NULL_MEMORY_HANDLE:
            os << "Null memory handle.";
            break;
        case ValidationCode::ZERO_MEMORY_SIZE:
            os << "Zero memory size.";
            break;
        case ValidationCode::INCORRECT_SUBALLOC_OFFSET:
            os << "Incorrect suballocation offset.";
            break;
        case ValidationCode::NEED_MERGE_SUBALLOCS:
            os << "Adjacent free suballocations not merged.";
            break;
        case ValidationCode::FREE_SUBALLOC_COUNT_MISMATCH:
            os << "Mismatch between counted and caculated quantity of free suballocations.";
            break;
        case ValidationCode::USED_SUBALLOC_IN_FREE_LIST:
            os << "Used suballocation in free/available suballocation list.";
            break;
        case ValidationCode::FREE_SUBALLOC_SORT_INCORRECT:
            os << "Sorting of available suballocations not correct.";
            break;
        case ValidationCode::FINAL_SIZE_MISMATCH:
            os << "Declared total size of allocation doesn't match calculated total size.";
            break;
        case ValidationCode::FINAL_FREE_SIZE_MISMATCH:
            os << "Declared total free size doesn't match caculated total free size.";
            break;
        default:
            break;
        }
        return os;
    }

    MemoryBlock::MemoryBlock(Allocator * alloc) : allocator(alloc), availSize(0), freeCount(0), memory(VK_NULL_HANDLE), Size(0) {}

    MemoryBlock::~MemoryBlock() {
        if (memory != VK_NULL_HANDLE) {
            LOG(WARNING) << "Memory block destructor called before memory block's memory was destroyed.";
            Destroy(allocator);
        }
    }

    void MemoryBlock::Init(VkDeviceMemory & new_memory, const VkDeviceSize & new_size) {
        assert(memory == VK_NULL_HANDLE);
        memory = new_memory;
        Size = new_size;
        freeCount = 1;
        availSize = new_size;
        Suballocations.clear();
        availSuballocations.clear();

        // note/create suballocation defining our singular free region.
        Suballocation suballoc{ 0, new_size, SuballocationType::Free };
        Suballocations.push_back(suballoc);

        // add location of that suballocation to mapping vector
        auto suballoc_iter = Suballocations.end();
        --suballoc_iter;
        availSuballocations.push_back(suballoc_iter);

        LOG(INFO) << "Created new MemoryBlock with size " << std::to_string(Size * 1e-6) << "mb ";
    }

    void MemoryBlock::Destroy(Allocator * alloc) {

        if (!Suballocations.empty()) {
            Suballocations.clear();
        }

        if (!availSuballocations.empty()) {
            availSuballocations.clear();
        }

        LOG(INFO) << "MemoryBlock memory freed, memory handle was: " << memory << " and size was " << std::to_string(Size * 1e-6) << "mb";
        vkFreeMemory(alloc->DeviceHandle(), memory, nullptr);
        memory = VK_NULL_HANDLE;

    }

    bool MemoryBlock::operator<(const MemoryBlock & other) {
        return (availSize < other.availSize);
    }

    VkDeviceSize MemoryBlock::AvailableMemory() const noexcept {
        return availSize;
    }

    const VkDeviceMemory& MemoryBlock::Memory() const noexcept {
        return memory;
    }

    ValidationCode MemoryBlock::Validate() const {

        if (memory == VK_NULL_HANDLE) {
            return ValidationCode::NULL_MEMORY_HANDLE;
        }

        if (Size == 0 || Suballocations.empty()) {
            return ValidationCode::ZERO_MEMORY_SIZE;
        }

        // expected offset of newest suballocation compared to previous
        VkDeviceSize calculated_offset = 0;
        // expected number of free suballocations, from traversing that list
        uint32_t calculated_free_suballocs = 0;
        // available size, taken as sum of avail size of all free suballocations.
        VkDeviceSize calculated_free_size = 0;
        // number of free suballocations that need to be registered in the list for these objects
        uint32_t num_suballocs_to_register = 0;
        // set as we iterate, true when prev suballoc was free (used to sort/swap entries)
        bool prev_entry_free = false;

        for (auto iter = Suballocations.cbegin(); iter != Suballocations.cend(); ++iter) {
            const Suballocation& curr = *iter;

            // if offset of current suballoc is wrong, then this allocation is invalid
            if (curr.offset != calculated_offset) {
                return ValidationCode::INCORRECT_SUBALLOC_OFFSET;
            }

            // two adjacent free entries is invalid: should be merged.
            const bool curr_free = (curr.type == SuballocationType::Free);
            if (curr_free && prev_entry_free) {
                return ValidationCode::NEED_MERGE_SUBALLOCS;
            }

            // update prev free status
            prev_entry_free = curr_free;

            if (curr_free) {
                calculated_free_size += curr.size;
                ++calculated_free_suballocs;
                if (curr.size >= MinSuballocationSizeToRegister) {
                    ++num_suballocs_to_register; // this suballocation should be registered in the free list.
                }
            }

            calculated_offset += curr.size;
        }

        // Number of free suballocations in objects list doesn't match what our calculated value says we should have
        if (availSuballocations.size() != num_suballocs_to_register) {
            return ValidationCode::FREE_SUBALLOC_COUNT_MISMATCH;
        }

        VkDeviceSize last_entry_size = 0;
        for (size_t i = 0; i < availSuballocations.size(); ++i) {
            auto curr_iter = availSuballocations[i];

            // non-free suballoc in free list
            if (curr_iter->type != SuballocationType::Free) {
                return ValidationCode::USED_SUBALLOC_IN_FREE_LIST;
            }

            // sorting of free list is incorrect
            if (curr_iter->size < last_entry_size) {
                return ValidationCode::FREE_SUBALLOC_SORT_INCORRECT;
            }

            last_entry_size = curr_iter->size;
        }

        if (calculated_offset != Size) {
            return ValidationCode::FINAL_SIZE_MISMATCH;
        }

        if (calculated_free_size != availSize) {
            return ValidationCode::FINAL_FREE_SIZE_MISMATCH;
        }

        if (calculated_free_suballocs != freeCount) {
            return ValidationCode::FREE_SUBALLOC_COUNT_MISMATCH;
        }

        return ValidationCode::VALIDATION_PASSED;
    }

    bool MemoryBlock::RequestSuballocation(const VkDeviceSize & buffer_image_granularity, const VkDeviceSize & allocation_size, const VkDeviceSize & allocation_alignment, SuballocationType allocation_type, SuballocationRequest * dest_request) {
        if ((availSize < allocation_size) || availSuballocations.empty()) {
            // not enough space in this allocation object
            return false;
        }

        const size_t numFreeSuballocs = availSuballocations.size();

        // use lower_bound to find location of avail suballocation

        size_t avail_idx = std::numeric_limits<size_t>::max();
        for (auto iter = availSuballocations.cbegin(); iter != availSuballocations.cend(); ++iter) {
            if ((*iter)->size > allocation_size) {
                avail_idx = iter - availSuballocations.cbegin();
                break;
            }
        }

        if (avail_idx == std::numeric_limits<size_t>::max()) {
            return false;
        }

        for (size_t idx = avail_idx; idx < numFreeSuballocs; ++idx) {
            VkDeviceSize offset = 0;
            const auto iter = availSuballocations[idx];
            // Check allocation for validity
            bool allocation_valid = VerifySuballocation(buffer_image_granularity, allocation_size, allocation_alignment, allocation_type, iter, &offset);
            if (allocation_valid) {
                dest_request->freeSuballocation = iter;
                dest_request->offset = offset;
                return true;
            }
        }

        return false;
    }

    bool MemoryBlock::VerifySuballocation(const VkDeviceSize & buffer_image_granularity, const VkDeviceSize & allocation_size, const VkDeviceSize & allocation_alignment, SuballocationType allocation_type, const suballocationList::const_iterator & dest_suballocation_location, VkDeviceSize * dest_offset) const {
        assert(allocation_size > 0);
        assert(allocation_type != SuballocationType::Free);
        assert(dest_suballocation_location != Suballocations.cend());
        assert(dest_offset != nullptr);

        const Suballocation& suballoc = *dest_suballocation_location;
        assert(suballoc.type == SuballocationType::Free);

        if (suballoc.size < allocation_size) {
            return false;
        }

        *dest_offset = suballoc.offset;

        // Apply alignment
        *dest_offset = AlignUp<VkDeviceSize>(*dest_offset, allocation_alignment);

        // check previous suballocations for conflicts with buffer-image granularity, change alignment as needed
        if (buffer_image_granularity > 1) {
            bool conflict_found = false;
            // iterate backwards, since we're checking previous suballoations for alignment conflicts
            auto prev_suballoc_iter = dest_suballocation_location;
            while (prev_suballoc_iter != Suballocations.cbegin()) {
                --prev_suballoc_iter;
                const Suballocation& prev_suballoc = *prev_suballoc_iter;
                bool on_same_page = CheckBlocksOnSamePage(prev_suballoc.offset, prev_suballoc.size, *dest_offset, buffer_image_granularity);
                if (on_same_page) {
                    conflict_found = CheckBufferImageGranularityConflict(prev_suballoc.type, allocation_type);
                    if (conflict_found) {
                        LOG(INFO) << "A buffer-image granularity conflict was identified in suballocation " << std::to_string(reinterpret_cast<size_t>(&prev_suballoc));
                        break;
                    }
                }
                else {
                    break;
                }
            }
            if (conflict_found) {
                // align up by a page size to get off the current page and remove the conflict.
                *dest_offset = AlignUp<VkDeviceSize>(*dest_offset, buffer_image_granularity);
            }
        }

        // calculate padding at beginning from offset
        const VkDeviceSize padding_begin = *dest_offset - suballoc.offset;

        // calculate required padding at end, assuming current suballoc isn't at end of memory object
        auto next_iter = dest_suballocation_location;
        ++next_iter;
        const VkDeviceSize padding_end = (next_iter != Suballocations.cend()) ? DEBUG_PADDING : 0;

        // Can't allocate if padding at begin and end is greater than requested size.
        if (padding_begin + padding_end + allocation_size > suballoc.size) {
            LOG(INFO) << "Suballocation verification failed as required padding for alignment + required size is greater than available space.";
            return false;
        }

        // We checked previous allocations for conflicts: now, we'll check next suballocations
        if(buffer_image_granularity > 1) {
            auto next_suballoc_iter = dest_suballocation_location;
            ++next_suballoc_iter;
            while (next_suballoc_iter != Suballocations.cend()) {
                const auto& next_suballoc = *next_iter;
                bool on_same_page = CheckBlocksOnSamePage(*dest_offset, allocation_size, next_suballoc.offset, buffer_image_granularity);
                if (on_same_page) {
                    if (CheckBufferImageGranularityConflict(allocation_type, next_suballoc.type)) {
                        LOG(INFO) << "Suballocation verification failed as there were too many buffer-image granularity conflicts.";
                        return false;
                    }
                }
                else {
                    break;
                }
                ++next_suballoc_iter;
            }
        }

        return true;
    }

    bool MemoryBlock::Empty() const {
        return (Suballocations.size() == 1) && (freeCount == 1);
    }

    void MemoryBlock::Allocate(const SuballocationRequest & request, const SuballocationType & allocation_type, const VkDeviceSize & allocation_size) {

        std::lock_guard<std::mutex> alloc_guard(memoryMutex);
        assert(request.freeSuballocation != Suballocations.cend());
        Suballocation& suballoc = *request.freeSuballocation;
        assert(suballoc.type == SuballocationType::Free); 

        const VkDeviceSize padding_begin = request.offset - suballoc.offset;
        const VkDeviceSize padding_end = suballoc.size - padding_begin - allocation_size;

        removeFreeSuballocation(request.freeSuballocation);
        suballoc.offset = request.offset;
        suballoc.size = allocation_size;
        suballoc.type = allocation_type;

        // if there's any remaining memory after this allocation, register it

        if (padding_end) {
            Suballocation padding_suballoc{ request.offset + allocation_size, padding_end, SuballocationType::Free };
            auto next_iter = request.freeSuballocation;
            ++next_iter;
            const auto insert_iter = Suballocations.insert(next_iter, padding_suballoc);
            // insert_iter returns iterator giving location of inserted item
            insertFreeSuballocation(insert_iter);
        }

        // if there's any remaining memory before the allocation, register it.
        if (padding_begin) {
            Suballocation padding_suballoc{ request.offset - padding_begin, padding_begin, SuballocationType::Free };
            auto next_iter = request.freeSuballocation;
            const auto insert_iter = Suballocations.insert(next_iter, padding_suballoc);
            insertFreeSuballocation(insert_iter);
        }

        --freeCount;

        if (padding_begin > 0) {
            ++freeCount;
        }

        if (padding_end > 0) {
            ++freeCount;
        }

        availSize -= allocation_size;

    }

    void MemoryBlock::Free(const Allocation* memory_to_free) {
        std::lock_guard<std::mutex> suballoc_guard(memoryMutex);
        for (auto iter = Suballocations.begin(); iter != Suballocations.end(); ++iter) {
            auto& suballoc = *iter;
            if (suballoc.offset == memory_to_free->Offset()) {
                freeSuballocation(iter);
                if (VALIDATE_MEMORY) {
                    auto vcode = Validate();
                    if (vcode != ValidationCode::VALIDATION_PASSED) {
                        LOG(ERROR) << "Validation of memory failed: " << vcode;
                        throw std::runtime_error("Validation of memory failed");
                    }
                }
                return;
            }
        }
    }
    
    void MemoryBlock::Map(const Allocation* alloc_being_mapped, const VkDeviceSize& size_of_map, const VkDeviceSize& offset_to_map_at, void* destination_address) {
        std::lock_guard<std::mutex> mapping_lock(memoryMutex);
        VkResult result = vkMapMemory(allocator->DeviceHandle(), memory, alloc_being_mapped->Offset() + offset_to_map_at, size_of_map, 0, &destination_address);
        VkAssert(result);
    }

    void MemoryBlock::Unmap() {
        std::lock_guard<std::mutex> unmapping_lock(memoryMutex);
        vkUnmapMemory(allocator->DeviceHandle(), memory);
    }

    VkDeviceSize MemoryBlock::LargestAvailRegion() const noexcept {
        return (*availSuballocations.front()).size;
    }

    suballocation_iterator_t MemoryBlock::begin() {
        return Suballocations.begin();
    }

    suballocation_iterator_t MemoryBlock::end() {
        return Suballocations.end();
    }

    const_suballocation_iterator_t MemoryBlock::begin() const {
        return Suballocations.begin();
    }

    const_suballocation_iterator_t MemoryBlock::end() const {
        return Suballocations.end();
    }

    const_suballocation_iterator_t MemoryBlock::cbegin() const {
        return Suballocations.cbegin();
    }

    const_suballocation_iterator_t MemoryBlock::cend() const {
        return Suballocations.cend();
    }

    avail_suballocation_iterator_t MemoryBlock::avail_begin() {
        return availSuballocations.begin();
    }

    avail_suballocation_iterator_t MemoryBlock::avail_end() {
        return availSuballocations.end();
    }

    const_avail_suballocation_iterator_t MemoryBlock::avail_begin() const {
        return availSuballocations.begin();
    }

    const_avail_suballocation_iterator_t MemoryBlock::avail_end() const {
        return availSuballocations.end();;
    }

    const_avail_suballocation_iterator_t MemoryBlock::avail_cbegin() const {
        return availSuballocations.cbegin();
    }

    const_avail_suballocation_iterator_t MemoryBlock::avail_cend() const {
        return availSuballocations.cend();;
    }

    void MemoryBlock::mergeFreeWithNext(const suballocationList::iterator & item_to_merge) {
        auto next_iter = item_to_merge;
        ++next_iter;
        assert(next_iter != Suballocations.cend());
        assert(next_iter->type == SuballocationType::Free);
        // add item to merge's size to the size of the object after it
        item_to_merge->size += next_iter->size;
        --freeCount;
        Suballocations.erase(next_iter);
    }

    void MemoryBlock::freeSuballocation(const suballocationList::iterator & item_to_free) {
        Suballocation& suballoc = *item_to_free;
        suballoc.type = SuballocationType::Free;

        ++freeCount;
        availSize += suballoc.size;

        bool merge_next = false, merge_prev = false;

        auto next_iter = item_to_free;
        ++next_iter;
        if ((next_iter != Suballocations.cend()) && (next_iter->type == SuballocationType::Free)) {
            merge_next = true;
        }

        auto prev_iter = item_to_free;
        
        if (prev_iter != Suballocations.cbegin()) {
            --prev_iter;
            if (prev_iter->type == SuballocationType::Free) {
                merge_prev = true;
            }
        }

        if (merge_next) {
            removeFreeSuballocation(next_iter);
            mergeFreeWithNext(item_to_free);
        }

        if (merge_prev) {
            removeFreeSuballocation(prev_iter);
            mergeFreeWithNext(prev_iter);
            insertFreeSuballocation(prev_iter);
        }
        else {
            insertFreeSuballocation(item_to_free);
        }
    }

    void MemoryBlock::insertFreeSuballocation(const suballocationList::iterator & item_to_insert) {
        if (item_to_insert->size >= MinSuballocationSizeToRegister) {
            if (availSuballocations.empty()) {
                availSuballocations.push_back(item_to_insert);
            }
            else {
                // find correct position ot insert "item_to_insert" and do so.
                auto insert_iter = std::lower_bound(availSuballocations.begin(), availSuballocations.end(), item_to_insert, suballocIterCompare());
                availSuballocations.insert(insert_iter, item_to_insert);
            }
        }

    }

    void MemoryBlock::removeFreeSuballocation(const suballocationList::iterator & item_to_remove) {
        if (item_to_remove->size >= MinSuballocationSizeToRegister) {
            auto remove_iter = std::remove(availSuballocations.begin(), availSuballocations.end(), item_to_remove);
            assert(remove_iter != availSuballocations.cend());
            availSuballocations.erase(remove_iter);
        }

    }

    AllocationCollection::AllocationCollection(Allocator * _allocator) : allocator(_allocator) {}

    AllocationCollection::~AllocationCollection() {
        for (size_t i = 0; i < allocations.size(); ++i) {
            if (allocations[i]) {
                allocations[i]->Destroy(allocator);
                allocations[i].reset();
            }
        }

        allocations.clear();
        allocations.shrink_to_fit();

    }

    MemoryBlock* AllocationCollection::operator[](const size_t & idx) {
        return allocations[idx].get();
    }

    const MemoryBlock* AllocationCollection::operator[](const size_t & idx) const {
        return allocations[idx].get();
    }

    bool AllocationCollection::Empty() const {
        return allocations.empty();
    }

    void AllocationCollection::RemoveBlock(MemoryBlock* block_to_erase) {
        for (auto iter = allocations.begin(); iter != allocations.end(); ++iter) {
            if ((*iter).get() == block_to_erase) {
                LOG(INFO) << "Removed memory block " << (*iter).get() << " from allocation collection.";
                allocations.erase(iter);
                return;
            }
        }

        LOG(ERROR) << "Couldn't free memory block " << block_to_erase << " !";
    }

    void AllocationCollection::SortAllocations() {

        // sorts collection so that allocation with most free space is first.
        std::sort(allocations.begin(), allocations.end());

    }

    Allocator::Allocator(const Device * parent_dvc) : parent(parent_dvc), preferredSmallHeapBlockSize(DefaultSmallHeapBlockSize), preferredLargeHeapBlockSize(DefaultLargeHeapBlockSize) {
        deviceProperties = parent->GetPhysicalDevice().Properties;
        deviceMemoryProperties = parent->GetPhysicalDevice().MemoryProperties;
        allocations.resize(GetMemoryTypeCount());
        privateAllocations.resize(GetMemoryTypeCount());
        emptyAllocations.resize(GetMemoryTypeCount());
        // initialize base pools, one per memory type.
        for (size_t i = 0; i < GetMemoryTypeCount(); ++i) {
            allocations[i] = std::make_unique<AllocationCollection>(this);
            privateAllocations[i] = std::make_unique<AllocationCollection>(this);
            emptyAllocations[i] = true;
        }
    }

    Allocator::~Allocator() {

        /*
            Delete collections: destructors of these should take care of the rest.
        */

        allocations.clear();
        privateAllocations.clear();
        emptyAllocations.clear();
        allocations.shrink_to_fit();
        privateAllocations.shrink_to_fit();
        emptyAllocations.shrink_to_fit();

    }

    void Allocator::Recreate() {

        allocations.clear();
        privateAllocations.clear();
        emptyAllocations.clear();
        allocations.shrink_to_fit();
        privateAllocations.shrink_to_fit();
        emptyAllocations.shrink_to_fit();

        allocations.resize(GetMemoryTypeCount());
        privateAllocations.resize(GetMemoryTypeCount());
        emptyAllocations.resize(GetMemoryTypeCount());
        // initialize base pools, one per memory type.
        for (size_t i = 0; i < GetMemoryTypeCount(); ++i) {
            allocations[i] = std::make_unique<AllocationCollection>(this);
            privateAllocations[i] = std::make_unique<AllocationCollection>(this);
            emptyAllocations[i] = true;
        }

    }

    VkDeviceSize Allocator::GetPreferredBlockSize(const uint32_t& memory_type_idx) const noexcept {
        VkDeviceSize heapSize = deviceMemoryProperties.memoryHeaps[deviceMemoryProperties.memoryTypes[memory_type_idx].heapIndex].size;
        return (heapSize <= DefaultSmallHeapBlockSize) ? preferredSmallHeapBlockSize : preferredLargeHeapBlockSize;
    }

    VkDeviceSize Allocator::GetBufferImageGranularity() const noexcept {
        return deviceProperties.limits.bufferImageGranularity;
    }

    uint32_t Allocator::GetMemoryHeapCount() const noexcept {
        return deviceMemoryProperties.memoryHeapCount;
    }

    uint32_t Allocator::GetMemoryTypeCount() const noexcept {
        return deviceMemoryProperties.memoryTypeCount;
    }

    const VkDevice & Allocator::DeviceHandle() const noexcept {
        return parent->vkHandle();
    }

    VkResult Allocator::AllocateMemory(const VkMemoryRequirements& memory_reqs, const AllocationRequirements& alloc_details, const SuballocationType& suballoc_type, Allocation& dest_allocation) {
        
        // find memory type (i.e idx) required for this allocation
        uint32_t memory_type_idx = findMemoryTypeIdx(memory_reqs, alloc_details);
        if (memory_type_idx != std::numeric_limits<uint32_t>::max()) {
            return allocateMemoryType(memory_reqs, alloc_details, memory_type_idx, suballoc_type, dest_allocation);
        }
        else {
            return VK_ERROR_OUT_OF_DEVICE_MEMORY;
        }

    }

    void Allocator::FreeMemory(const Allocation* memory_to_free) {
        uint32_t type_idx = 0;

        if (std::holds_alternative<Allocation::blockAllocation>(memory_to_free->typeData)) {

            type_idx = memory_to_free->MemoryTypeIdx();
            auto& allocation_collection = allocations[type_idx];
            
            auto& block = allocation_collection->allocations.front();
            auto free_size = memory_to_free->Size;
            block->Free(memory_to_free);
            LOG_IF((static_cast<float>(free_size) / 1.0e6f) > 0.5f, INFO) << "Freed a memory allocation with size " << std::to_string(free_size / 1e6) << "mb";
            if (VALIDATE_MEMORY) {
                auto err = block->Validate();
                if (err != ValidationCode::VALIDATION_PASSED) {
                    LOG(ERROR) << "Validation of memory failed: " << err;
                    throw std::runtime_error("Validation of memory failed");
                }
            }

            if (block->Empty()) {
                if (emptyAllocations[type_idx]) {
                    block->Destroy(this);
                    allocation_collection->RemoveBlock(block.get());
                }
                else {
                    emptyAllocations[type_idx] = true;
                }
            }

            // re-sort the collection.
            allocation_collection->SortAllocations();
            
            return;
        }

        


        // memory_to_free not found, possible a privately/singularly allocated memory object
        if (freePrivateMemory(memory_to_free)) {
            return;
        }

    }

    uint32_t Allocator::findMemoryTypeIdx(const VkMemoryRequirements& mem_reqs, const AllocationRequirements & details) const noexcept {
        auto req_flags = details.requiredFlags;
        auto preferred_flags = details.preferredFlags;
        if (req_flags == 0) {
            assert(preferred_flags != VkMemoryPropertyFlagBits(0));
            req_flags = preferred_flags;
        }

        if (preferred_flags == 0) {
            preferred_flags = req_flags;
        }

        uint32_t min_cost = std::numeric_limits<uint32_t>::max();
        uint32_t result_idx = std::numeric_limits<uint32_t>::max();
        // preferred_flags, if not zero, must be a subset of req_flags
        for (uint32_t type_idx = 0, memory_type_bit = 1; type_idx < GetMemoryTypeCount(); ++type_idx, memory_type_bit <<= 1) {
            // memory type of idx is acceptable according to mem_reqs
            if ((memory_type_bit & mem_reqs.memoryTypeBits) != 0) {
                const VkMemoryPropertyFlags& curr_flags = deviceMemoryProperties.memoryTypes[type_idx].propertyFlags;
                // current type contains required flags.
                if ((req_flags & ~curr_flags) == 0) {
                    // calculate the cost of the memory type as the number of bits from preferred_flags
                    // not present in current type at type_idx.
                    uint32_t cost = countBitsSet(preferred_flags & ~req_flags);
                    if (cost < min_cost) {
                        result_idx = type_idx;
                        // ideal memory type, return it.
                        if (cost == 0) {
                            return result_idx;
                        }
                        min_cost = cost;
                    }
                }

            }
        }
        // didn't find zero "cost" idx, but return it.
        // this means that any methods that call this particular type finding method should handle the "exception"
        // of invalid indices themselves.
        return result_idx;
    }

    VkResult Allocator::allocateMemoryType(const VkMemoryRequirements & memory_reqs, const AllocationRequirements & alloc_details, const uint32_t & memory_type_idx, const SuballocationType & type, Allocation& dest_allocation) {
    
        const VkDeviceSize preferredBlockSize = GetPreferredBlockSize(memory_type_idx);

        // If given item is bigger than our preferred block size, we give it its own special allocation (using a single device memory object for this).
        const bool private_memory = alloc_details.privateMemory || memory_reqs.size > preferredBlockSize / 2;

        if (private_memory) {
            if (AllocationRequirements::noNewAllocations) {
                return VK_ERROR_OUT_OF_DEVICE_MEMORY;
            }
            else {
                LOG(INFO) << "Allocating private memory object...";
                return allocatePrivateMemory(memory_reqs.size, type, memory_type_idx, dest_allocation);
            }
        }
        else {
            auto& alloc_collection = allocations[memory_type_idx];

            // first, check existing allocations
            for (auto iter = alloc_collection->allocations.cbegin(); iter != alloc_collection->allocations.cend(); ++iter) {
                SuballocationRequest request;
                const auto& block = *iter;
                if (block->RequestSuballocation(GetBufferImageGranularity(), memory_reqs.size, memory_reqs.alignment, type, &request)) {
                    if (block->Empty()) {
                        emptyAllocations[memory_type_idx] = false;
                    }

                    block->Allocate(request, type, memory_reqs.size);
                    dest_allocation.Init(block.get(), request.offset, memory_reqs.alignment, memory_reqs.size, type);

                    if (VALIDATE_MEMORY) {
                        ValidationCode result_code = block->Validate();
                        if (result_code != ValidationCode::VALIDATION_PASSED) {
                            LOG(ERROR) << "Validation of new allocation failed with reason: " << result_code;
                            throw std::runtime_error("");
                        }
                    }
                    // only log for larger allocations of at least half a mb: otherwise, log gets clogged with updates
                    LOG_IF((static_cast<float>(memory_reqs.size) / 1.0e6f) > 0.5f, INFO) << "Successfully allocated by binding to suballocation with size of " << std::to_string(memory_reqs.size / 1e6) << "mb at offset " << std::to_string(request.offset);
                    return VK_SUCCESS;
                }
            }

            // search didn't pass: create new allocation.
            if (AllocationRequirements::noNewAllocations) {
                LOG(ERROR) << "All available allocations full or invalid, and requested allocation not allowed to be private!";
                return VK_ERROR_OUT_OF_DEVICE_MEMORY;
            }
            else {
                VkMemoryAllocateInfo alloc_info{ VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO, nullptr, preferredBlockSize, memory_type_idx };
                VkDeviceMemory new_memory = VK_NULL_HANDLE;
                VkResult result = vkAllocateMemory(parent->vkHandle(), &alloc_info, nullptr, &new_memory);
                assert(result != VK_ERROR_OUT_OF_DEVICE_MEMORY); // make sure we're not over-allocating and using all device memory.
                if (result != VK_SUCCESS) {
                    // halve allocation size
                    alloc_info.allocationSize /= 2;
                    if (alloc_info.allocationSize >= memory_reqs.size) {
                        result = vkAllocateMemory(parent->vkHandle(), &alloc_info, nullptr, &new_memory);
                        if (result != VK_SUCCESS) {
                            alloc_info.allocationSize /= 2;
                            if (alloc_info.allocationSize >= memory_reqs.size) {
                                result = vkAllocateMemory(parent->vkHandle(), &alloc_info, nullptr, &new_memory);
                            }
                        }
                    }
                }
                // if still not allocated, try allocating private memory (if allowed)
                if (result != VK_SUCCESS && alloc_details.privateMemory) {
                    result = allocatePrivateMemory(memory_reqs.size, type, memory_type_idx, dest_allocation);
                    if (result == VK_SUCCESS) {
                        LOG(INFO) << "Allocation of memory succeeded";
                        return VK_SUCCESS;
                    }
                    else {
                        LOG(WARNING) << "Allocation of memory failed, after multiple attempts.";
                        return result;
                    }
                }

                std::unique_ptr<MemoryBlock> new_block = std::make_unique<MemoryBlock>(this);
                // need pointer to initialize child objects, but need to move unique_ptr into container.
                alloc_collection->allocations.push_back(std::move(new_block));

                auto new_block_ptr = alloc_collection->allocations.back().get();
                // allocation size is more up-to-date than mem reqs size
                new_block_ptr->Init(new_memory, alloc_info.allocationSize);
                new_block_ptr->MemoryTypeIdx = memory_type_idx;

                SuballocationRequest request{ *new_block_ptr->avail_begin(), 0 };
                new_block_ptr->Allocate(request, type, memory_reqs.size);
                
                dest_allocation.Init(new_block_ptr, request.offset, memory_reqs.alignment, memory_reqs.size, type);

                if (VALIDATE_MEMORY) {
                    ValidationCode result_code = new_block_ptr->Validate();
                    if (result_code != ValidationCode::VALIDATION_PASSED) {
                        LOG(ERROR) << "Validation of new allocation failed with reason: " << result_code;
                    }
                }

                LOG(INFO) << "Created new allocation object w/ size of " << std::to_string(alloc_info.allocationSize * 1e-6) << "mb";
                return VK_SUCCESS;
            }
        }

    }

    VkResult Allocator::allocatePrivateMemory(const VkDeviceSize & size, const SuballocationType & type, const uint32_t & memory_type_idx, Allocation& dest_allocation) {
        VkMemoryAllocateInfo alloc_info{ VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO, nullptr, size, memory_type_idx };

        privateSuballocation private_suballoc;
        private_suballoc.size = size;
        private_suballoc.type = type;

        VkResult result = vkAllocateMemory(parent->vkHandle(), &alloc_info, nullptr, &private_suballoc.memory);
        VkAssert(result);
        
        dest_allocation.InitPrivate(memory_type_idx, private_suballoc.memory, type, false, nullptr, size);

        return VK_SUCCESS;
    }

    bool Allocator::freePrivateMemory(const Allocation * alloc_to_free) {
        uint32_t type_idx = alloc_to_free->MemoryTypeIdx();
        auto& private_allocation = privateAllocations[type_idx];
        return false;
    }

    VkResult Allocator::AllocateForImage(VkImage & image_handle, const AllocationRequirements & details, const SuballocationType & alloc_type, Allocation& dest_allocation) {
        std::mutex image_alloc_mutex;
        std::lock_guard<std::mutex> image_alloc_guard(image_alloc_mutex);
        // Get memory info.
        VkMemoryRequirements memreqs;
        vkGetImageMemoryRequirements(parent->vkHandle(), image_handle, &memreqs);
        size_t mem_sz = memreqs.size;
        return AllocateMemory(memreqs, details, alloc_type, dest_allocation);
    }

    VkResult Allocator::AllocateForBuffer(VkBuffer & buffer_handle, const AllocationRequirements & details, const SuballocationType & alloc_type, Allocation& dest_allocation) {
        std::mutex buffer_alloc_mutex;
        std::lock_guard<std::mutex> buffer_alloc_guard(buffer_alloc_mutex);
        VkMemoryRequirements memreqs;
        vkGetBufferMemoryRequirements(parent->vkHandle(), buffer_handle, &memreqs);
        return AllocateMemory(memreqs, details, alloc_type, dest_allocation);
    }

    VkResult Allocator::CreateImage(VkImage * image_handle, const VkImageCreateInfo * img_create_info, const AllocationRequirements & alloc_reqs, Allocation& dest_allocation) {

        // create image object first.
        LOG(INFO) << "Creating new image handle.";
        VkResult result = vkCreateImage(parent->vkHandle(), img_create_info, nullptr, image_handle);
        VkAssert(result);
        LOG(INFO) << "Allocating memory for image.";
        SuballocationType image_type = img_create_info->tiling == VK_IMAGE_TILING_OPTIMAL ? SuballocationType::ImageOptimal : SuballocationType::ImageLinear;
        result = AllocateForImage(*image_handle, alloc_reqs, image_type, dest_allocation);
        VkAssert(result);
        LOG(INFO) << "Binding image to memory.";
        result = vkBindImageMemory(parent->vkHandle(), *image_handle, dest_allocation.Memory(), dest_allocation.Offset());
        VkAssert(result);

        return VK_SUCCESS;

    }

    VkResult Allocator::CreateBuffer(VkBuffer * buffer_handle, const VkBufferCreateInfo * buffer_create_info, const AllocationRequirements & alloc_reqs, Allocation& dest_allocation) {

        // create buffer object first
        VkResult result = vkCreateBuffer(parent->vkHandle(), buffer_create_info, nullptr, buffer_handle);
        VkAssert(result);

        // allocate memory
        result = AllocateForBuffer(*buffer_handle, alloc_reqs, SuballocationType::Buffer, dest_allocation);
        VkAssert(result);

        result = vkBindBufferMemory(parent->vkHandle(), *buffer_handle, dest_allocation.Memory(), dest_allocation.Offset());
        VkAssert(result);

        return VK_SUCCESS;
    }

    void Allocator::DestroyImage(const VkImage & image_handle, Allocation& allocation_to_free) {
        
        if (image_handle == VK_NULL_HANDLE) {
            LOG(ERROR) << "Tried to destroy null image object.";
            throw std::runtime_error("Cannot destroy null image objects.");
        }

        // delete handle.
        vkDestroyImage(parent->vkHandle(), image_handle, nullptr);

        // Free memory previously tied to handle.
        FreeMemory(&allocation_to_free);
    }

    void Allocator::DestroyBuffer(const VkBuffer & buffer_handle, Allocation& allocation_to_free) {
        
        if (buffer_handle == VK_NULL_HANDLE) {
            LOG(ERROR) << "Tried to destroy null buffer object.";
            throw std::runtime_error("Cannot destroy null buffer objects.");
        }

        vkDestroyBuffer(parent->vkHandle(), buffer_handle, nullptr);

        FreeMemory(&allocation_to_free);

    }

    Allocation::Allocation(Allocation && other) noexcept : SuballocType(std::move(other.SuballocType)), Size(std::move(other.Size)), Alignment(std::move(other.Alignment)), 
        typeData(std::move(other.typeData)){}

    Allocation & Allocation::operator=(Allocation && other) noexcept {
        SuballocType = std::move(other.SuballocType);
        Size = std::move(other.Size);
        Alignment = std::move(other.Alignment);
        typeData = std::move(other.typeData);
        return *this;
    }

    void Allocation::Init(MemoryBlock * parent_block, const VkDeviceSize & offset, const VkDeviceSize & alignment, const VkDeviceSize & alloc_size, const SuballocationType & suballoc_type) {
        blockAllocation alloc;
        alloc.ParentBlock = parent_block;
        alloc.Offset = offset;
        Size = alloc_size;
        Alignment = alignment;
        typeData = alloc;
    }

    void Allocation::Update(MemoryBlock * new_parent_block, const VkDeviceSize & new_offset) {
        auto& alloc = std::get<blockAllocation>(typeData);
        alloc.ParentBlock = new_parent_block;
        alloc.Offset = new_offset;
    }

    void Allocation::InitPrivate(const uint32_t & type_idx, VkDeviceMemory & dvc_memory, const SuballocationType & suballoc_type, bool persistently_mapped, void * mapped_data, const VkDeviceSize & data_size) {
        Size = data_size;
        SuballocType = suballoc_type;
        privateAllocation p_alloc;
        p_alloc.DvcMemory = dvc_memory;
        p_alloc.MemoryTypeIdx = type_idx;
        p_alloc.PersistentlyMapped = persistently_mapped;
        p_alloc.MappedData = mapped_data;
        typeData = p_alloc;
    }
    
    void Allocation::Map(const VkDeviceSize& size_to_map, const VkDeviceSize& offset_to_map_at, void* address_to_map_to) const {
        if (std::holds_alternative<blockAllocation>(typeData)) {
            std::get<blockAllocation>(typeData).ParentBlock->Map(this, size_to_map, offset_to_map_at, address_to_map_to);
        }
        else if (std::holds_alternative<privateAllocation>(typeData)) {
            LOG(INFO) << "Attempted to map private allocation, setting given address_to_map_to to permanently mapped address.";
            address_to_map_to = std::get<privateAllocation>(typeData).MappedData;
        }
    }

    void Allocation::Unmap() const noexcept {
        if (std::holds_alternative<blockAllocation>(typeData)) {
            std::get<blockAllocation>(typeData).ParentBlock->Unmap();
        }
    }

    const VkDeviceMemory & Allocation::Memory() const {
        if (std::holds_alternative<blockAllocation>(typeData)) {
            return std::get<blockAllocation>(typeData).ParentBlock->Memory();
        }
        else if (std::holds_alternative<privateAllocation>(typeData)) {
            return std::get<privateAllocation>(typeData).DvcMemory;
        }
        else {
            throw std::runtime_error("Allocation had invalid type or was improperly initialized!");
        }
    }

    VkDeviceSize Allocation::Offset() const noexcept {
        if (std::holds_alternative<blockAllocation>(typeData)) {
            return std::get<blockAllocation>(typeData).Offset;
        }
        else {
            return VkDeviceSize(0);
        }
    }

    uint32_t Allocation::MemoryTypeIdx() const {
        if (std::holds_alternative<blockAllocation>(typeData)) {
            return std::get<blockAllocation>(typeData).ParentBlock->MemoryTypeIdx;
        }
        else if (std::holds_alternative<privateAllocation>(typeData)) {
            return std::get<privateAllocation>(typeData).MemoryTypeIdx;
        }
        else {
            throw std::runtime_error("Allocation had invalid type, or was improperly initialized!");
        }
    }

}
