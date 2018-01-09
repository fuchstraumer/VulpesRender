#pragma once
#ifndef VPR_SUBALLOCATION_HPP
#define VPR_SUBALLOCATION_HPP
#include "vpr_stdafx.h"
#include "ForwardDecl.hpp"
#include "AllocCommon.hpp"
#include <list>

namespace vpr {
    
    /**During the process of finding a suitable region to bind to, we need to store things like "SuballocationType",
     * which helps keep track of the precise kind of memory we are looking for and lets us know if its optimally/linearly tiled.
     * \ingroup Allocation
     */ 
    struct Suballocation {
        bool operator<(const Suballocation& other) {
            return offset < other.offset;
        }
        VkDeviceSize offset, size;
        SuballocationType type;
    };

    struct suballocOffsetCompare {
        bool operator()(const Suballocation& s0, const Suballocation& s1) const {
            return s0.offset < s1.offset; // true when s0 is before s1
        }
    };

    using suballocationList = std::list<Suballocation>;

    struct SuballocationRequest {
        suballocationList::iterator freeSuballocation; // location of suballoc this request can use.
        VkDeviceSize offset;
    };


    struct suballocIterCompare {
        bool operator()(const suballocationList::iterator& iter0, const suballocationList::iterator& iter1) const {
            return iter0->size < iter1->size;
        }
    };

    using avail_suballocation_iterator_t = std::vector<suballocationList::iterator>::iterator;
    using const_avail_suballocation_iterator_t = std::vector<suballocationList::iterator>::const_iterator;
    using suballocation_iterator_t = suballocationList::iterator;
    using const_suballocation_iterator_t = suballocationList::const_iterator;

    

}

#endif //!VPR_SUBALLOCATION_HPP