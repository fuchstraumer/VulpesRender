ADD_VPR_LIBRARY(
    vpr_alloc
    "include/Allocation.hpp" 
    "include/AllocationRequirements.hpp"
    "include/Allocator.hpp"
    "src/Allocation.cpp"
    "src/AllocationCollection.hpp"
    "src/AllocationCollection.cpp"
    "src/Allocator.cpp"
    "src/AllocatorImpl.hpp"
    "src/AllocatorImpl.cpp"
    "src/AllocCommon.hpp"
    "src/MemoryBlock.hpp"
    "src/MemoryBlock.cpp"
    "src/Suballocation.hpp"
    "../third_party/easyloggingpp/src/easylogging++.cc"
)

TARGET_INCLUDE_DIRECTORIES(vpr_alloc PUBLIC 
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

IF(APPLE)
# Needed to support Boost variant
set(boost_min_ver 1.67.0)
find_package(Boost ${boost_min_ver})
ENDIF()

IF(APPLE)
TARGET_INCLUDE_DIRECTORIES(vpr_alloc PRIVATE ${Boost_INCLUDE_DIRS} ${Boost_INCLUDE_DIR})
ENDIF()
