ADD_VPR_LIBRARY(vpr_sync
    "include/Fence.hpp"
    "include/Event.hpp"
    "include/Semaphore.hpp"
    "src/Fence.cpp"
    "src/Event.cpp"
    "src/Semaphore.cpp")

TARGET_INCLUDE_DIRECTORIES(vpr_sync PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
INSTALL(DIRECTORY "include/" DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/vpr/")