#include "vpr_stdafx.h"
#include "alloc/AllocationCollection.hpp"
#include "alloc/Allocator.hpp"
#include "alloc/MemoryBlock.hpp"
#include "util/easylogging++.h"

namespace vpr {

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

}