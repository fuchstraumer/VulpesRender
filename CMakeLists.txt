MACRO(ADD_MSVC_PRECOMPILED_HEADER PrecompiledHeader PrecompiledSource SourcesVar)
  IF(MSVC)
    GET_FILENAME_COMPONENT(PrecompiledBasename ${PrecompiledHeader} NAME_WE)
    SET(PrecompiledBinary "${CMAKE_CURRENT_BINARY_DIR}/${PrecompiledBasename}.pch")
    SET(Sources ${${SourcesVar}})

    SET_SOURCE_FILES_PROPERTIES(${PrecompiledSource}
                                PROPERTIES COMPILE_FLAGS "/Yc\"${PrecompiledHeader}\" /Fp\"${PrecompiledBinary}\""
                                           OBJECT_OUTPUTS "${PrecompiledBinary}")
    SET_SOURCE_FILES_PROPERTIES(${Sources}
                                PROPERTIES COMPILE_FLAGS "/Yu\"${PrecompiledHeader}\" /FI\"${PrecompiledHeader}\" /Fp\"${PrecompiledBinary}\""
                                           OBJECT_DEPENDS "${PrecompiledBinary}")  
    # Add precompiled header to SourcesVar
    LIST(APPEND ${SourcesVar} ${PrecompiledSource})
  ENDIF(MSVC)
ENDMACRO(ADD_MSVC_PRECOMPILED_HEADER)

INCLUDE(ExternalProject)
CMAKE_MINIMUM_REQUIRED(VERSION 3.6)
PROJECT(VulpesRender)

SET_PROPERTY(GLOBAL PROPERTY USE_FOLDERS ON) 
SET(CMAKE_CXX_STANDARD 14)
SET(CMAKE_CXX_STANDARD_REQUIRED ON)

IF(NOT MSVC) # compile commands used for clang-format and clang-tidy
    SET(CMAKE_EXPORT_COMPILE_COMMANDS ON)
ENDIF()

IF(MSVC)
  OPTION(USE_MSVC_RUNTIME_LIBRARY_DLL "Use MSVC runtime DLL" ON)
  ADD_DEFINITIONS("-DUSE_EXPERIMENTAL_FILESYSTEM")
ENDIF()

IF(CLANG) 
 SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")
ENDIF()

IF(APPLE)
FIND_LIBRARY(MoltenVK NAMES "libMoltenVK.dylib" HINTS "${CMAKE_CURRENT_SOURCE_DIR}/ext/MoltenVK/macOS")
ENDIF()

OPTION(FORCE_ALLOCATOR_VALIDATION "Force validation of Vulkan memory even in Release builds." OFF)
OPTION(BUILD_EXAMPLE_SCENES "Build example scene executables testing/demonstrating Vulkan features" OFF)

IF(FORCE_ALLOCATOR_VALIDATION) 
    ADD_DEFINITIONS("-DFORCE_ALLOCATOR_VALIDATION")
ENDIF()

ExternalProject_Add(glfw3
  PREFIX "${CMAKE_CURRENT_SOURCE_DIR}/download/glfw"
  GIT_REPOSITORY "https://github.com/glfw/glfw.git"
  GIT_TAG ""
  BUILD_IN_SOURCE 1
  CMAKE_ARGS 
    "-DCMAKE_INSTALL_PREFIX=${CMAKE_CURRENT_SOURCE_DIR}/ext/"
    "-DGLFW_BUILD_EXAMPLES=OFF"
    "-DGLFW_BUILD_TESTS=OFF"
    "-DGLFW_BUILD_DOCS=OFF"
    "-DGLFW_DOCUMENT_INTERNALS=OFF"
    "-DCMAKE_DEBUG_POSTFIX=-gd"
    "-DUSE_MSVC_RUNTIME_LIBRARY_DLL=ON"
    "-DVULKAN_LIBRARY=${MoltenVK}"
    "-DVULKAN_INCLUDE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/ext/MoltenVk/include/vulkan")

