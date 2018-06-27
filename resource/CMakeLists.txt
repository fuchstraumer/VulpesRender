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
)

TARGET_INCLUDE_DIRECTORIES(vpr_resource PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
TARGET_INCLUDE_DIRECTORIES(vpr_resource PRIVATE "../alloc/include")
