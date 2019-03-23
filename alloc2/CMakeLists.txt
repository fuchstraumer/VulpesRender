
ADD_VPR_LIBRARY(
    vpr_alloc2
    "include/Allocation.hpp"
    "include/AllocationTypes.hpp"
    "include/Allocator.hpp"
    "include/Defrag.hpp"
    "src/impl/AllocationImpl.hpp"
    "src/impl/AllocationImpl.cpp"
    "src/impl/AllocatorImpl.hpp"
    "src/impl/AllocatorImpl.cpp"
    "src/impl/Common.hpp"
    "src/impl/MemoryBlock.hpp"
    "src/impl/MemoryBlock.cpp"
    "src/impl/MemoryBlockTechniques.hpp"
    "src/impl/MemoryBlockTechniques.cpp"
    "src/impl/vprMutex.hpp"
    "src/Allocator.cpp"
    "src/Config.hpp"
)

TARGET_INCLUDE_DIRECTORIES(vpr_alloc2 PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/../third_party/variant_lite/include"    
)
