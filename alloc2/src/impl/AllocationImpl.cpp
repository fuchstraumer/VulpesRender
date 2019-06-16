#include "AllocationImpl.hpp"
#include <cassert>

namespace vpr {

    void AllocationImpl::Create(uint32_t curr_frame_idx, bool user_data_string)
    {
        alignment = 1u;
        size = 0u;
        lastFrameUsed = curr_frame_idx;
        suballocationType = SuballocationType::SuballocTypeUnknown;
        flags = user_data_string ? USER_DATA_AS_STRING_FLAG_BITS : 0u;
        creationFrameIdx = curr_frame_idx;
        bufferImageUsage = 0u;
    }

    void AllocationImpl::Destroy() 
    {
        assert(userData == nullptr);
        assert((mappingFlags & MAPPING_FLAG_PERSISTENTLY_MAPPED) == 0);
    }

    void AllocationImpl::CreateBlockAllocation(DeviceMemoryBlock * block, VkDeviceSize _offset, VkDeviceSize _alignment, VkDeviceSize _size, SuballocationType _type, bool mapped, bool can_become_lost)
    {
        data = BlockAllocationImpl();
        BlockAllocationImpl& data_impl = nonstd::get<BlockAllocationImpl>(data);
        data_impl.Offset = _offset;
        alignment = _alignment;
        size = _size;
        mappingFlags = mapped ? MAPPING_FLAG_PERSISTENTLY_MAPPED : 0u;
        suballocationType = _type;
        data_impl.ParentBlock = block;
        canBeLost = can_become_lost;
    }

    void AllocationImpl::CreateDedicatedAllocation(uint32_t type_index, VkDeviceMemory memory_handle, SuballocationType type, void * mapped_data, VkDeviceSize size)
    {
        data = PrivateAllocationImpl();
        PrivateAllocationImpl& data_impl = nonstd::get<PrivateAllocationImpl>(data);
        data_impl.memoryTypeIdx = type_index;
        data_impl.DeviceMemory = memory_handle;
        data_impl.mappedData = mapped_data;
        size = size;
        alignment = 1;
        mappingFlags = (mapped_data == nullptr) ? 0u : MAPPING_FLAG_PERSISTENTLY_MAPPED;
        suballocationType = type;
    }

    void AllocationImpl::CreateLostAllocation() noexcept
    {
        data = BlockAllocationImpl();
        auto& data_impl = nonstd::get<BlockAllocationImpl>(data);
        data_impl.ParentBlock = nullptr;
        canBeLost = true;
    }

    void AllocationImpl::UpdateBlockAllocation(Allocator alloc, DeviceMemoryBlock * block, VkDeviceSize offset)
    {
        assert(IsBlockAllocation());
        auto& data_impl = nonstd::get<BlockAllocationImpl>(data);
        if (data_impl.ParentBlock != block) 
        {
            uint32_t map_ref_count = static_cast<uint32_t>(mappingFlags & ~MAPPING_FLAG_PERSISTENTLY_MAPPED);
            if (IsPersistentlyMapped())
            {
                ++map_ref_count;
            }

        }
    }

    void AllocationImpl::Resize(VkDeviceSize size)
    {
    }

    void AllocationImpl::MoveOffset(VkDeviceSize size)
    {
    }

    bool AllocationImpl::IsBlockAllocation() const noexcept
    {
    }

    bool AllocationImpl::IsDedicatedAllocation() const noexcept
    {
    }

    void AllocationImpl::SetUserData(Allocator alloc_handle, void * user_data)
    {
    }

    bool AllocationImpl::CmpExchangeLastFrameUsedIdx(uint32_t & expected, uint32_t desired) noexcept
    {
        return false;
    }

    bool AllocationImpl::ForceLost(uint32_t frame_idx, uint32_t frame_in_use_count)
    {
        return false;
    }

    void AllocationImpl::RetrieveDedicatedAllocStats(MemoryPoolStats & output) const noexcept
    {
    }

    VkResult AllocationImpl::Map(const VkDeviceSize & size_to_map, const VkDeviceSize offset_to_map_at, void ** address_to_map)
    {
        return VkResult();
    }

    VkResult AllocationImpl::Unmap()
    {
        return VkResult();
    }

    void AllocationImpl::SetBufferImageUsage(uint32_t bits)
    {
    }

    void AllocationImpl::PrintParamsToJSON(AllocatorJsonWriter * writer) const
    {
    }

    void AllocationImpl::FreeUserDataString(Allocator alloc_handle)
    {
    }

}
