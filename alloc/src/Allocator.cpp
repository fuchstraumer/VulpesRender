#include "vpr_stdafx.h"
#include "Allocator.hpp"
#include "AllocCommon.hpp"
#include "AllocationCollection.hpp"
#include "Allocation.hpp"
#include "AllocationRequirements.hpp"
#include "MemoryBlock.hpp"
#include "easylogging++.h"
INITIALIZE_EASYLOGGINGPP
#include "vkAssert.hpp"
#include "CreateInfoBase.hpp"
#include <sstream>
#include <vulkan/vulkan.h>
#include <unordered_set>
#include <map>
#include <vector>

namespace vpr {

    void SetLoggingRepository_VprAlloc(void* storage_ptr) {
        el::base::type::StoragePointer* ptr = reinterpret_cast<el::base::type::StoragePointer*>(storage_ptr);
        el::Helpers::setStorage(*ptr);
        LOG(INFO) << "Updated easyloggingpp storage pointer in vpr_alloc module...";
    }

    void* GetLoggingRepository_VprAlloc() {
        static el::base::type::StoragePointer ptr = el::Helpers::storage();
        return ptr.get();
    }

    vpr::SuballocationType suballocTypeFromAllocType(const AllocationType& alloc_type) {
        switch (alloc_type) {
        case AllocationType::Buffer:
            return SuballocationType::Buffer;
        case AllocationType::ImageLinear:
            return SuballocationType::ImageLinear;
        case AllocationType::ImageTiled:
            return SuballocationType::ImageOptimal;
        case AllocationType::Unknown:
            return SuballocationType::Unknown;
        default:
            return SuballocationType::Unknown;
        };
    }

    struct AllocatorImpl {

        AllocatorImpl(const VkDevice& dvc, const VkPhysicalDevice& physical_device, Allocator::allocation_extensions extensions);

        VkDeviceSize GetPreferredBlockSize(const uint32_t& memory_type_idx) const noexcept;
        VkDeviceSize GetBufferImageGranularity() const noexcept;

        uint32_t GetMemoryHeapCount() const noexcept;
        uint32_t GetMemoryTypeCount() const noexcept;

        void createAllocationMaps();
        void clearAllocationMaps();

        // Won't throw: but can return invalid indices. Make sure to handle this.
        uint32_t findMemoryTypeIdx(const VkMemoryRequirements& mem_reqs, const AllocationRequirements& details) const noexcept;

        // These allocation methods return VkResult's so that we can try different parameters (based partially on return code) in main allocation method.
        VkResult allocateMemoryType(const VkMemoryRequirements& memory_reqs, const AllocationRequirements& alloc_details, const uint32_t& memory_type_idx, const SuballocationType& type, Allocation& dest_allocation);
        VkResult allocatePrivateMemory(const VkDeviceSize& size, const uint32_t& memory_type_idx, Allocation& dest_allocation, const VkMemoryPropertyFlags memory_flags);

        // called from "FreeMemory" if memory to free isn't found in the allocation vectors for any of our active memory types.
        bool freePrivateMemory(const Allocation* memory_to_free);

        void getBufferMemReqs(VkBuffer& handle, VkMemoryRequirements& reqs, bool& requires_dedicated, bool& prefers_dedicated);
        void getImageMemReqs(VkImage& handle, VkMemoryRequirements& reqs, bool& requires_dedicated, bool& prefers_dedicated);

        enum class AllocationSize {
            SMALL,
            MEDIUM,
            LARGE,
            EXTRA_LARGE
        };

        AllocationSize GetAllocSize(const VkDeviceSize& size) const;

        std::map<AllocationSize, std::vector<std::unique_ptr<AllocationCollection>>> allocations;
        std::map<AllocationSize, std::vector<bool>> emptyAllocations;
        std::unordered_set<std::unique_ptr<Allocation>> privateAllocations;

        /**Guards the private allocations set, since it's a different object entirely than the main one.
        */
        std::mutex privateMutex;
        std::mutex allocMutex;
        const VkDevice logicalDevice;
        const VkPhysicalDevice physicalDevice;

