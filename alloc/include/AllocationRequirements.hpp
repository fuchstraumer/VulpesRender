#pragma once
#ifndef VPR_ALLOCATION_REQUIREMENTS_HPP
#define VPR_ALLOCATION_REQUIREMENTS_HPP
#include "vpr_stdafx.h"

namespace vpr {

    /**This struct is the primary item submitted to allocator methods for resource creation.
    *  \ingroup Allocation
    */
    struct VPR_API AllocationRequirements {
        /** Defaults to false. If true, no new allocations are created beyond
        * the set created upon initilization of the allocator system. */
        inline static VkBool32 noNewAllocations = VK_FALSE;
        /** True if whatever allocation this belongs to should be in its own device memory object. Don't use this too often, of course. */
        VkBool32 privateMemory = false;
        /** The memory properties that are absolutely required by the item you are allocating for. */
        VkMemoryPropertyFlags requiredFlags;
        /** Additional flags that would be nice/useful to have, but are not required. An attempt to meet these will be
        *  made, but not meeting them won't be considered a failure.*/
        VkMemoryPropertyFlags preferredFlags = VkMemoryPropertyFlags(0);
        /**Set if this allocation benefits from a private/unique VkDeviceMemory instance, but doesn't require it.*/
        bool prefersDedicatedKHR = false;
        /**This allocation requires a unique VkDeviceMemory instance, and will fail if we're unable to meet this requirement.*/
        bool requiresDedicatedKHR = false;
    };

}

#endif //!VPR_ALLOCATION_REQUIREMENTS_HPP
