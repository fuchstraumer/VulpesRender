#pragma once
#ifndef VPR_ALLOCATION_HPP
#define VPR_ALLOCATION_HPP
#include "vpr_stdafx.h"
#include "ForwardDecl.hpp"
#include <variant>

namespace vpr {
    
    /**    
    *    Allocation class represents a singular allocation: can be a private allocation (i.e, only user
    *    of attached DeviceMemory) or a block allocation (bound to sub-region of device memory)
    *   \ingroup Allocator
    */
    class VPR_API Allocation {
        Allocation(const Allocation&) = delete;
        Allocation& operator=(const Allocation&) = delete;
    public:
        /** If this is an allocation bound to a smaller region of a larger object, it is a block allocation. 
         *  Otherwise, it has it's own VkDeviceMemory object and is a "PRIVATE_ALLOCATION" type.
        */
        Allocation() = default;
        ~Allocation() = default;
        Allocation(Allocation&& other) noexcept;
        Allocation& operator=(Allocation&& other) noexcept;

        void Init(MemoryBlock* parent_block, const VkDeviceSize& offset, const VkDeviceSize& alignment, const VkDeviceSize& alloc_size, void* user_data = nullptr);
        void Update(MemoryBlock* new_parent_block, const VkDeviceSize& new_offset);
        /** \param persistently_mapped: If set, this object will be considered to be always mapped. This will remove any worries about mapping/unmapping the object. */
        void InitPrivate(const uint32_t& type_idx, VkDeviceMemory& dvc_memory, bool persistently_mapped, void* mapped_data, const VkDeviceSize& data_size, void* user_data = nullptr);
        void Map(const VkDeviceSize& size_to_map, const VkDeviceSize& offset_to_map_at, void* address_to_map_to) const;
        void Unmap() const noexcept;

        const VkDeviceMemory& Memory() const;
        VkDeviceSize Offset() const noexcept;
        uint32_t MemoryTypeIdx() const;
        void* GetUserData() const;
        bool IsPrivateAllocation() const noexcept;

        VkDeviceSize Size{ 0 };
        VkDeviceSize Alignment{ 0 };
    private:

        struct blockAllocation {
            MemoryBlock* ParentBlock;
            VkDeviceSize Offset;
        };

        struct privateAllocation {
            uint32_t MemoryTypeIdx;
            VkDeviceMemory DvcMemory;
            bool PersistentlyMapped;
            void* MappedData;
        };

        void* userData = nullptr;
        std::variant<blockAllocation, privateAllocation> typeData;
    };

}

#endif