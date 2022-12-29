CMAKE_MINIMUM_REQUIRED(VERSION 3.5)
PROJECT(vpr)

OPTION(VPR_BUILD_DYNAMIC_LIBRARY "Build VPR as a dynamic library - static is far easier" OFF)
OPTION(VPR_VERBOSE_LOGGING "Enable extra output from easyloggingpp" OFF)
OPTION(VPR_BUILD_ANDROID "Build for Android platform" OFF)
OPTION(VPR_USE_SDL "Use SDL instead of glfw" OFF)
OPTION(VPR_BUILD_TESTS "Build Unit and Integration tests" ON)

SET(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Doing this just so we have our own variable scope for GLFW, because we really want to use glfw as a shared 
# library. Otherwise, variable state and functionality gets weird across separate modules / libraries
FUNCTION(ADD_GLFW)
    SET(BUILD_SHARED_LIBS ON)
    ADD_SUBDIRECTORY(third_party/glfw)
ENDFUNCTION()

SET(GLFW_BUILD_DOCS OFF CACHE BOOL "Build the GLFW documentation")
SET(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "Build the GLFW example programs")
SET(GLFW_BUILD_TESTS OFF CACHE BOOL "Build the GLFW test programs")
IF(NOT VPR_USE_SDL)
    ADD_GLFW()
ENDIF()

FIND_PACKAGE(Vulkan)

FUNCTION(ADD_VPR_LIBRARY NAME)
    IF(VPR_BUILD_DYNAMIC_LIBRARY)
        ADD_LIBRARY(${NAME} SHARED ${ARGN} ${easyloggingpp_src})
        TARGET_LINK_LIBRARIES(${NAME} PRIVATE ${Vulkan_LIBRARY})
    ELSE()
        ADD_LIBRARY(${NAME} STATIC ${ARGN})
        TARGET_LINK_LIBRARIES(${NAME} INTERFACE ${Vulkan_LIBRARY})
        TARGET_COMPILE_DEFINITIONS(${NAME} PRIVATE VPR_BUILD_STATIC)
    ENDIF()
    TARGET_COMPILE_OPTIONS(${NAME} PRIVATE ${VPR_CXX_FLAGS})
    TARGET_INCLUDE_DIRECTORIES(${NAME} PUBLIC 
        "${Vulkan_INCLUDE_DIR}"
        "../common/"
        "../third_party/easyloggingpp/src"
        "../third_party/variant_lite/include")
    INSTALL(TARGETS ${NAME} LIBRARY DESTINATION "lib" ARCHIVE DESTINATION "lib"
        RUNTIME DESTINATION "bin")
    INSTALL(DIRECTORY "include/" DESTINATION "include/vpr/")
    IF(BUILD_DYNAMIC_LIBRARY)
    TARGET_COMPILE_DEFINITIONS(${NAME} PUBLIC 
        $<BUILD_INTERFACE:VPR_BUILD_DLL>
        $<INSTALL_INTERFACE:VPR_DLL>)
    ENDIF()
    IF(VPR_USE_SDL)
         TARGET_COMPILE_DEFINITIONS(${NAME} PRIVATE "VPR_USE_SDL")
    ENDIF()
    SET_TARGET_PROPERTIES(${NAME} PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED YES)
    IF(VERBOSE_LOGGING) 
        TARGET_COMPILE_DEFINITIONS(${NAME} PRIVATE "VPR_VERBOSE_LOGGING")
    ENDIF()
    SET_TARGET_PROPERTIES(${NAME} PROPERTIES FOLDER "VPR Modules")
    TARGET_COMPILE_DEFINITIONS(${NAME} PRIVATE "NOMINMAX")
ENDFUNCTION()

INSTALL(DIRECTORY "common/" DESTINATION "include/vpr/")

ADD_SUBDIRECTORY(command)
ADD_SUBDIRECTORY(core)
ADD_SUBDIRECTORY(render)
ADD_SUBDIRECTORY(resource)
ADD_SUBDIRECTORY(sync)
IF (VPR_BUILD_TESTS)
    ADD_EXECUTABLE(vpr_triangle_test "${CMAKE_CURRENT_SOURCE_DIR}/test/triangle.cpp")
    TARGET_INCLUDE_DIRECTORIES(vpr_triangle_test PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/core/include"
        vpr_core vpr_render vpr_resource vpr_command vpr_sync)
    TARGET_LINK_LIBRARIES(vpr_triangle_test PRIVATE
        vpr_core vpr_render vpr_resource vpr_command
        vpr_sync glfw)
ENDIF()
