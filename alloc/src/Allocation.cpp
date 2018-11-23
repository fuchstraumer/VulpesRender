#include "vpr_stdafx.h"
#include "Allocation.hpp"
#include "MemoryBlock.hpp"
#include "easylogging++.h"
#if defined(__APPLE_CC__) || defined(__linux__)
#include <boost/variant.hpp>
#else
#include <variant>
#endif
#include <limits>

namespace vpr {

    struct AllocationImpl {

        AllocationImpl() = default;
        AllocationImpl(const AllocationImpl& other) : typeData(other.typeData), userData(other.userData) {}

        struct blockAllocation {
            blockAllocation() : ParentBlock{ nullptr }, Offset{ std::numeric_limits<VkDeviceSize>::max() } {}
            MemoryBlock* ParentBlock;
            VkDeviceSize Offset;
        };

        struct privateAllocation {
            uint32_t MemoryTypeIdx;
            VkDeviceMemory DvcMemory;
            bool PersistentlyMapped;
            void* MappedData;
        };
#if defined(__APPLE_CC__) || defined(__linux__)
        boost::variant<blockAllocation, privateAllocation> typeData;
#else
        std::variant<blockAllocation, privateAllocation> typeData;
#endif
        void* userData;
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
        AllocationImpl::blockAllocation alloc;
        alloc.ParentBlock = parent_block;
        alloc.Offset = offset;
        Size = alloc_size;
        Alignment = alignment;
        impl->typeData = std::move(alloc);
        impl->userData = user_data;
    }

    void Allocation::Update(MemoryBlock * new_parent_block, const VkDeviceSize & new_offset) {
#if defined(__APPLE_CC__) || defined(__linux__)
        auto& alloc = boost::get<AllocationImpl::blockAllocation>(impl->typeData);
#else
        auto& alloc = std::get<AllocationImpl::blockAllocation>(impl->typeData);
#endif
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
#if defined(__APPLE_CC__) || defined(__linux__)
        auto map_private_alloc = [&](AllocationImpl::privateAllocation& alloc) {
            if (alloc.PersistentlyMapped) {
                *address_to_map_to = alloc.MappedData;
            }
            else {
                throw std::runtime_error("Private allocation cannot be mapped!");
            }
        };
        switch (impl->typeData.which()) {
        case 0:
            boost::get<AllocationImpl::blockAllocation>(impl->typeData).ParentBlock->Map(this, size_to_map, offset_to_map_at, address_to_map_to);
            break;
        case 1:
            map_private_alloc(boost::get<AllocationImpl::privateAllocation>(impl->typeData));
            break;
        };
#else
        if (std::holds_alternative<AllocationImpl::blockAllocation>(impl->typeData)) {
            std::get<AllocationImpl::blockAllocation>(impl->typeData).ParentBlock->Map(this, size_to_map, offset_to_map_at, address_to_map_to);
        }
        else if (std::holds_alternative<AllocationImpl::privateAllocation>(impl->typeData)) {
            auto& p_alloc = std::get<AllocationImpl::privateAllocation>(impl->typeData);
            if (p_alloc.PersistentlyMapped) {
                LOG_IF(VERBOSE_LOGGING, INFO) << "Attempted to map private allocation, setting given address_to_map_to to permanently mapped address.";
                *address_to_map_to = std::get<AllocationImpl::privateAllocation>(impl->typeData).MappedData;
            }
            else {
                throw std::runtime_error("Given private allocation is not mapped.");
            }
        }
#endif
    }

    void Allocation::Unmap() const noexcept {
#if defined(__APPLE_CC__) || defined(__linux__)
        switch (impl->typeData.which()) {
        case 0:
            boost::get<AllocationImpl::blockAllocation>(impl->typeData).ParentBlock->Unmap();
            break;
        default:
            break;
        };
#else
        if (std::holds_alternative<AllocationImpl::blockAllocation>(impl->typeData)) {
            std::get<AllocationImpl::blockAllocation>(impl->typeData).ParentBlock->Unmap();
        }
#endif
    }

    const VkDeviceMemory& Allocation::Memory() const {
#if defined(__APPLE_CC__) || defined(__linux__)
        switch (impl->typeData.which()) {
        case 0:
            return boost::get<AllocationImpl::blockAllocation>(impl->typeData).ParentBlock->Memory();
        case 1:
            return boost::get<AllocationImpl::privateAllocation>(impl->typeData).DvcMemory;
        default:
            throw std::runtime_error("Allocation had invalid type!");
        };
#else
        if (std::holds_alternative<AllocationImpl::blockAllocation>(impl->typeData)) {
            return std::get<AllocationImpl::blockAllocation>(impl->typeData).ParentBlock->Memory();
        }
        else if (std::holds_alternative<AllocationImpl::privateAllocation>(impl->typeData)) {
            return std::get<AllocationImpl::privateAllocation>(impl->typeData).DvcMemory;
        }
        else {
            throw std::runtime_error("Allocation had invalid type or was improperly initialized!");
        }
#endif
    }

    VkDeviceSize Allocation::Offset() const noexcept {
#if defined(__APPLE_CC__) || defined(__linux__)
        switch (impl->typeData.which()) {
        case 0:
            return boost::get<AllocationImpl::blockAllocation>(impl->typeData).Offset;
        case 1:
            return VkDeviceSize(0);
        default:
            throw std::runtime_error("Allocation had invalid type!");
        };
#else
        if (std::holds_alternative<AllocationImpl::blockAllocation>(impl->typeData)) {
            return std::get<AllocationImpl::blockAllocation>(impl->typeData).Offset;
        }
        else {
            return VkDeviceSize(0);
        }
#endif
    }

    uint32_t Allocation::MemoryTypeIdx() const {
#if defined(__APPLE_CC__) || defined(__linux__)
        switch (impl->typeData.which()) {
        case 0:
            return boost::get<AllocationImpl::blockAllocation>(impl->typeData).ParentBlock->MemoryTypeIdx;
        case 1:
            return boost::get<AllocationImpl::privateAllocation>(impl->typeData).MemoryTypeIdx;
        default:
            throw std::runtime_error("Allocation had invalid type!");
        };
#else
        if (std::holds_alternative<AllocationImpl::blockAllocation>(impl->typeData)) {
            return std::get<AllocationImpl::blockAllocation>(impl->typeData).ParentBlock->MemoryTypeIdx;
        }
        else if (std::holds_alternative<AllocationImpl::privateAllocation>(impl->typeData)) {
            return std::get<AllocationImpl::privateAllocation>(impl->typeData).MemoryTypeIdx;
        }
        else {
            throw std::runtime_error("Allocation had invalid type, or was improperly initialized!");
        }
#endif
    }

    bool Allocation::IsPrivateAllocation() const noexcept {
#if defined(__APPLE_CC__) || defined(__linux__)
        switch(impl->typeData.which()){
        case 0:
            return false;
        case 1:
            return true;
        default:
            return false;
        };
#else
        return std::holds_alternative<AllocationImpl::privateAllocation>(impl->typeData);
#endif
    }

    bool Allocation::operator==(const Allocation & other) const noexcept {
        return (Memory() == other.Memory()) && (MemoryTypeIdx() == other.MemoryTypeIdx()) &&
            (Offset() == other.Offset()) && (Size == other.Size) && (Alignment == other.Alignment) &&
            (IsPrivateAllocation() == other.IsPrivateAllocation());
    }
    
}
