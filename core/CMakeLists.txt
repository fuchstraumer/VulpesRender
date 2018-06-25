ADD_VPR_LIBRARY(vpr_core
    "include/Instance.hpp"
    "include/LogicalDevice.hpp"
    "include/PhysicalDevice.hpp"
    "include/SurfaceKHR.hpp"
    "include/Swapchain.hpp"
    "src/Instance.cpp"
    "src/LogicalDevice.cpp"
    "src/PhysicalDevice.cpp"
    "src/SurfaceKHR.cpp"
    "src/Swapchain.cpp"
)

TARGET_INCLUDE_DIRECTORIES(vpr_core PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
TARGET_INCLUDE_DIRECTORIES(vpr_core PRIVATE "../third_party/glfw/include")
INSTALL(DIRECTORY "include/" DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/vpr/")

IF(BUILD_DYNAMIC_LIBRARY)
    TARGET_LINK_LIBRARIES(vpr_core PRIVATE glfw)
ELSE()
    TARGET_LINK_LIBRARIES(vpr_core INTERFACE glfw)
ENDIF()
