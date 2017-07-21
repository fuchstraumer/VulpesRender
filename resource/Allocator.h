#pragma once
#ifndef VULPES_VK_ALLOCATOR_H
#define VULPES_VK_ALLOCATOR_H

#include "stdafx.h"
#include "../ForwardDecl.h"
#include "../NonCopyable.h"

/*
	
	TODO:
	- Further dividing allocation among three size pools, and then still dividing among type in those pools
	- Measure costliness of Validate(), possibly use return codes to fix errors when possible.
	- Some kind of fragmentation check/routine: at least a check to make sure it isn't happening
	- Possibly use above to remove MinSuballocationSizeToRegister, instead shuffling dest ranges up/down
	  the needed amount to avoid gaps or fragmentation
	- in turn, will buffer-image granularity + alignment increase fragmentation? Couldn't find data on this.

*/

namespace vulpes {

	/*
	
		Constants and enums used by allocation system
	
	*/


	/*
		This sets the calling/usage of the memory validation routine.
		This can be costly, but it also returns the error present
		and could be used  (in some cases) to fix the error instead 
		of reporting a likely critical failure
	*/

#ifndef NDEBUG
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

	enum class SuballocationType : uint8_t {
		Free = 0, // unused entry
		Unknown, // could be various cpu storage objects, or extension objects
		Buffer,
		ImageUnknown, // image memory without defined tiling - possibly related to extensions
		ImageLinear,
		ImageOptimal,
	};

	enum class ValidationCode : uint8_t {
		VALIDATION_PASSED = 0,
		NULL_MEMORY_HANDLE, // allocation's memory handle is invalid
		ZERO_MEMORY_SIZE, // allocation's memory size is zero
		INCORRECT_SUBALLOC_OFFSET, // Offset of suballocation is incorrect: it may overlap with another, or it may be placed beyond the range of the allocation
		NEED_MERGE_SUBALLOCS, // two adjacent free suballoctions: merge them into one bigger suballocation
		FREE_SUBALLOC_COUNT_MISMATCH, // we found more free suballocations while validating than there are in the free suballoc list
		USED_SUBALLOC_IN_FREE_LIST, // non-free suballocation in free suballoc list
		FREE_SUBALLOC_SORT_INCORRECT, // free suballocation list is sorted by size ascending: sorting is incorrect, re-sort
		FINAL_SIZE_MISMATCH, // calculated offset as sum of all suballoc sizes is not equal to allocations total size
		FINAL_FREE_SIZE_MISMATCH, // calculated total available size doesn't match stored available size
	};

