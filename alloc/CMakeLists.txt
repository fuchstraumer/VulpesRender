ADD_VPR_LIBRARY(
    vpr_alloc
    "include/Allocation.hpp" 
    "include/AllocationRequirements.hpp"
    "include/Allocator.hpp"
    "src/Allocation.cpp"
    "src/AllocationCollection.hpp"
    "src/AllocationCollection.cpp"
    "src/Allocator.cpp"
    "src/AllocCommon.hpp"
    "src/MemoryBlock.hpp"
    "src/MemoryBlock.cpp"
    "src/Suballocation.hpp"
    "../third_party/easyloggingpp/src/easylogging++.cc"
)

TARGET_INCLUDE_DIRECTORIES(vpr_alloc PUBLIC 
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

