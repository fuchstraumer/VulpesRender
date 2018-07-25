ADD_VPR_LIBRARY(vpr_resource
    "include/DescriptorPool.hpp"
    "include/DescriptorSet.hpp"
    "include/DescriptorSetLayout.hpp"
    "include/PipelineCache.hpp"
    "include/PipelineLayout.hpp"
    "include/Sampler.hpp"
    "include/ShaderModule.hpp"
    "src/DescriptorPool.cpp"
    "src/DescriptorSet.cpp"
    "src/DescriptorSetLayout.cpp"
    "src/PipelineCache.cpp"
    "src/PipelineLayout.cpp"
    "src/Sampler.cpp"
    "src/ShaderModule.cpp"
    "../third_party/easyloggingpp/src/easylogging++.cc"
)

IF(APPLE)
# Needed to support Boost filesystem
set(boost_min_ver 1.67.0)
set(boost_libs system filesystem)
find_package(Boost ${boost_min_ver})
if(Boost_FOUND)
    find_package(Boost ${boost_min_ver} COMPONENTS ${boost_libs})
endif()
ENDIF()

TARGET_INCLUDE_DIRECTORIES(vpr_resource PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
TARGET_INCLUDE_DIRECTORIES(vpr_resource PRIVATE "../alloc/include")

IF(APPLE)
TARGET_LINK_LIBRARIES(vpr_resource PRIVATE ${Boost_LIBRARIES})
TARGET_INCLUDE_DIRECTORIES(vpr_resource PRIVATE ${Boost_INCLUDE_DIRS} ${Boost_INCLUDE_DIR})
ENDIF()
