#include "vpr_stdafx.h"
#include "alloc/AllocationCollection.hpp"
#include "alloc/Allocator.hpp"
#include "alloc/MemoryBlock.hpp"
#include "easylogging++.h"

namespace vpr {

    AllocationCollection::AllocationCollection(Allocator * _allocator) : allocator(_allocator) {}

    AllocationCollection::AllocationCollection(AllocationCollection && other) noexcept : allocations(std::move(other.allocations)), allocator(std::move(other.allocator)) {}

    AllocationCollection & AllocationCollection::operator=(AllocationCollection && other) noexcept {
        allocations = std::move(other.allocations);
        allocator = std::move(other.allocator);
        return *this;
    }

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

    size_t AllocationCollection::AddMemoryBlock(std::unique_ptr<MemoryBlock>&& new_block) noexcept {
        std::lock_guard<std::mutex> push_guard(containerMutex);
        allocations.emplace_back(std::move(new_block));
        return allocations.size() - 1;
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

        std::lock_guard<std::mutex> remove_guard(containerMutex);

        for (auto iter = allocations.begin(); iter != allocations.end(); ++iter) {
            if ((*iter).get() == block_to_erase) {
                LOG_IF(VERBOSE_LOGGING, INFO) << "Removed memory block " << (*iter).get() << " from allocation collection.";
                allocations.erase(iter);
                return;
            }
        }

        LOG(ERROR) << "Couldn't free memory block " << block_to_erase << " !";
        throw std::runtime_error("Failed to free a memory block.");
    }

    void AllocationCollection::SortAllocations() {
        std::lock_guard<std::mutex> sort_guard(containerMutex);
        // sorts collection so that allocation with most free space is first.
        std::sort(allocations.begin(), allocations.end());
    }

    AllocationCollection::iterator AllocationCollection::begin() noexcept {
        return allocations.begin();
    }

    AllocationCollection::iterator AllocationCollection::end() noexcept {
        return allocations.end();
    }

    AllocationCollection::const_iterator AllocationCollection::begin() const noexcept {
        return allocations.begin();
    }

    AllocationCollection::const_iterator AllocationCollection::end() const noexcept {
        return allocations.end();
    }

    AllocationCollection::const_iterator AllocationCollection::cbegin() const noexcept {
        return allocations.cbegin();
    }

    AllocationCollection::const_iterator AllocationCollection::cend() const noexcept {
        return allocations.cend();
    }

}
