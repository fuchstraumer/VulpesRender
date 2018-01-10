#include "vpr_stdafx.h"
#include "alloc/Allocator.hpp"
#include "core/LogicalDevice.hpp"
#include "alloc/Allocation.hpp"
#include "alloc/MemoryBlock.hpp"
#include "util/easylogging++.h"
#include <sstream>

namespace vpr {

    Allocator::Allocator(const Device * parent_dvc) : parent(parent_dvc), preferredSmallHeapBlockSize(DefaultSmallHeapBlockSize), preferredLargeHeapBlockSize(DefaultLargeHeapBlockSize) {
        deviceProperties = parent->GetPhysicalDeviceProperties();
        deviceMemoryProperties = parent->GetPhysicalDeviceMemoryProperties();
        allocations.resize(GetMemoryTypeCount());
        emptyAllocations.resize(GetMemoryTypeCount());
        // initialize base pools, one per memory type.
        for (size_t i = 0; i < GetMemoryTypeCount(); ++i) {
            allocations[i] = std::make_unique<AllocationCollection>(this);
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
        emptyAllocations.shrink_to_fit();

    }

    void Allocator::Recreate() {

        allocations.clear();
        privateAllocations.clear();
        emptyAllocations.clear();
        allocations.shrink_to_fit();
        emptyAllocations.shrink_to_fit();

        allocations.resize(GetMemoryTypeCount());
        emptyAllocations.resize(GetMemoryTypeCount());
        // initialize base pools, one per memory type.
        for (size_t i = 0; i < GetMemoryTypeCount(); ++i) {
            allocations[i] = std::make_unique<AllocationCollection>(this);
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

        if (!memory_to_free->IsPrivateAllocation()) {

            type_idx = memory_to_free->MemoryTypeIdx();
            auto& allocation_collection = allocations[type_idx];
            
            auto* block = (*allocation_collection)[0];
            auto free_size = memory_to_free->Size;
            block->Free(memory_to_free);
            LOG_IF((static_cast<float>(free_size) / 1.0e6f) > 0.5f, INFO) << "Freed a memory allocation with size " << std::to_string(free_size / 1e6) << "mb";
            if (VALIDATE_MEMORY) {
                auto err = block->Validate();
                if (err != ValidationCode::VALIDATION_PASSED) {
                    std::stringstream ss;
                    ss << "Validtion of memory failed with error: " << err;
                    const std::string err_str = ss.str();
                    LOG(ERROR) << err_str.c_str();
                    throw std::runtime_error(err_str.c_str());
                }
            }

            if (block->Empty()) {
                if (emptyAllocations[type_idx]) {
                    block->Destroy(this);
                    allocation_collection->RemoveBlock(block);
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

        // Should never reach this point.
        LOG(ERROR) << "Failed to free a memory allocation!";
        throw std::runtime_error("Failed to free a memory allocation.");
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
                return allocatePrivateMemory(memory_reqs.size, memory_type_idx, dest_allocation);
            }
        }
        else {
            auto& alloc_collection = allocations[memory_type_idx];

            // first, check existing allocations
            for (auto iter = alloc_collection->cbegin(); iter != alloc_collection->cend(); ++iter) {
                SuballocationRequest request;
                const auto& block = *iter;
                if (block->RequestSuballocation(GetBufferImageGranularity(), memory_reqs.size, memory_reqs.alignment, type, &request)) {
                    if (block->Empty()) {
                        emptyAllocations[memory_type_idx] = false;
                    }

                    block->Allocate(request, type, memory_reqs.size);
                    dest_allocation.Init(block.get(), request.offset, memory_reqs.alignment, memory_reqs.size);

                    if (VALIDATE_MEMORY) {
                        ValidationCode result_code = block->Validate();
                        if (result_code != ValidationCode::VALIDATION_PASSED) {
                            std::stringstream ss;
                            ss << "Validation of new allocation failed with reason: " << result_code;
                            const std::string fail_str{ ss.str() }; 
                            LOG(ERROR) << fail_str.c_str();
                            throw std::runtime_error(fail_str.c_str());
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
                    result = allocatePrivateMemory(memory_reqs.size, memory_type_idx, dest_allocation);
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
                size_t new_block_idx = alloc_collection->AddMemoryBlock(std::move(new_block));

                auto* new_block_ptr = (*alloc_collection)[new_block_idx];
                // allocation size is more up-to-date than mem reqs size
                new_block_ptr->Init(new_memory, alloc_info.allocationSize);
                new_block_ptr->MemoryTypeIdx = memory_type_idx;

                SuballocationRequest request{ *new_block_ptr->avail_begin(), 0 };
                new_block_ptr->Allocate(request, type, memory_reqs.size);
                
                dest_allocation.Init(new_block_ptr, request.offset, memory_reqs.alignment, memory_reqs.size);

                if (VALIDATE_MEMORY) {
                    ValidationCode result_code = new_block_ptr->Validate();
                    if (result_code != ValidationCode::VALIDATION_PASSED) {
                        std::stringstream ss;
                        ss << "Validation of new allocation failed with reason: " << result_code;
                        const std::string err_str{ ss.str() };
                        LOG(ERROR) << err_str.c_str();
                        throw std::runtime_error(err_str.c_str());
                    }
                }

                LOG(INFO) << "Created new allocation object w/ size of " << std::to_string(alloc_info.allocationSize * 1e-6) << "mb";
                return VK_SUCCESS;
            }
        }

    }

    VkResult Allocator::allocatePrivateMemory(const VkDeviceSize & size, const uint32_t & memory_type_idx, Allocation& dest_allocation) {
        VkMemoryAllocateInfo alloc_info{ VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO, nullptr, size, memory_type_idx };

        VkDeviceMemory private_memory_handle = VK_NULL_HANDLE;
        VkResult result = vkAllocateMemory(parent->vkHandle(), &alloc_info, nullptr, &private_memory_handle);
        VkAssert(result);
        
        dest_allocation.InitPrivate(memory_type_idx, private_memory_handle, false, nullptr, size);
        {
            std::lock_guard<std::mutex> private_guard(privateMutex);
            privateAllocations.insert(std::unique_ptr<Allocation>(&dest_allocation));
        }
        
        return VK_SUCCESS;
    }

    struct raw_equal_comparator {
        raw_equal_comparator(const Allocation* _ptr) : ptr(ptr) {};
        bool operator()(const Allocation* other) const {
            return ptr == other;
        }
        bool operator()(const std::unique_ptr<Allocation>& unique_other) const {
            return ptr == unique_other.get();
        }
        const Allocation* ptr;
    };

    bool Allocator::freePrivateMemory(const Allocation * alloc_to_free) {

        assert(alloc_to_free->IsPrivateAllocation());
        auto iter = std::find_if(privateAllocations.begin(), privateAllocations.end(), raw_equal_comparator(alloc_to_free));
        if (iter == privateAllocations.end()) {
            LOG(ERROR) << "Couldn't find alloc_to_free in privateAllocations container!";
            return false;
        }
        else {
            std::lock_guard<std::mutex> private_guard(privateMutex);
            VkDeviceMemory private_handle = (*iter).get()->Memory();
            (*iter).get()->Unmap();
            vkFreeMemory(parent->vkHandle(), private_handle, nullptr);
            privateAllocations.erase(iter);
            LOG(INFO) << "Freed a private memory allocation.";
            return true;
        }

        return false;
    }

    VkResult Allocator::AllocateForImage(VkImage & image_handle, const AllocationRequirements & details, const SuballocationType & alloc_type, Allocation& dest_allocation) {
        std::mutex image_alloc_mutex;
        std::lock_guard<std::mutex> image_alloc_guard(image_alloc_mutex);
        // Get memory info.
        VkMemoryRequirements memreqs;
        vkGetImageMemoryRequirements(parent->vkHandle(), image_handle, &memreqs);
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

}