        VkPhysicalDeviceProperties deviceProperties;
        VkPhysicalDeviceMemoryProperties deviceMemoryProperties;

        VkDeviceSize preferredLargeHeapBlockSize;
        VkDeviceSize preferredSmallHeapBlockSize;
        const VkAllocationCallbacks* pAllocationCallbacks = nullptr;
        bool usingMemoryExtensions;

        /*
        Used GPU Open allocator impl. hints and this:
        http://asawicki.info/articles/VK_KHR_dedicated_allocation.php5
        blogpost to implement support for this extension.
        */
        void fetchAllocFunctionPointersKHR();

        PFN_vkGetBufferMemoryRequirements2KHR pVkGetBufferMemoryRequirements2KHR;
        PFN_vkGetImageMemoryRequirements2KHR pVkGetImageMemoryRequirements2KHR;
        PFN_vkGetImageSparseMemoryRequirements2KHR pVkGetImageSparseMemoryRequirements2KHR;
    };

    struct raw_equal_comparator {
        raw_equal_comparator(const Allocation* _ptr) : ptr(_ptr) {};
        bool operator()(const Allocation* other) const {
            return ptr == other;
        }
        bool operator()(const std::unique_ptr<Allocation>& unique_other) const {
            return ptr == unique_other.get();
        }
        const Allocation* ptr;
    };

    Allocator::Allocator(const VkDevice& logical_device, const VkPhysicalDevice& physical_device, allocation_extensions dedicated_alloc_enabled) : impl(std::make_unique<AllocatorImpl>(logical_device, physical_device, dedicated_alloc_enabled)) {}

    void AllocatorImpl::createAllocationMaps() {
        allocations.emplace(AllocationSize::SMALL, std::vector<std::unique_ptr<AllocationCollection>>());
        auto& small_allocs = allocations.at(AllocationSize::SMALL);
        for (size_t i = 0; i < GetMemoryTypeCount(); ++i) {
            small_allocs.emplace_back(std::make_unique<AllocationCollection>());
        }
        emptyAllocations.emplace(AllocationSize::SMALL, std::vector<bool>(GetMemoryTypeCount(), true));

        allocations.emplace(AllocationSize::MEDIUM, std::vector<std::unique_ptr<AllocationCollection>>());
        auto& med_allocs = allocations.at(AllocationSize::MEDIUM);
        for (size_t i = 0; i < GetMemoryTypeCount(); ++i) {
            med_allocs.emplace_back(std::make_unique<AllocationCollection>());
        }
        emptyAllocations.emplace(AllocationSize::MEDIUM, std::vector<bool>(GetMemoryTypeCount(), true));

        allocations.emplace(AllocationSize::LARGE, std::vector<std::unique_ptr<AllocationCollection>>());
        auto& large_allocs = allocations.at(AllocationSize::LARGE);
        for (size_t i = 0; i < GetMemoryTypeCount(); ++i) {
            large_allocs.emplace_back(std::make_unique<AllocationCollection>());
        }
        emptyAllocations.emplace(AllocationSize::LARGE, std::vector<bool>(GetMemoryTypeCount(), true));
    }

    void AllocatorImpl::clearAllocationMaps() {
        /*
        Delete collections: destructors of these should take care of the rest.
        */
        allocations.clear();
        privateAllocations.clear();
        emptyAllocations.clear();
    }

    Allocator::~Allocator() {
        impl->clearAllocationMaps();
    }

    void Allocator::Recreate() {
        impl->clearAllocationMaps();
        impl->createAllocationMaps();
    }

    AllocatorImpl::AllocatorImpl(const VkDevice& parent_dvc, const VkPhysicalDevice& phys_device, Allocator::allocation_extensions extensions) : logicalDevice(parent_dvc), physicalDevice(phys_device),
        usingMemoryExtensions((extensions == Allocator::allocation_extensions::DedicatedAllocations) ? true : false), preferredSmallHeapBlockSize(DefaultSmallHeapBlockSize), preferredLargeHeapBlockSize(DefaultLargeHeapBlockSize) {
        vkGetPhysicalDeviceProperties(physicalDevice, &deviceProperties);
        vkGetPhysicalDeviceMemoryProperties(physicalDevice, &deviceMemoryProperties);

        if (usingMemoryExtensions) {
            fetchAllocFunctionPointersKHR();
        }

        createAllocationMaps();
    }