	static std::ostream& operator<<(std::ostream& os, const ValidationCode& code) {
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

	/*
	
		Utility functions for performing various allocation tasks.
		
	*/

	template<typename T>
	constexpr static T AlignUp(const T& offset, const T& alignment) {
		return (offset + alignment - 1) / alignment * alignment;
	}

	/*
		taken from spec section 11.6
		Essentially, we need to ensure that linear and non-linear resources are properly placed in adjacent memory so that
		they avoid any accidental aliasing.

		item_a = non-linaer object. item_b = linear object. page_size tends to be the bufferImageGranularity value retrieved by the allocators.
	*/
	constexpr static inline bool CheckBlocksOnSamePage(const VkDeviceSize& item_a_offset, const VkDeviceSize& item_a_size, const VkDeviceSize& item_b_offset, const VkDeviceSize& page_size) {
		assert(item_a_offset + item_a_size <= item_b_offset && item_a_size > 0 && page_size > 0);
		VkDeviceSize item_a_end = item_a_offset + item_a_size - 1;
		VkDeviceSize item_a_end_page = item_a_end & ~(page_size - 1);
		VkDeviceSize item_b_start_Page = item_b_offset & ~(page_size - 1);
		return item_a_end_page == item_b_start_Page;
	}

	/*
		Checks to make sure the two objects of type "type_a" and "type_b" wouldn't cause a conflict with the buffer-image granularity values. Returns true if
		conflict, false if no conflict.

		BufferImageGranularity specifies interactions between linear and non-linear resources, so we check based on those.
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
			// unkown return is playing it safe again, optimal return is because optimal tiling and linear buffers don't mix
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


	/*
	
		Small mostly POD-like structs used in allocation
		
	*/

	struct AllocationRequirements {
		// Defaults to false. If true, no new allocations are created beyond
		// the set created upon initilization of the allocator system.
		static VkBool32 noNewAllocations;

		// true if whatever allocation this belongs to should be in its own device memory object
		VkBool32 privateMemory = false;

		VkMemoryPropertyFlags requiredFlags;
		// acts as additional flags over above.
		VkMemoryPropertyFlags preferredFlags = VkMemoryPropertyFlags(0);
	};

	struct AllocationInfo {
		uint32_t memoryTypeIdx;
		VkDeviceMemory memory;
		VkDeviceSize offset;
		VkDeviceSize size;
		void* mappedData;
	};

	struct DefragInfo {
		VkDeviceSize maxSizeToMove;
		VkDeviceSize maxSuballocationsToMove;
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

	/*
		super-simple mutex helper struct that automatically locks-unlocks a mutex
		when it enters/exists scope.
	*/
	struct mutexWrapper {
		mutexWrapper(std::mutex& _mutex) : mutex(_mutex) {
			mutex.lock();
		}

		~mutexWrapper() {
			mutex.unlock();
		}
	private:
		std::mutex& mutex;
	};

	typedef std::vector<suballocationList::iterator>::iterator avail_suballocation_iterator_t;
	typedef std::vector<suballocationList::iterator>::const_iterator const_avail_suballocation_iterator_t;
	typedef suballocationList::iterator suballocation_iterator_t;
	typedef suballocationList::const_iterator const_suballocation_iterator_t;


	/*
	
		Main allocation classes and objects
	
	*/
	
	/*
		
		Allocation class represents a singular allocation: can be a private allocation (i.e, only user
		of attached DeviceMemory) or a block allocation (bound to sub-region of device memory)

	*/

	struct Allocation {

		enum class allocType {
			BLOCK_ALLOCATION,
			PRIVATE_ALLOCATION,
		};

		allocType Type;

		Allocation() = default;

		void Init(MemoryBlock* parent_block, const VkDeviceSize& offset, const VkDeviceSize& alignment, const VkDeviceSize& alloc_size, const SuballocationType& suballoc_type);
		void Update(MemoryBlock* new_parent_block, const VkDeviceSize& new_offset);
		void InitPrivate(const uint32_t& type_idx, VkDeviceMemory& dvc_memory, const SuballocationType& suballoc_type, bool persistently_mapped, void* mapped_data, const VkDeviceSize& data_size);

		const VkDeviceMemory& Memory() const;
		VkDeviceSize Offset() const noexcept;
		uint32_t MemoryTypeIdx() const noexcept;

		SuballocationType SuballocType;
		VkDeviceSize Size, Alignment;

		union {

			struct BlockAllocation {
				MemoryBlock* ParentBlock;
				VkDeviceSize Offset;
			} blockAllocation;

			struct PrivateAllocation {
				uint32_t MemoryTypeIdx;
				VkDeviceMemory DvcMemory;
				bool PersistentlyMapped;
				void* MappedData;
			} privateAllocation;

		};

	};

	/*
		MemoryBlock class contains singular VkDeviceMemory object.
		Should only have a handful of these at any one time.
	*/

	class MemoryBlock {
	public:

		MemoryBlock(Allocator* alloc);
		~MemoryBlock(); // just assert that memory has been free'd

		// new_memory is a fresh device memory object, new_size is size of this device memory object.
		void Init(VkDeviceMemory& new_memory, const VkDeviceSize& new_size);

		// cleans up resources and prepares object to be safely destroyed.
		void Destroy(Allocator* alloc);

		// Used when sorting AllocationCollection
		bool operator<(const MemoryBlock& other);

		VkDeviceSize AvailableMemory() const noexcept;
		const VkDeviceMemory& Memory() const noexcept;

		// Verifies integrity of memory by checking all contained structs/objects.
		ValidationCode Validate() const;

		bool RequestSuballocation(const VkDeviceSize& buffer_image_granularity, const VkDeviceSize& allocation_size, const VkDeviceSize& allocation_alignment, SuballocationType allocation_type, SuballocationRequest* dest_request);

		// Verifies that requested suballocation can be added to this object, and sets dest_offset to reflect offset of this now-inserted suballocation.
		bool VerifySuballocation(const VkDeviceSize& buffer_image_granularity, const VkDeviceSize& allocation_size, const VkDeviceSize& allocation_alignment,
			SuballocationType allocation_type, const suballocationList::const_iterator & dest_suballocation_location, VkDeviceSize* dest_offset) const;

		bool Empty() const;

		// performs the actual allocation, once "request" has been checked and made valid.
		void Allocate(const SuballocationRequest& request, const SuballocationType& allocation_type, const VkDeviceSize& allocation_size);

		// Frees memory in region specified (i.e frees/destroys a suballocation)
		void Free(const Allocation* memory_to_free);

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

		VkDeviceSize availSize; // total available size among all sub-allocations.
		uint32_t freeCount;
		VkDeviceMemory memory;

		// given a free suballocation, this method merges it with the one immediately after it in the list
		// the second item must also be free: this is a method used to pool smaller adjacent regions together.
		// (which reduces fragmentation and increases the max available size)
		void mergeFreeWithNext(const suballocationList::iterator& item_to_merge);

		// releases given suballocation, and then merges it with any adjacent blocks if possible.
		void freeSuballocation(const suballocationList::iterator& item_to_free);

		// given a free suballocation, place it in the correct location of our suballocation list (based on avail size)
		void insertFreeSuballocation(const suballocationList::iterator& item_to_insert);

		// given a free suballocation, remove it from the list (if possible)
		void removeFreeSuballocation(const suballocationList::iterator& item_to_remove);

		// Suballocations sorted by available size, only in this vector
		// if available size is greater than a threshold we set shortly.
		std::vector<suballocationList::iterator> availSuballocations;
	};

	typedef std::vector<MemoryBlock*>::iterator allocation_iterator_t;
	typedef std::vector<MemoryBlock*>::const_iterator const_allocation_iterator_t;

	struct AllocationCollection {
		std::vector<MemoryBlock*> allocations;

		AllocationCollection() = default;
		AllocationCollection(Allocator* allocator);

		~AllocationCollection();

		MemoryBlock* operator[](const size_t& idx);
		const MemoryBlock* operator[](const size_t& idx) const;

		bool Empty() const;

		// attempts to free memory: returns index of free'd allocation or -1 if not 
		// able to free or not able to find desired memory
		size_t Free(const Allocation* memory_to_free);

		// performs single sort step, to order "allocations" so that it is sorted
		// by total available free memory.
		void SortAllocations();

		
	private:
		Allocator* allocator;
	};

	class Defragmenter {
		const Device* parent;
		VkDeviceSize BufferImageGranularity;
		uint32_t MemoryTypeIdx;
		VkDeviceSize BytesMoved;
		uint32_t AllocationsMoved;
	};

	class Allocator : public NonMovable {
	public:

		Allocator(const Device* parent_dvc);
		~Allocator();

		VkDeviceSize GetPreferredBlockSize(const uint32_t& memory_type_idx) const noexcept;
		VkDeviceSize GetBufferImageGranularity() const noexcept;

		uint32_t GetMemoryHeapCount() const noexcept;
		uint32_t GetMemoryTypeCount() const noexcept;

		const VkDevice& DeviceHandle() const noexcept;

		VkResult AllocateMemory(const VkMemoryRequirements& memory_reqs, const AllocationRequirements& alloc_details, const SuballocationType& suballoc_type, Allocation& dest_allocation);

		void FreeMemory(const Allocation* memory_to_free);

		//  Maps given memory range to given void** destination
		VkResult MapMemoryAllocation(const Allocation& alloc_to_map, void** dest);

		// Unmaps given range
		VkResult UnmapMemoryAllocation(const Allocation& range);

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

		std::vector<AllocationCollection*> allocations;
		std::vector<AllocationCollection*> privateAllocations;
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
