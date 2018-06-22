#pragma once
#ifndef VPR_SUBALLOCATION_HPP
#define VPR_SUBALLOCATION_HPP
#include "vpr_stdafx.h"
#include "ForwardDecl.hpp"
#include "AllocCommon.hpp"
#include <list>
#include <vector>

namespace vpr {
    
    /**During the process of finding a suitable region to bind to, we need to store things like "SuballocationType",
     * which helps keep track of the precise kind of memory we are looking for and lets us know if its optimally/linearly tiled.
     * \ingroup Allocation
     */ 
    struct Suballocation {
        VkDeviceSize Offset{ std::numeric_limits<VkDeviceSize>::max() };
        VkDeviceSize Size{ std::numeric_limits<VkDeviceSize>::max() };
        SuballocationType Type{ SuballocationType::Unknown };
        bool operator<(const Suballocation& other) const noexcept {
            return Offset < other.Offset;
        }
    };

    struct suballocOffsetCompare {
        bool operator()(const Suballocation& s0, const Suballocation& s1) const noexcept {
            return s0.Offset < s1.Offset; // true when s0 is before s1
        }
    };

    using suballocationList = std::list<Suballocation>;

    /**Represents an in-progress allocation that we will shortly attempt to assign a slot to.
     * \ingroup Allocation
     */
    struct SuballocationRequest {
        suballocationList::iterator FreeSuballocation; // location of suballoc this request can use.
        VkDeviceSize Offset{ 0 };
    };


    struct suballocIterCompare {
        bool operator()(const suballocationList::iterator& iter0, const suballocationList::iterator& iter1) const noexcept {
            return iter0->Size < iter1->Size;
        }
    };

    using avail_suballocation_iterator_t = std::vector<suballocationList::iterator>::iterator;
    using const_avail_suballocation_iterator_t = std::vector<suballocationList::iterator>::const_iterator;
    using suballocation_iterator_t = suballocationList::iterator;
    using const_suballocation_iterator_t = suballocationList::const_iterator;

    

}

#endif //!VPR_SUBALLOCATION_HPP
