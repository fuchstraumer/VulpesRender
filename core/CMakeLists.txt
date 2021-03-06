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
    "../third_party/easyloggingpp/src/easylogging++.cc"
)

TARGET_INCLUDE_DIRECTORIES(vpr_core PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")

IF(BUILD_DYNAMIC_LIBRARY AND NOT VPR_BUILD_ANDROID)
    IF(VPR_USE_SDL)
        TARGET_LINK_LIBRARIES(vpr_core PRIVATE SDL2)
    ELSE()
        TARGET_INCLUDE_DIRECTORIES(vpr_core PRIVATE "../third_party/glfw/include")
        TARGET_LINK_LIBRARIES(vpr_core PRIVATE glfw)
    ENDIF()
ELSEIF(NOT VPR_BUILD_ANDROID)
    IF(VPR_USE_SDL)
        TARGET_LINK_LIBRARIES(vpr_core INTERFACE SDL2)
    ELSE()
        TARGET_INCLUDE_DIRECTORIES(vpr_core PUBLIC "../third_party/glfw/include")
        TARGET_LINK_LIBRARIES(vpr_core INTERFACE glfw)
    ENDIF()
ENDIF()
