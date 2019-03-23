#pragma once
#ifndef VPR_ALLOC2_ALLOCATION_IMPL_HPP
#define VPR_ALLOC2_ALLOCATION_IMPL_HPP
#include "vpr_stdafx.h"
#include "Allocator.hpp"
#include "Common.hpp"
#include <nonstd/variant.hpp>
#include <atomic>

namespace vpr {

    class DeviceMemoryBlock;

    struct BlockAllocationImpl {
        constexpr BlockAllocationImpl() = default;
        ~BlockAllocationImpl() = default;
        DeviceMemoryBlock* ParentBlock{ nullptr };
        VkDeviceSize Offset{ std::numeric_limits<VkDeviceSize>::max() };
    };

    struct PrivateAllocationImpl {
        PrivateAllocationImpl() noexcept = default;
        uint32_t memoryTypeIdx{ std::numeric_limits<uint32_t>::max() };
        VkDeviceMemory DeviceMemory{ VK_NULL_HANDLE };
        void* mappedData{ nullptr };
    };

    struct AllocationImpl {

        AllocationImpl() noexcept = default;

        void Create(uint32_t curr_frame_idx, bool user_data_string);
        void Destroy();

        void CreateBlockAllocation(DeviceMemoryBlock* block, VkDeviceSize offset, VkDeviceSize alignment, VkDeviceSize size,
            SuballocationType type, bool mapped, bool can_become_lost);
        void CreateDedicatedAllocation(uint32_t type_index, VkDeviceMemory memory_handle, SuballocationType type, void* mapped_data,
            VkDeviceSize size);
        void CreateLostAllocation() noexcept;
        void UpdateBlockAllocation(Allocator alloc, DeviceMemoryBlock* block, VkDeviceSize offset);
        void Resize(VkDeviceSize size);
        void MoveOffset(VkDeviceSize size);

        bool IsBlockAllocation() const noexcept;
        bool IsDedicatedAllocation() const noexcept;
        bool IsPersistentlyMapped() const noexcept;

        void SetUserData(Allocator alloc_handle, void* user_data);
        void RetrieveDedicatedAllocStats(MemoryPoolStats& output) const noexcept;

        VkResult Map(const VkDeviceSize& size_to_map, const VkDeviceSize offset_to_map_at, void** address_to_map);
        VkResult Unmap();

        void SetBufferImageUsage(uint32_t bits);
        void PrintParamsToJSON(AllocatorJsonWriter* writer) const;
        void FreeUserDataString(Allocator alloc_handle);

        constexpr inline static uint8_t MAPPING_FLAG_PERSISTENTLY_MAPPED{ 0x80 };
        constexpr inline static uint8_t USER_DATA_AS_STRING_FLAG_BITS{ 0x01 };
        VkDeviceSize size{ std::numeric_limits<VkDeviceSize>::max() };
        VkDeviceSize alignment{ 0u };
        void* userData{ nullptr };
        uint8_t mappingFlags{ 0u };
        uint8_t suballocationType{ 0u };
        uint8_t flags{ 0u };        
        bool canBeLost{ false };
        nonstd::variant<BlockAllocationImpl, PrivateAllocationImpl> data;
        uint32_t creationFrameIdx{ 0u };
        uint32_t bufferImageUsage{ 0u };

    };

}

#endif //!VPR_ALLOC2_ALLOCATION_IMPL_HPP