    VkDeviceSize AllocatorImpl::GetPreferredBlockSize(const uint32_t& memory_type_idx) const noexcept {
        VkDeviceSize heapSize = deviceMemoryProperties.memoryHeaps[deviceMemoryProperties.memoryTypes[memory_type_idx].heapIndex].size;
        return (heapSize <= DefaultSmallHeapBlockSize) ? preferredSmallHeapBlockSize : preferredLargeHeapBlockSize;
    }

    AllocatorImpl::AllocationSize AllocatorImpl::GetAllocSize(const VkDeviceSize & size) const {
        if (size > LargeBlockSingleAllocSize * 32) {
            // Would take up half a large block - should probably make private
            return AllocationSize::LARGE;
        }
        else if (size > MediumBlockSingleAllocSize) {
            return AllocationSize::LARGE;
        }
        else if (size > SmallBlockSingleAllocSize) {
            return AllocationSize::MEDIUM;
        }
        else {
            return AllocationSize::SMALL;
        }
    }

    VkDeviceSize AllocatorImpl::GetBufferImageGranularity() const noexcept {
        return deviceProperties.limits.bufferImageGranularity;
    }

    uint32_t AllocatorImpl::GetMemoryHeapCount() const noexcept {
        return deviceMemoryProperties.memoryHeapCount;
    }

    uint32_t AllocatorImpl::GetMemoryTypeCount() const noexcept {
        return deviceMemoryProperties.memoryTypeCount;
    }

    const VkDevice& Allocator::DeviceHandle() const noexcept {
        return impl->logicalDevice;
    }

    VkResult Allocator::AllocateMemory(const VkMemoryRequirements& memory_reqs, const AllocationRequirements& alloc_details, const AllocationType& alloc_type, Allocation& dest_allocation) {
        
        // find memory type (i.e idx) required for this allocation
        uint32_t memory_type_idx = impl->findMemoryTypeIdx(memory_reqs, alloc_details);
        if (memory_type_idx != std::numeric_limits<uint32_t>::max()) {
            return impl->allocateMemoryType(memory_reqs, alloc_details, memory_type_idx, suballocTypeFromAllocType(alloc_type), dest_allocation);
        }
        else {
            return VK_ERROR_OUT_OF_DEVICE_MEMORY;
        }

    }

    void Allocator::FreeMemory(const Allocation* memory_to_free) {
        uint32_t type_idx = 0;

        if (!memory_to_free->IsPrivateAllocation()) {

            type_idx = memory_to_free->MemoryTypeIdx();
            // allocations binned vaguely by size, alongside type
            const auto alloc_bin = impl->GetAllocSize(memory_to_free->Size);
            auto& allocation_collection = impl->allocations.at(alloc_bin)[type_idx];
            
            auto* block = (*allocation_collection)[0];
            auto free_size = memory_to_free->Size;
            block->Free(memory_to_free);
            LOG_IF(VERBOSE_LOGGING, INFO) << "Freed a memory allocation with size " << std::to_string(free_size / 1e6) << "mb";
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
                if (impl->emptyAllocations.at(alloc_bin)[type_idx]) {
                    block->Destroy();
                    allocation_collection->RemoveBlock(block);
                }
                else {
                    impl->emptyAllocations.at(alloc_bin)[type_idx] = true;
                }
            }

            // re-sort the collection.
            allocation_collection->SortAllocations();
            
            return;
        }

        


        // memory_to_free not found, possible a privately/singularly allocated memory object
        if (impl->freePrivateMemory(memory_to_free)) {
            return;
        }

