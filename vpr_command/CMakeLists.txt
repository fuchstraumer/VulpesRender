ADD_VPR_LIBRARY(vpr_command
    "CommandPool.hpp"
    "CommandPool.cpp")

TARGET_INCLUDE_DIRECTORIES(vpr_command PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
INSTALL(FILES "CommandPool.hpp" DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/vpr/")