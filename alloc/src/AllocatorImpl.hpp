#pragma once
#ifndef VPR_ALLOCATOR_IMPL_HPP
#define VPR_ALLOCATOR_IMPL_HPP
#include "Allocator.hpp"
#include "AllocCommon.hpp"
#include "AllocationCollection.hpp"
#include "Allocation.hpp"
#include "AllocationRequirements.hpp"
#include "MemoryBlock.hpp"
#include "vkAssert.hpp"
#include "CreateInfoBase.hpp"
#include <vulkan/vulkan.h>
#include <unordered_set>
#include <map>
#include <vector>

namespace vpr {

    struct AllocationHash {
        size_t operator()(const Allocation& alloc) const noexcept {
            return (std::hash<uint64_t>()((uint64_t)alloc.Memory()) << 32) ^ (std::hash<VkDeviceSize>()(alloc.Offset()) << 16) ^ (std::hash<VkDeviceSize>()(alloc.Size));
        }
    };

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
        bool freePrivateMemory(const Allocation& memory_to_free);

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
        std::unordered_set<Allocation, AllocationHash> privateAllocations;

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

        friend class DebugVisualization;
        friend struct DebugVisualizationImpl;
    };

    struct raw_equal_comparator {
        raw_equal_comparator(const Allocation& _ptr) : ptr(_ptr) {};
        bool operator()(const Allocation& other) const {
            return ptr == other;
        }
        const Allocation& ptr;
    };

}

#endif //!VPR_ALLOCATOR_IMPL_HPP
