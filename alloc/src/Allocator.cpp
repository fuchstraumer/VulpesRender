#include "vpr_stdafx.h"
#include "Allocator.hpp"
#include "AllocatorImpl.hpp"
#include "easylogging++.h"

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

    Allocator::Allocator(const VkDevice& logical_device, const VkPhysicalDevice& physical_device, allocation_extensions dedicated_alloc_enabled) : impl(std::make_unique<AllocatorImpl>(logical_device, physical_device, dedicated_alloc_enabled)) {}

    Allocator::~Allocator() {
        impl->clearAllocationMaps();
    }

    void Allocator::Recreate() {
        impl->clearAllocationMaps();
        impl->createAllocationMaps();
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
        if (impl->freePrivateMemory(*memory_to_free)) {
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
    
}