        // Should never reach this point.
        LOG(ERROR) << "Failed to free a memory allocation!";
        throw std::runtime_error("Failed to free a memory allocation.");
    }

    VkResult Allocator::AllocateForImage(VkImage & image_handle, const AllocationRequirements & details, const AllocationType& alloc_type, Allocation& dest_allocation) {
        VkMemoryRequirements memreqs;
        AllocationRequirements details2 = details;
        impl->getImageMemReqs(image_handle, memreqs, details2.requiresDedicatedKHR, details2.prefersDedicatedKHR);
        VkResult result = AllocateMemory(memreqs, details2, alloc_type, dest_allocation);  
        vkBindImageMemory(impl->logicalDevice, image_handle, dest_allocation.Memory(), dest_allocation.Offset());
        return result;
    }

    VkResult Allocator::AllocateForBuffer(VkBuffer & buffer_handle, const AllocationRequirements & details, const AllocationType& alloc_type, Allocation& dest_allocation) {
        VkMemoryRequirements memreqs;
        AllocationRequirements details2 = details;
        impl->getBufferMemReqs(buffer_handle, memreqs, details2.requiresDedicatedKHR, details2.prefersDedicatedKHR);
        VkResult result = AllocateMemory(memreqs, details2, alloc_type, dest_allocation);
        vkBindBufferMemory(impl->logicalDevice, buffer_handle, dest_allocation.Memory(), dest_allocation.Offset());
        return result;
    }
    
    uint32_t AllocatorImpl::findMemoryTypeIdx(const VkMemoryRequirements& mem_reqs, const AllocationRequirements & details) const noexcept {
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

    VkResult AllocatorImpl::allocateMemoryType(const VkMemoryRequirements & memory_reqs, const AllocationRequirements & alloc_details, const uint32_t & memory_type_idx, const SuballocationType & type, Allocation& dest_allocation) {
    
        const VkDeviceSize preferredBlockSize = GetPreferredBlockSize(memory_type_idx);

        // If given item is bigger than our preferred block size, we give it its own special allocation (using a single device memory object for this).
        const bool private_memory = (alloc_details.prefersDedicatedKHR || alloc_details.requiresDedicatedKHR) || (memory_reqs.size > preferredBlockSize / 2);

        if (private_memory) {
            if (AllocationRequirements::noNewAllocations) {
                LOG(ERROR) << "Disabled creation of allocations outside of block-vectors and subregion binding, but tried to allocate a unique/private/dedicated allocation!";
                throw std::domain_error("Required options for this type of allocation not enabled.");
            }
            else {
                LOG_IF((alloc_details.prefersDedicatedKHR || alloc_details.requiresDedicatedKHR),INFO) << "Allocating dedicated memory object using extensions...";
                return allocatePrivateMemory(memory_reqs.size, memory_type_idx, dest_allocation, alloc_details.requiredFlags);
            }
        }
        else {
            const auto rec_alloc_bin = GetAllocSize(memory_reqs.size);

            auto& alloc_collection = allocations.at(rec_alloc_bin)[memory_type_idx];

            // first, check existing allocations
            for (auto iter = alloc_collection->cbegin(); iter != alloc_collection->cend(); ++iter) {
                SuballocationRequest request;
                const auto& block = *iter;
                if (block->RequestSuballocation(GetBufferImageGranularity(), memory_reqs.size, memory_reqs.alignment, type, &request)) {
                    if (block->Empty()) {
                        emptyAllocations.at(rec_alloc_bin)[memory_type_idx] = false;
                    }

                    block->Allocate(request, type, memory_reqs.size);
                    dest_allocation.Init(block.get(), request.Offset, memory_reqs.alignment, memory_reqs.size);

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
                    LOG_IF(VERBOSE_LOGGING, INFO) << "Successfully allocated by binding to suballocation with size of " << std::to_string(memory_reqs.size / 1e6) << "mb at offset " << std::to_string(request.Offset);
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

                switch (rec_alloc_bin) {
                case AllocationSize::LARGE:
                    alloc_info.allocationSize = DefaultLargeHeapBlockSize;
                    break;
                case AllocationSize::MEDIUM:
                    alloc_info.allocationSize = DefaultMediumHeapBlockSize;
                    break;
                case AllocationSize::SMALL:
                    alloc_info.allocationSize = DefaultSmallHeapBlockSize;
                    break;
                default:
                    throw std::domain_error("Somehow got to an invalid case in switch statement setting new allocation size: how did this occur?");
                }
                
                VkDeviceMemory new_memory = VK_NULL_HANDLE;
                VkResult result = vkAllocateMemory(logicalDevice, &alloc_info, nullptr, &new_memory);
                assert(result != VK_ERROR_OUT_OF_DEVICE_MEMORY); // make sure we're not over-allocating and using all device memory.
                if (result != VK_SUCCESS) {
                    // halve allocation size
                    alloc_info.allocationSize /= 2;
                    if (alloc_info.allocationSize >= memory_reqs.size) {
                        result = vkAllocateMemory(logicalDevice, &alloc_info, nullptr, &new_memory);
                        if (result != VK_SUCCESS) {
                            alloc_info.allocationSize /= 2;
                            if (alloc_info.allocationSize >= memory_reqs.size) {
                                result = vkAllocateMemory(logicalDevice, &alloc_info, nullptr, &new_memory);
                            }
                        }
                    }
                }
                // if still not allocated, try allocating private memory (if allowed)
                if (result != VK_SUCCESS && (private_memory)) {
                    result = allocatePrivateMemory(memory_reqs.size, memory_type_idx, dest_allocation, alloc_details.requiredFlags);
                    if (result == VK_SUCCESS) {
                        LOG_IF(VERBOSE_LOGGING, INFO) << "Allocation of memory succeeded";
                        return VK_SUCCESS;
                    }
                    else {
                        LOG(ERROR) << "Allocation of memory failed, after multiple attempts.";
                        return result;
                    }
                }

                MemoryBlock* new_block_ptr{ nullptr };
                size_t new_block_idx{ std::numeric_limits<size_t>::max() };

                {
                    std::lock_guard<std::mutex> new_block_guard(allocMutex);
                    std::unique_ptr<MemoryBlock> new_block = std::make_unique<MemoryBlock>(logicalDevice);
                    // need pointer to initialize child objects, but need to move unique_ptr into container.
                    new_block_idx = alloc_collection->AddMemoryBlock(std::move(new_block));
                }

                new_block_ptr = (*alloc_collection)[new_block_idx];
                // allocation size is more up-to-date than mem reqs size
                new_block_ptr->Init(new_memory, alloc_info.allocationSize);
                new_block_ptr->MemoryTypeIdx = memory_type_idx;

                SuballocationRequest request{ *new_block_ptr->avail_begin(), 0 };
                new_block_ptr->Allocate(request, type, memory_reqs.size);

                dest_allocation.Init(new_block_ptr, request.Offset, memory_reqs.alignment, memory_reqs.size);
                
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

                LOG_IF(VERBOSE_LOGGING, INFO) << "Created new allocation object w/ size of " << std::to_string(alloc_info.allocationSize * 1e-6) << "mb";
                return VK_SUCCESS;
            }
        }

    }

    VkResult AllocatorImpl::allocatePrivateMemory(const VkDeviceSize & size, const uint32_t & memory_type_idx, Allocation& dest_allocation, const VkMemoryPropertyFlags memory_flags) {
        VkMemoryAllocateInfo alloc_info{ VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO, nullptr, size, memory_type_idx };

        VkDeviceMemory private_memory_handle = VK_NULL_HANDLE;
        VkResult result = vkAllocateMemory(logicalDevice, &alloc_info, nullptr, &private_memory_handle);
        VkAssert(result);
        if (memory_flags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) {
            void* mapped_address = nullptr;
            result = vkMapMemory(logicalDevice, private_memory_handle, 0, alloc_info.allocationSize, 0, &mapped_address);
            VkAssert(result);
            dest_allocation.InitPrivate(memory_type_idx, private_memory_handle, true, mapped_address, size);
        }
        else {
            dest_allocation.InitPrivate(memory_type_idx, private_memory_handle, false, nullptr, size);
        }
        {
            std::lock_guard<std::mutex> private_guard(privateMutex);
            privateAllocations.insert(std::unique_ptr<Allocation>(&dest_allocation));
        }
        
        return VK_SUCCESS;
    }
    
    bool AllocatorImpl::freePrivateMemory(const Allocation * alloc_to_free) {

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
            vkFreeMemory(logicalDevice, private_handle, nullptr);
            privateAllocations.erase(iter);
            LOG_IF(VERBOSE_LOGGING, INFO) << "Freed a private memory allocation.";
            return true;
        }

        return false;
    }

    void AllocatorImpl::getBufferMemReqs(VkBuffer& handle, VkMemoryRequirements& reqs, bool& requires_dedicated, bool& prefers_dedicated) {
        if (!usingMemoryExtensions) {
            vkGetBufferMemoryRequirements(logicalDevice, handle, &reqs);
            requires_dedicated = false;
            prefers_dedicated = false;
            return;
        }
        else {
            VkBufferMemoryRequirementsInfo2KHR memory_reqs = vk_buffer_memory_requirements_info_khr_base;
            memory_reqs.buffer = handle;

            VkMemoryDedicatedRequirementsKHR ded_reqs = vk_dedicated_memory_requirements_khr_base;
            VkMemoryRequirements2KHR reqs2 = vk_memory_requirements_2_khr_base;
            reqs2.pNext = &ded_reqs;

            pVkGetBufferMemoryRequirements2KHR(logicalDevice, &memory_reqs, &reqs2);

            reqs = reqs2.memoryRequirements;
            prefers_dedicated= ded_reqs.prefersDedicatedAllocation;
            requires_dedicated = ded_reqs.requiresDedicatedAllocation;
            return;
        }
    }

    void AllocatorImpl::getImageMemReqs(VkImage& handle, VkMemoryRequirements& reqs, bool& requires_dedicated, bool& prefers_dedicated) {
        if (!usingMemoryExtensions) {
            vkGetImageMemoryRequirements(logicalDevice, handle, &reqs);
            requires_dedicated = false;
            prefers_dedicated = false;
            return;
        }
        else {
            VkImageMemoryRequirementsInfo2KHR memory_reqs = vk_image_memory_requirements_info_khr_base;
            memory_reqs.image = handle;

            VkMemoryDedicatedRequirementsKHR ded_reqs = vk_dedicated_memory_requirements_khr_base;
            VkMemoryRequirements2KHR reqs2 = vk_memory_requirements_2_khr_base;
            reqs2.pNext = &ded_reqs;

            pVkGetImageMemoryRequirements2KHR(logicalDevice, &memory_reqs, &reqs2);

            reqs = reqs2.memoryRequirements;
            requires_dedicated = ded_reqs.requiresDedicatedAllocation;
            prefers_dedicated= ded_reqs.prefersDedicatedAllocation;
            return;
        }
    }

 

    void AllocatorImpl::fetchAllocFunctionPointersKHR() {
        
        pVkGetBufferMemoryRequirements2KHR = reinterpret_cast<PFN_vkGetBufferMemoryRequirements2KHR>(vkGetDeviceProcAddr(logicalDevice,
            "vkGetBufferMemoryRequirements2KHR"));
        if (!pVkGetBufferMemoryRequirements2KHR) {
            usingMemoryExtensions = false;
            return;
        }

        pVkGetImageMemoryRequirements2KHR = reinterpret_cast<PFN_vkGetImageMemoryRequirements2KHR>(vkGetDeviceProcAddr(logicalDevice,
            "vkGetImageMemoryRequirements2KHR"));
        if (!pVkGetImageMemoryRequirements2KHR) {
            usingMemoryExtensions = false;
            return;
        }

        pVkGetImageSparseMemoryRequirements2KHR = reinterpret_cast<PFN_vkGetImageSparseMemoryRequirements2KHR>(vkGetDeviceProcAddr(logicalDevice,
            "vkGetImageSparseMemoryRequirements2KHR"));

        if (!pVkGetImageMemoryRequirements2KHR) {
            usingMemoryExtensions = false;
            return;
        }
    
    }

}
