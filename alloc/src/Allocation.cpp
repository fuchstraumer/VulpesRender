#include "vpr_stdafx.h"
#include "Allocation.hpp"
#include "MemoryBlock.hpp"
#include "easylogging++.h"

namespace vpr {

    Allocation::Allocation(Allocation && other) noexcept : Size(std::move(other.Size)), Alignment(std::move(other.Alignment)),
        typeData(std::move(other.typeData)) {}

    Allocation &Allocation::operator=(Allocation && other) noexcept {
        Size = std::move(other.Size);
        Alignment = std::move(other.Alignment);
        typeData = std::move(other.typeData);
        return *this;
    }

    void Allocation::Init(MemoryBlock * parent_block, const VkDeviceSize & offset, const VkDeviceSize & alignment, const VkDeviceSize & alloc_size, void* user_data) {
        blockAllocation alloc;
        alloc.ParentBlock = parent_block;
        alloc.Offset = offset;
        Size = alloc_size;
        Alignment = alignment;
        typeData = alloc;
    }

    void Allocation::Update(MemoryBlock * new_parent_block, const VkDeviceSize & new_offset) {
        auto& alloc = std::get<blockAllocation>(typeData);
        alloc.ParentBlock = new_parent_block;
        alloc.Offset = new_offset;
    }

    void Allocation::InitPrivate(const uint32_t & type_idx, VkDeviceMemory & dvc_memory, bool persistently_mapped, void * mapped_data, const VkDeviceSize & data_size, void* user_data) {
        Size = data_size;
        privateAllocation p_alloc;
        p_alloc.DvcMemory = dvc_memory;
        p_alloc.MemoryTypeIdx = type_idx;
        p_alloc.PersistentlyMapped = persistently_mapped;
        p_alloc.MappedData = mapped_data;
        typeData = p_alloc;
    }

    void Allocation::Map(const VkDeviceSize& size_to_map, const VkDeviceSize& offset_to_map_at, void* address_to_map_to) const {
        if (std::holds_alternative<blockAllocation>(typeData)) {
            std::get<blockAllocation>(typeData).ParentBlock->Map(this, size_to_map, offset_to_map_at, address_to_map_to);
        }
        else if (std::holds_alternative<privateAllocation>(typeData)) {
            LOG_IF(VERBOSE_LOGGING,INFO) << "Attempted to map private allocation, setting given address_to_map_to to permanently mapped address.";
            address_to_map_to = std::get<privateAllocation>(typeData).MappedData;
        }
    }

    void Allocation::Unmap() const noexcept {
        if (std::holds_alternative<blockAllocation>(typeData)) {
            std::get<blockAllocation>(typeData).ParentBlock->Unmap();
        }
    }

    const VkDeviceMemory & Allocation::Memory() const {
        if (std::holds_alternative<blockAllocation>(typeData)) {
            return std::get<blockAllocation>(typeData).ParentBlock->Memory();
        }
        else if (std::holds_alternative<privateAllocation>(typeData)) {
            return std::get<privateAllocation>(typeData).DvcMemory;
        }
        else {
            throw std::runtime_error("Allocation had invalid type or was improperly initialized!");
        }
    }

    VkDeviceSize Allocation::Offset() const noexcept {
        if (std::holds_alternative<blockAllocation>(typeData)) {
            return std::get<blockAllocation>(typeData).Offset;
        }
        else {
            return VkDeviceSize(0);
        }
    }

    uint32_t Allocation::MemoryTypeIdx() const {
        if (std::holds_alternative<blockAllocation>(typeData)) {
            return std::get<blockAllocation>(typeData).ParentBlock->MemoryTypeIdx;
        }
        else if (std::holds_alternative<privateAllocation>(typeData)) {
            return std::get<privateAllocation>(typeData).MemoryTypeIdx;
        }
        else {
            throw std::runtime_error("Allocation had invalid type, or was improperly initialized!");
        }
    }

    bool Allocation::IsPrivateAllocation() const noexcept {
        return std::holds_alternative<privateAllocation>(typeData);
    }
    
}
