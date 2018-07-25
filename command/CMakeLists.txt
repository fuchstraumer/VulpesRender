ADD_VPR_LIBRARY(vpr_command
    "include/CommandPool.hpp"
    "src/CommandPool.cpp"
    "../third_party/easyloggingpp/src/easylogging++.cc"
)

TARGET_INCLUDE_DIRECTORIES(vpr_command PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