#ExternalProject_Add(gli 
#    PREFIX "${CMAKE_CURRENT_SOURCE_DIR}/download/gli"
#    GIT_REPOSITORY "https://github.com/g-truc/gli.git"
#    CMAKE_ARGS "-DCMAKE_INSTALL_PREFIX=${CMAKE_CURRENT_SOURCE_DIR}/ext")

ExternalProject_Add(glm
    PREFIX "${CMAKE_CURRENT_SOURCE_DIR}/download/glm"
    GIT_REPOSITORY "https://github.com/g-truc/glm.git"
    CMAKE_ARGS "-DCMAKE_INSTALL_PREFIX=${CMAKE_CURRENT_SOURCE_DIR}/ext")

# Update and init imgui
ADD_CUSTOM_TARGET(update_imgui_submodule
    COMMAND git submodule update --init ${PROJECT_SOURCE_DIR}/imgui)

ADD_CUSTOM_TARGET(UPDATE_STB COMMAND git submodule update --init ${PROJECT_SOURCE_DIR}/ext/include/stb)
ADD_CUSTOM_TARGET(UPDATE_TINYOBJ COMMAND git submodule update --init ${PROJECT_SOURCE_DIR}/ext/include/tinyobj)


FILE(GLOB UTIL "src/util/*.cc" "src/util/*.cpp" "include/util/*.h" "include/util/*.hpp")
FILE(GLOB COMMON "src/common/*.h" "src/common/*.hpp")
FILE(GLOB COMMAND_FILES "src/command/*.cpp" "include/command/*.hpp")
FILE(GLOB CORE "src/core/*.cpp" "include/core/*.hpp")
FILE(GLOB RESOURCES "src/resource/*.cpp" "include/resource/*.hpp")
FILE(GLOB RENDER "src/render/*.cpp" "include/render/*.hpp")
FILE(GLOB GUI "src/gui/*.cpp" "include/gui/*.hpp")
FILE(GLOB GUI_ADDONS "src/gui/addons/*.cpp" "include/gui/addons/*.h")
FILE(GLOB IMGUI "imgui/imgui.cpp" "imgui/imgui.h" "imgui/imgui_draw.cpp" "imgui/imgui_internal.h" "imgui/imgui_demo.cpp")
FILE(GLOB OBJECTS "src/objects/*.cpp" "include/objects/*.hpp")
FILE(GLOB SCENES "scenes/*.cpp")

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/include)
INCLUDE_DIRECTORIES("imgui/")
INCLUDE_DIRECTORIES("ext/include")
IF(NOT APPLE)
INCLUDE_DIRECTORIES($ENV{VULKAN_SDK}/Include)
ELSE()
INCLUDE_DIRECTORIES("${PROJECT_SOURCE_DIR}/ext/MoltenVK/include")
ENDIF()

FILE(GLOB PROJECT_SOURCES "src/util/*.cpp" "src/command/*.cpp" "src/core/*.cpp" "src/render/*.cpp" "src/resource/*.cpp" "src/gui/*.cpp" "src/objects/*.cpp" "src/BaseScene.cpp")
ADD_MSVC_PRECOMPILED_HEADER("vpr_stdafx.h" "src/vpr_stdafx.cpp" ${PROJECT_SOURCES})

SOURCE_GROUP("util" FILES ${UTIL})
SOURCE_GROUP("common" FILES ${COMMON})
SOURCE_GROUP("command" FILES ${COMMAND_FILES})
SOURCE_GROUP("core" FILES ${CORE})
SOURCE_GROUP("resource" FILES ${RESOURCES})
SOURCE_GROUP("render" FILES ${RENDER})
SOURCE_GROUP("gui" FILES ${GUI})
SOURCE_GROUP("imgui addons" FILES ${GUI_ADDONS})
SOURCE_GROUP("imgui" FILES ${IMGUI})
SOURCE_GROUP("objects" FILES ${OBJECTS})

SET(CMAKE_STATIC_LINKER_FLAGS_DEBUG "-NODEFAULTLIB 
  \"${PROJECT_SOURCE_DIR}/ext/lib/glfw3-gd.lib")
