#include "vpr_stdafx.h"
#include "alloc/MemoryBlock.hpp"
#include "alloc/Allocation.hpp"
#include "alloc/Allocator.hpp"
#include "util/easylogging++.h"


namespace vpr {

    constexpr static VkDeviceSize DEBUG_PADDING = 0;

    VkBool32 AllocationRequirements::noNewAllocations = false;
    
    /** This is a simple and common overload to print enum info to any stream (this also works, FYI, with easylogging++). A note to make, however,
    *  is that compilers running at W4/Wall warning levels will warn that this method is unreferenced in release mode, if validation is not forced.
    *  As the memory verification routine is not used in this case, much of that code will fold away but should still be left in for debug builds.
    * \ingroup Allocation
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
        Suballocations.emplace_back(Suballocation{ 0, new_size, SuballocationType::Free });

        // add location of that suballocation to mapping vector
        auto suballoc_iter = Suballocations.end();
        --suballoc_iter;
        availSuballocations.emplace_back(suballoc_iter);

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
            if (curr.Offset != calculated_offset) {
                return ValidationCode::INCORRECT_SUBALLOC_OFFSET;
            }

            // two adjacent free entries is invalid: should be merged.
            const bool curr_free = (curr.Type == SuballocationType::Free);
            if (curr_free && prev_entry_free) {
                return ValidationCode::NEED_MERGE_SUBALLOCS;
            }

            // update prev free status
            prev_entry_free = curr_free;

            if (curr_free) {
                calculated_free_size += curr.Size;
                ++calculated_free_suballocs;
                if (curr.Size >= MinSuballocationSizeToRegister) {
                    ++num_suballocs_to_register; // this suballocation should be registered in the free list.
                }
            }

            calculated_offset += curr.Size;
        }

        // Number of free suballocations in objects list doesn't match what our calculated value says we should have
        if (availSuballocations.size() != num_suballocs_to_register) {
            return ValidationCode::FREE_SUBALLOC_COUNT_MISMATCH;
        }

        VkDeviceSize last_entry_size = 0;
        for (size_t i = 0; i < availSuballocations.size(); ++i) {
            auto curr_iter = availSuballocations[i];

            // non-free suballoc in free list
            if (curr_iter->Type != SuballocationType::Free) {
                return ValidationCode::USED_SUBALLOC_IN_FREE_LIST;
            }

            // sorting of free list is incorrect
            if (curr_iter->Size < last_entry_size) {
                return ValidationCode::FREE_SUBALLOC_SORT_INCORRECT;
            }

            last_entry_size = curr_iter->Size;
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
            if ((*iter)->Size > allocation_size) {
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
                dest_request->FreeSuballocation = iter;
                dest_request->Offset = offset;
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
        assert(suballoc.Type == SuballocationType::Free);

        if (suballoc.Size < allocation_size) {
            return false;
        }

        *dest_offset = suballoc.Offset;

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
                bool on_same_page = CheckBlocksOnSamePage(prev_suballoc.Offset, prev_suballoc.Size, *dest_offset, buffer_image_granularity);
                if (on_same_page) {
                    conflict_found = CheckBufferImageGranularityConflict(prev_suballoc.Type, allocation_type);
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
        const VkDeviceSize padding_begin = *dest_offset - suballoc.Offset;

        // calculate required padding at end, assuming current suballoc isn't at end of memory object
        auto next_iter = dest_suballocation_location;
        ++next_iter;
        const VkDeviceSize padding_end = (next_iter != Suballocations.cend()) ? DEBUG_PADDING : 0;

        // Can't allocate if padding at begin and end is greater than requested size.
        if (padding_begin + padding_end + allocation_size > suballoc.Size) {
            LOG(INFO) << "Suballocation verification failed as required padding for alignment + required size is greater than available space.";
            return false;
        }

        // We checked previous allocations for conflicts: now, we'll check next suballocations
        if(buffer_image_granularity > 1) {
            auto next_suballoc_iter = dest_suballocation_location;
            ++next_suballoc_iter;
            while (next_suballoc_iter != Suballocations.cend()) {
                const auto& next_suballoc = *next_suballoc_iter;
                bool on_same_page = CheckBlocksOnSamePage(*dest_offset, allocation_size, next_suballoc.Offset, buffer_image_granularity);
                if (on_same_page) {
                    if (CheckBufferImageGranularityConflict(allocation_type, next_suballoc.Type)) {
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

        assert(request.FreeSuballocation != Suballocations.cend());
        Suballocation& suballoc = *request.FreeSuballocation;
        assert(suballoc.Type == SuballocationType::Free); 

        const VkDeviceSize padding_begin = request.Offset - suballoc.Offset;
        const VkDeviceSize padding_end = suballoc.Size - padding_begin - allocation_size;

        removeFreeSuballocation(request.FreeSuballocation);
        suballoc.Offset = request.Offset;
        suballoc.Size = allocation_size;
        suballoc.Type = allocation_type;

        // if there's any remaining memory after this allocation, register it

        if (padding_end) {
            Suballocation padding_suballoc{ request.Offset + allocation_size, padding_end, SuballocationType::Free };
            auto next_iter = request.FreeSuballocation;
            ++next_iter;
            {
                std::lock_guard<std::mutex> alloc_guard(guardMutex);
                const auto insert_iter = Suballocations.insert(next_iter, padding_suballoc);
                // insert_iter returns iterator giving location of inserted item
                insertFreeSuballocation(insert_iter);
            }
        }

        // if there's any remaining memory before the allocation, register it.
        if (padding_begin) {
            Suballocation padding_suballoc{ request.Offset - padding_begin, padding_begin, SuballocationType::Free };
            auto next_iter = request.FreeSuballocation;
            {
                std::lock_guard<std::mutex> alloc_guard(guardMutex);
                const auto insert_iter = Suballocations.insert(next_iter, padding_suballoc);
                insertFreeSuballocation(insert_iter);
            }
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
        for (auto iter = Suballocations.begin(); iter != Suballocations.end(); ++iter) {        
            auto& suballoc = *iter;
            if (suballoc.Offset == memory_to_free->Offset()) {
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
        std::lock_guard<std::mutex> mapping_lock(guardMutex);
        VkResult result = vkMapMemory(allocator->DeviceHandle(), memory, alloc_being_mapped->Offset() + offset_to_map_at, size_of_map, 0, &destination_address);
        VkAssert(result);
    }

    void MemoryBlock::Unmap() {
        std::lock_guard<std::mutex> unmapping_lock(guardMutex);
        vkUnmapMemory(allocator->DeviceHandle(), memory);
    }

    VkDeviceSize MemoryBlock::LargestAvailRegion() const noexcept {
        return (*availSuballocations.front()).Size;
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
        assert(next_iter->Type == SuballocationType::Free);
        // add item to merge's size to the size of the object after it
        item_to_merge->Size += next_iter->Size;
        --freeCount;
        std::lock_guard<std::mutex> alloc_guard(guardMutex);
        Suballocations.erase(next_iter);
    }

    void MemoryBlock::freeSuballocation(const suballocationList::iterator & item_to_free) {
        Suballocation& suballoc = *item_to_free;
        suballoc.Type = SuballocationType::Free;

        ++freeCount;
        availSize += suballoc.Size;

        bool merge_next = false, merge_prev = false;

        auto next_iter = item_to_free;
        ++next_iter;
        if ((next_iter != Suballocations.cend()) && (next_iter->Type == SuballocationType::Free)) {
            merge_next = true;
        }

        auto prev_iter = item_to_free;
        
        if (prev_iter != Suballocations.cbegin()) {
            --prev_iter;
            if (prev_iter->Type == SuballocationType::Free) {
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
        std::lock_guard<std::mutex> alloc_guard(guardMutex);
        if (item_to_insert->Size >= MinSuballocationSizeToRegister) {
            if (availSuballocations.empty()) {
                availSuballocations.emplace_back(item_to_insert);
            }
            else {
                // find correct position ot insert "item_to_insert" and do so.
                auto insert_iter = std::lower_bound(availSuballocations.begin(), availSuballocations.end(), item_to_insert, suballocIterCompare());
                availSuballocations.insert(insert_iter, item_to_insert);
            }
        }

    }

    void MemoryBlock::removeFreeSuballocation(const suballocationList::iterator & item_to_remove) {
        std::lock_guard<std::mutex> alloc_guard(guardMutex);
        if (item_to_remove->Size >= MinSuballocationSizeToRegister) {
            auto remove_iter = std::remove(availSuballocations.begin(), availSuballocations.end(), item_to_remove);
            assert(remove_iter != availSuballocations.cend());
            availSuballocations.erase(remove_iter);
        }

    }

}