#pragma once
#ifndef VPR_ALLOC_2_ALLOCATION_STRUCT_HPP
#define VPR_ALLOC_2_ALLOCATION_STRUCT_HPP
#include "vpr_stdafx.h"

namespace vpr {

    /*
        \struct Allocation
        \brief Represents a singular memory allocation

        Depending on the creation parameters and requirements, this object may be a singular object or it 
        could be a suballocation in a larger meta-object. Unlike previous versions of this library, the 
        Allocations themselves are no longer objects that can be retrieved and stored by clients. If a client
        wants to access or use an Allocation, then they have to retrieve the #AllocationInfo structure
        and read the data they need from this structure. This also means that retrieving up-to-date data
        about the Allocation is left up to the user.

        \ingroup Allocation
    */
    struct AllocationImpl;
    using Allocation = AllocationImpl*;

    /*
        \brief AllocationInfo is how users will interact with and retrieve state information about the allocations they have created

        \ingroup Allocation
    */
    struct AllocationInfo {
        /* \brief Memory type index that this allocation belongs to */
        uint32_t MemoryTypeIdx{ 0u };
        /* 
            \brief Handle to underlying Vulkan memory object.
            It is likely that several allocations will share a single object, so keep this in mind. It usually won't change, unless
            this allocation is included in a defragmentation routine or if it's a "lost" allocation (assuming the relevant flags were set).
        */
        VkDeviceMemory DeviceMemory{ VK_NULL_HANDLE };
        /*
            \brief Offset into DeviceMemory where this allocation is located (in bytes)
            It should not change, unless the allocation is lost or relocated during defragmentation.
        */
        VkDeviceSize Offset{ 0u };
        /*
            \brief Size of the allocation in bytes
            Note that this will likely by different from the requested size: alignment and sizing requirements will most likely increase it.
        */
        VkDeviceSize Size{ 0u };
        /*
            \brief Pointer to the beginning of this allocation as CPU-mapped data
            If this allocation hasn't been mapped or hasn't been created with #ALLOCATION_CREATE_PERSISTENTLY_MAPPED_BIT, it will
            be `nullptr`. It can change during calls to map/unmap, and if the allocation has been defragmented.
        */
        void* MappedData{ nullptr };
        /*
            \brief General-purpose pointer for users to use as they please.
            If the allocation was created with #ALLOCATION_USER_DATA_AS_STRING_BIT, this field is interpreted as a C-string. A copy of this
            data will be made and stored (not just the pointer). Otherwise, the underlying contents and their structure are assumed as unknowns.
        */
        void* UserData{ nullptr };
    };

}

#endif //!VPR_ALLOC_2_ALLOCATION_STRUCT_HPP