SET(CMAKE_STATIC_LINKER_FLAGS_RELEASE "-NODEFAULTLIB 
  \"${PROJECT_SOURCE_DIR}/ext/lib/glfw3.lib\"")

IF(MSVC) # enable multi-threaded compile for release builds, when its available
  SET(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MP")
  SET(CMAKE_CXX_FLAGS_MINSIZEREL "${CMAKE_CXX_FLAGS_MINSIZEREL} /MP")
  SET(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} /MP")
ENDIF()

# Blatantly stolen from GLFW
IF (NOT USE_MSVC_RUNTIME_LIBRARY_DLL)
  FOREACH (flag CMAKE_C_FLAGS
                CMAKE_CXX_FLAGS_DEBUG
                CMAKE_CXX_FLAGS_RELEASE
                CMAKE_CXX_FLAGS_MINSIZEREL
                CMAKE_CXX_FLAGS_RELWITHDEBINFO)

    IF(${flag} MATCHES "/MD")
        string(REGEX REPLACE "/MD" "/MT" ${flag} "${${flag}}")
    ENDIF()
    IF (${flag} MATCHES "/MDd")
        string(REGEX REPLACE "/MDd" "/MTd" ${flag} "${${flag}}")
    ENDIF()

  ENDFOREACH()
ENDIF()

ADD_LIBRARY(VulpesRender STATIC "include/vpr_stdafx.h" "src/vpr_stdafx.cpp" "include/ForwardDecl.hpp" "include/BaseScene.hpp" "src/BaseScene.cpp" "include/BaseSceneConfig.hpp" ${OBJECTS} ${IMGUI} ${GUI_ADDONS} ${UTIL} ${COMMON} ${COMMAND_FILES} ${CORE} ${RESOURCES} ${RENDER} ${GUI} )
ADD_DEPENDENCIES(VulpesRender update_imgui_submodule UPDATE_STB UPDATE_TINYOBJ) # make sure we update+init imgui before creating the project
IF(APPLE)
ADD_DEPENDENCIES(VulpesRender glfw3)
ENDIF()

IF(BUILD_EXAMPLE_SCENES)

IF(APPLE)
FIND_LIBRARY(GLFW_3_LIB NAMES "libglfw3.a" HINTS "${PROJECT_SOURCE_DIR}/ext/lib")
SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -framework Cocoa -framework Metal -framework IOKit -framework CoreVideo")
ENDIF()

ADD_EXECUTABLE(Triangle "scenes/Triangle.cpp")
IF(NOT APPLE)
TARGET_LINK_LIBRARIES(Triangle VulpesRender "$ENV{VULKAN_SDK}/Lib/vulkan-1.lib" "${PROJECT_SOURCE_DIR}/ext/lib/glfw3.lib")
ELSE()
TARGET_LINK_LIBRARIES(Triangle VulpesRender "${GLFW_3_LIB}" "${MoltenVK}")
ENDIF()

ADD_EXECUTABLE(House "scenes/House.cpp")

IF(NOT APPLE)
TARGET_LINK_LIBRARIES(House VulpesRender "$ENV{VULKAN_SDK}/Lib/vulkan-1.lib" "${PROJECT_SOURCE_DIR}/ext/lib/glfw3.lib")
ELSE()
TARGET_LINK_LIBRARIES(House VulpesRender "${GLFW_3_LIB}" "${MoltenVK}")
ENDIF()

ADD_EXECUTABLE(Arcball "scenes/Arcball.cpp")
IF(NOT APPLE)
TARGET_LINK_LIBRARIES(Arcball VulpesRender "$ENV{VULKAN_SDK}/Lib/vulkan-1.lib" "${PROJECT_SOURCE_DIR}/ext/lib/glfw3.lib")
ELSE()
TARGET_LINK_LIBRARIES(Arcball VulpesRender "${GLFW_3_LIB}" "${MoltenVK}")
ENDIF()

ENDIF()

