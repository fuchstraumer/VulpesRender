#include "stdafx.h"
#include "Allocator.h"
#include "engine/util/easylogging++.h"
#include "../core/LogicalDevice.h"
#include "../core/PhysicalDevice.h"

namespace vulpes {

	VkBool32 AllocationRequirements::noNewAllocations = false;

	// padding to inject at end of allocations ot test allocation system
	static constexpr size_t DEBUG_PADDING = 0;

	MemoryBlock::MemoryBlock(Allocator * alloc) : allocator(alloc), availSize(0), freeCount(0), memory(VK_NULL_HANDLE), Size(0) {}

	MemoryBlock::~MemoryBlock() {
		assert(memory == VK_NULL_HANDLE);
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

	}

	void MemoryBlock::Destroy(Allocator * alloc) {
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
		if (availSize < allocation_size | availSuballocations.empty()) {
			// not enough space in this allocation object
			return false;
		}

		const size_t numFreeSuballocs = availSuballocations.size();

		// use lower_bound to find location of avail suballocation

		size_t avail_idx = 0;
		for (auto iter = availSuballocations.cbegin(); iter != availSuballocations.cend(); ++iter) {
			if ((*iter)->size > allocation_size) {
				avail_idx = iter - availSuballocations.cbegin();
				break;
			}
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
		if (padding_begin + padding_end > suballoc.size) {
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
			++next_iter;
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
		// Choose search direction based based on size of object to free
		const bool forward_direction = (memory_to_free->blockAllocation.Offset) < (Size / 2);
		if (forward_direction) {
			for (auto iter = Suballocations.begin(); iter != Suballocations.end(); ++iter) {
				auto& suballoc = *iter;
				if (suballoc.offset == memory_to_free->blockAllocation.Offset) {
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
		else {
			auto iter = Suballocations.end();
			--iter;
			for (; iter != Suballocations.begin(); --iter) {
				auto& suballoc = *iter;
				if (suballoc.offset == memory_to_free->blockAllocation.Offset) {
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
			allocations[i]->Destroy(allocator);
			delete allocations[i];
		}
	}

	MemoryBlock * AllocationCollection::operator[](const size_t & idx) {
		return allocations[idx];
	}

	const MemoryBlock * AllocationCollection::operator[](const size_t & idx) const {
		return allocations[idx];
	}

	bool AllocationCollection::Empty() const {
		return allocations.empty();
	}

	size_t AllocationCollection::Free(const Allocation * memory_to_free) {
		
		VkDeviceSize alloc_offset = memory_to_free->blockAllocation.Offset;
		allocations[memory_to_free->blockAllocation.ParentBlock->MemoryTypeIdx]->Free(memory_to_free);

		return std::numeric_limits<size_t>::max();
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
			allocations[i] = new AllocationCollection(this);
			privateAllocations[i] = new AllocationCollection(this);
			emptyAllocations[i] = false;
		}
	}

	Allocator::~Allocator() {

		/*
			Delete collections: destructors of these should take care of the rest.
		*/

		for (size_t i = 0; i < GetMemoryTypeCount(); ++i) {
			delete allocations[i];
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
		MemoryBlock* alloc_to_delete = nullptr;
		bool found = false; // searching for given memory range.
		for (; type_idx < GetMemoryTypeCount(); ++type_idx) {
			auto& allocation_collection = allocations[type_idx];
			const size_t alloc_idx = allocation_collection->Free(memory_to_free);
			if (alloc_idx != std::numeric_limits<size_t>::max()) {
				LOG(INFO) << "Memory freed successfully.";
				found = true;
				auto& alloc = allocation_collection->allocations[alloc_idx];
				// did freeing the given memory made "alloc" empty?
				if (alloc->Empty()) {
					// alloc is now empty: but, we already have an empty allocation instance
					// for this memory type index, so remove it from the vector and specify 
					// that we'll fully delete it.
					if (emptyAllocations[type_idx]) {
						// remove allocation.
						alloc_to_delete = alloc;
						allocation_collection->allocations.erase(allocation_collection->allocations.begin() + alloc_idx);
						break;
					}
					else {
						// alloc is now the empty allocation at that index.
						emptyAllocations[type_idx] = true;
					}
				}
				// re-sort the collection.
				allocation_collection->SortAllocations();
				break;
			}
		}

		if (found) {
			if (alloc_to_delete != nullptr) {
				LOG(INFO) << "Deleted an allocation.";
				// need to cleanup resources first, before deleting the actual object.
				alloc_to_delete->Destroy(this);
				delete alloc_to_delete;
			}
			return;
		}

		// memory_to_free not found, possible a privately/singularly allocated memory object
		if (freePrivateMemory(memory_to_free)) {
			return;
		}

		LOG(ERROR) << "Failed to free memory.";
		throw std::runtime_error("Unable to free given memory.");
		return;
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
					dest_allocation.Init(block, request.offset, memory_reqs.alignment, memory_reqs.size, type);

					if (VALIDATE_MEMORY) {
						ValidationCode result_code = block->Validate();
						if (result_code != ValidationCode::VALIDATION_PASSED) {
							LOG(ERROR) << "Validation of new allocation failed with reason: " << result_code;
							throw std::runtime_error("");
						}
					}

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
				//assert(result != VK_ERROR_OUT_OF_DEVICE_MEMORY); // make sure we're not over-allocating and using all device memory.
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

				MemoryBlock* new_block = new MemoryBlock(this);
				// allocation size is more up-to-date than mem reqs size
				new_block->Init(new_memory, alloc_info.allocationSize);
				new_block->MemoryTypeIdx = memory_type_idx;
				alloc_collection->allocations.push_back(new_block);

				SuballocationRequest request{ *new_block->avail_begin(), 0 };
				new_block->Allocate(request, type, memory_reqs.size);
				
				dest_allocation.Init(new_block, request.offset, memory_reqs.alignment, memory_reqs.size, type);

				if (VALIDATE_MEMORY) {
					ValidationCode result_code = new_block->Validate();
					if (result_code != ValidationCode::VALIDATION_PASSED) {
						LOG(ERROR) << "Validation of new allocation failed with reason: " << result_code;
					}
				}

				LOG(INFO) << "Created new allocation object w/ size of " << std::to_string(alloc_info.allocationSize);
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

		// Get memory info.
		VkMemoryRequirements memreqs;
		vkGetImageMemoryRequirements(parent->vkHandle(), image_handle, &memreqs);

		return AllocateMemory(memreqs, details, alloc_type, dest_allocation);
	}

	VkResult Allocator::AllocateForBuffer(VkBuffer & buffer_handle, const AllocationRequirements & details, const SuballocationType & alloc_type, Allocation& dest_allocation) {
		VkMemoryRequirements memreqs;
		vkGetBufferMemoryRequirements(parent->vkHandle(), buffer_handle, &memreqs);
		return AllocateMemory(memreqs, details, alloc_type, dest_allocation);
	}

	VkResult Allocator::CreateImage(VkImage * image_handle, const VkImageCreateInfo * img_create_info, const AllocationRequirements & alloc_reqs, Allocation& dest_allocation) {

		// create image object first.
		VkResult result = vkCreateImage(parent->vkHandle(), img_create_info, nullptr, image_handle);
		VkAssert(result);

		SuballocationType image_type = img_create_info->tiling == VK_IMAGE_TILING_OPTIMAL ? SuballocationType::ImageOptimal : SuballocationType::ImageLinear;
		result = AllocateForImage(*image_handle, alloc_reqs, image_type, dest_allocation);
		VkAssert(result);

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

	void Allocation::Init(MemoryBlock * parent_block, const VkDeviceSize & offset, const VkDeviceSize & alignment, const VkDeviceSize & alloc_size, const SuballocationType & suballoc_type) {
		Type = allocType::BLOCK_ALLOCATION;
		blockAllocation.ParentBlock = parent_block;
		blockAllocation.Offset = offset;
		Size = alloc_size;
		Alignment = alignment;
	}

	void Allocation::Update(MemoryBlock * new_parent_block, const VkDeviceSize & new_offset) {
		blockAllocation.ParentBlock = new_parent_block;
		blockAllocation.Offset = new_offset;
	}

	void Allocation::InitPrivate(const uint32_t & type_idx, VkDeviceMemory & dvc_memory, const SuballocationType & suballoc_type, bool persistently_mapped, void * mapped_data, const VkDeviceSize & data_size) {
		Size = data_size;
		SuballocType = suballoc_type;
		privateAllocation.DvcMemory = dvc_memory;
		privateAllocation.MemoryTypeIdx = type_idx;
		privateAllocation.PersistentlyMapped = persistently_mapped;
		privateAllocation.MappedData = mapped_data;
	}

	const VkDeviceMemory & Allocation::Memory() const {
		if (Type == allocType::BLOCK_ALLOCATION) {
			return blockAllocation.ParentBlock->Memory();
		}
		else {
			return privateAllocation.DvcMemory;
		}
	}

	VkDeviceSize Allocation::Offset() const noexcept {
		if (Type == allocType::BLOCK_ALLOCATION) {
			return blockAllocation.Offset;
		}
		else {
			return VkDeviceSize(0);
		}
	}

	uint32_t Allocation::MemoryTypeIdx() const noexcept {
		if (Type == allocType::BLOCK_ALLOCATION) {
			return blockAllocation.ParentBlock->MemoryTypeIdx;
		}
		else {
			return privateAllocation.MemoryTypeIdx;
		}
	}

}