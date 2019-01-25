#include "vpr_stdafx.h"
#include "Allocation.hpp"
#include "MemoryBlock.hpp"
#include "easylogging++.h"
#include <nonstd/variant.hpp>
#include <limits>

namespace vpr {

    struct AllocationImpl {

        AllocationImpl() : typeData{ blockAllocation{} } {}

        struct blockAllocation {
            MemoryBlock* ParentBlock{ nullptr };
            VkDeviceSize Offset{ std::numeric_limits<VkDeviceSize>::max() };
        };

        struct privateAllocation {
            uint32_t MemoryTypeIdx{ std::numeric_limits<uint32_t>::max() };
            VkDeviceMemory DvcMemory{ VK_NULL_HANDLE };
            bool PersistentlyMapped{ false };
            void* MappedData{ nullptr };
        };

        nonstd::variant<blockAllocation, privateAllocation> typeData;
        void* userData{ nullptr };
    };

    Allocation::Allocation() : Size(0), Alignment(0), impl(std::make_unique<AllocationImpl>()) {}

    Allocation::~Allocation() { 
        impl.reset();
    }

    Allocation::Allocation(const Allocation& other) : Size(other.Size), Alignment(other.Alignment), impl(std::make_unique<AllocationImpl>(*other.impl)) {}

    Allocation& Allocation::operator=(const Allocation& other) {
        Size = other.Size;
        Alignment = other.Alignment;
        impl = std::make_unique<AllocationImpl>(*other.impl);
        return *this;
    }

    Allocation::Allocation(Allocation && other) noexcept : Size(std::move(other.Size)), Alignment(std::move(other.Alignment)),
        impl(std::move(other.impl)) {}

    Allocation &Allocation::operator=(Allocation && other) noexcept {
        Size = std::move(other.Size);
        Alignment = std::move(other.Alignment);
        impl = std::move(other.impl);
        return *this;
    }

    void Allocation::Init(MemoryBlock * parent_block, const VkDeviceSize & offset, const VkDeviceSize & alignment, const VkDeviceSize & alloc_size, void* user_data) {
        Size = alloc_size;
        Alignment = alignment;
        impl->typeData = AllocationImpl::blockAllocation{ parent_block, offset };
        impl->userData = user_data;
    }

    void Allocation::Update(MemoryBlock * new_parent_block, const VkDeviceSize & new_offset) {
        auto& alloc = nonstd::get<AllocationImpl::blockAllocation>(impl->typeData);
        alloc.ParentBlock = new_parent_block;
        alloc.Offset = new_offset;
    }

    void Allocation::InitPrivate(const uint32_t & type_idx, VkDeviceMemory & dvc_memory, bool persistently_mapped, void * mapped_data, const VkDeviceSize & data_size, void* user_data) {
        Size = data_size;
        AllocationImpl::privateAllocation p_alloc;
        p_alloc.DvcMemory = dvc_memory;
        p_alloc.MemoryTypeIdx = type_idx;
        p_alloc.PersistentlyMapped = persistently_mapped;
        p_alloc.MappedData = mapped_data;
        impl->typeData = std::move(p_alloc);
        impl->userData = user_data;
    }

    void Allocation::Map(const VkDeviceSize& size_to_map, const VkDeviceSize& offset_to_map_at, void** address_to_map_to) const {
        if (nonstd::holds_alternative<AllocationImpl::blockAllocation>(impl->typeData)) {
            nonstd::get<AllocationImpl::blockAllocation>(impl->typeData).ParentBlock->Map(this, size_to_map, offset_to_map_at, address_to_map_to);
        }
        else if (nonstd::holds_alternative<AllocationImpl::privateAllocation>(impl->typeData)) {
            auto& p_alloc = std::get<AllocationImpl::privateAllocation>(impl->typeData);
            if (p_alloc.PersistentlyMapped) {
                LOG_IF(VERBOSE_LOGGING, INFO) << "Attempted to map private allocation, setting given address_to_map_to to permanently mapped address.";
                *address_to_map_to = std::get<AllocationImpl::privateAllocation>(impl->typeData).MappedData;
            }
            else {
                throw std::runtime_error("Given private allocation is not mapped.");
            }
        }
    }

    void Allocation::Unmap() const noexcept {
        if (nonstd::holds_alternative<AllocationImpl::blockAllocation>(impl->typeData)) {
            nonstd::get<AllocationImpl::blockAllocation>(impl->typeData).ParentBlock->Unmap();
        }
    }

    const VkDeviceMemory& Allocation::Memory() const {
        if (nonstd::holds_alternative<AllocationImpl::blockAllocation>(impl->typeData)) {
            return nonstd::get<AllocationImpl::blockAllocation>(impl->typeData).ParentBlock->Memory();
        }
        else if (std::holds_alternative<AllocationImpl::privateAllocation>(impl->typeData)) {
            return nonstd::get<AllocationImpl::privateAllocation>(impl->typeData).DvcMemory;
        }
        else {
            throw std::runtime_error("Allocation had invalid type or was improperly initialized!");
        }
    }

    VkDeviceSize Allocation::Offset() const noexcept {
        if (nonstd::holds_alternative<AllocationImpl::blockAllocation>(impl->typeData)) {
            return nonstd::get<AllocationImpl::blockAllocation>(impl->typeData).Offset;
        }
        else {
            return VkDeviceSize(0);
        }
    }

    uint32_t Allocation::MemoryTypeIdx() const {
        if (nonstd::holds_alternative<AllocationImpl::blockAllocation>(impl->typeData)) {
            return nonstd::get<AllocationImpl::blockAllocation>(impl->typeData).ParentBlock->MemoryTypeIdx;
        }
        else if (std::holds_alternative<AllocationImpl::privateAllocation>(impl->typeData)) {
            return nonstd::get<AllocationImpl::privateAllocation>(impl->typeData).MemoryTypeIdx;
        }
        else {
            throw std::runtime_error("Allocation had invalid type, or was improperly initialized!");
        }
    }

    bool Allocation::IsPrivateAllocation() const noexcept {
        return nonstd::holds_alternative<AllocationImpl::privateAllocation>(impl->typeData);
    }

    bool Allocation::operator==(const Allocation & other) const noexcept {
        return (Memory() == other.Memory()) && (MemoryTypeIdx() == other.MemoryTypeIdx()) &&
            (Offset() == other.Offset()) && (Size == other.Size) && (Alignment == other.Alignment) &&
            (IsPrivateAllocation() == other.IsPrivateAllocation());
    }
    
}
