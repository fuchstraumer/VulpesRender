MACRO(ADD_PRECOMPILED_HEADER PrecompiledHeader PrecompiledSource SourcesVar)
  GET_FILENAME_COMPONENT(PrecompiledBasename ${PrecompiledHeader} NAME_WE)
  SET(PrecompiledBinary "${CMAKE_CURRENT_BINARY_DIR}/${PrecompiledBasename}.pch")
  SET(Sources ${${SourcesVar}})
  IF(MSVC)
    SET_SOURCE_FILES_PROPERTIES(
      ${PrecompiledSource}	PROPERTIES
      COMPILE_FLAGS "/Yc\"${PrecompiledHeader}\" /Fp\"${PrecompiledBinary}\""
      OBJECT_OUTPUTS "${PrecompiledBinary}"
    )

    SET_SOURCE_FILES_PROPERTIES(
      ${Sources} PROPERTIES
      COMPILE_FLAGS "/Yu\"${PrecompiledBinary}\" /FI\"${PrecompiledBinary}\" /Fp\"${PrecompiledBinary}\""
      OBJECT_DEPENDS "${PrecompiledBinary}"
    )
  ELSEIF(${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU" OR ${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang")
    ADD_CUSTOM_COMMAND(
      OUTPUT ${PrecompiledBinary}
      COMMAND "${CMAKE_CXX_COMPILER} -x c++-header -o ${PrecompiledBinary}"
      COMMENT "Precompiling vpr_stdafx.h..."
    )

    SET_SOURCE_FILES_PROPERTIES(
      ${Sources} PROPERTIES
      COMPILE_FLAGS "-include ${PrecompiledHeader}"
      OBJECT_DEPENDS "${PrecompiledBinary}"
    )
  ENDIF(MSVC)
ENDMACRO(ADD_MSVC_PRECOMPILED_HEADER)

INCLUDE(ExternalProject)
CMAKE_MINIMUM_REQUIRED(VERSION 3.3)
PROJECT(VulpesRender)

SET_PROPERTY(GLOBAL PROPERTY USE_FOLDERS ON) 

SET(CMAKE_CXX_STANDARD 14)
SET(CMAKE_CXX_STANDARD_REQUIRED ON)

IF(NOT MSVC) # compile commands used for clang-format and clang-tidy
    SET(CMAKE_EXPORT_COMPILE_COMMANDS ON)
ENDIF()

IF(APPLE)
  FIND_LIBRARY(MoltenVK NAMES "libMoltenVK.dylib" HINTS "${CMAKE_CURRENT_SOURCE_DIR}/ext/MoltenVK/macOS")
ENDIF()

OPTION(FORCE_ALLOCATOR_VALIDATION "Force validation of Vulkan memory even in Release builds." OFF)
OPTION(BUILD_EXAMPLE_SCENES "Build example scene executables testing/demonstrating Vulkan features" OFF)

IF(APPLE) 
  SET(APPLE_VK_INCL "-DVULKAN_INCLUDE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/ext/MoltenVk/include/vulkan")
  SET(APPLE_VK_LIB "-DVULKAN_LIBRARY=${MoltenVK}")
ENDIF()

ExternalProject_Add(glfw3
  PREFIX "${CMAKE_CURRENT_SOURCE_DIR}/download/glfw"
  GIT_REPOSITORY "https://github.com/glfw/glfw.git"
  GIT_TAG ""
  BUILD_IN_SOURCE 1
  CMAKE_ARGS 
    "-DCMAKE_INSTALL_PREFIX=${CMAKE_CURRENT_SOURCE_DIR}/ext/"
    "-DGLFW_BUILD_SHARED_LIBS=ON"
    "-DGLFW_BUILD_EXAMPLES=OFF"
    "-DGLFW_BUILD_TESTS=OFF"
    "-DGLFW_BUILD_DOCS=OFF"
    "-DGLFW_DOCUMENT_INTERNALS=OFF"
    "-DUSE_MSVC_RUNTIME_LIBRARY_DLL=ON"
    "${APPLE_VK_LIB}"
    "${APPLE_VK_INCLUDE}")

ExternalProject_Add(glm
    PREFIX "${CMAKE_CURRENT_SOURCE_DIR}/download/glm"
    GIT_REPOSITORY "https://github.com/g-truc/glm.git"
    CMAKE_ARGS "-DCMAKE_INSTALL_PREFIX=${CMAKE_CURRENT_SOURCE_DIR}/ext")

FILE(GLOB UTIL "src/util/*.cc" "src/util/*.cpp" "include/util/*.h" "include/util/*.hpp")
FILE(GLOB COMMON "src/common/*.h" "src/common/*.hpp")
FILE(GLOB COMMAND_FILES "src/command/*.cpp" "include/command/*.hpp")
FILE(GLOB CORE "src/core/*.cpp" "include/core/*.hpp")
FILE(GLOB RESOURCES "src/resource/*.cpp" "include/resource/*.hpp")
FILE(GLOB RENDER "src/render/*.cpp" "include/render/*.hpp" "include/render/*.inl")
FILE(GLOB GUI "src/gui/*.cpp" "include/gui/*.hpp")
FILE(GLOB GUI_ADDONS "src/gui/addons/*.cpp" "include/gui/addons/*.h")
FILE(GLOB IMGUI "imgui/imgui.cpp" "imgui/imgui.h" "imgui/imgui_draw.cpp" "imgui/imgui_internal.h" "imgui/imgui_demo.cpp")
FILE(GLOB OBJECTS "src/objects/*.cpp" "include/objects/*.hpp")
FILE(GLOB SCENES "scenes/*.cpp")
FILE(GLOB_RECURSE SHADERS 
    "scenes/scene_resources/shaders/*.vert" "scenes/scene_resources/shaders/*.frag" "scenes/scene_resources/shaders/*.spv" 
    "rsrc/shaders/*.vert" "rsrc/shaders/*.frag" "rsrc/shaders/*.spv"
    )

FILE(GLOB PROJECT_SOURCES "src/util/*.cpp" "src/command/*.cpp" "src/core/*.cpp" "src/render/*.cpp" "src/resource/*.cpp" "src/gui/*.cpp" "src/objects/*.cpp" "src/BaseScene.cpp")
ADD_PRECOMPILED_HEADER("vpr_stdafx.h" "src/vpr_stdafx.cpp" ${PROJECT_SOURCES})

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
SOURCE_GROUP("shaders" FILES ${SHADERS})

# enable multi-threaded compile for release builds, when its available. Enable minimal rebuild for debug
# as it's seemingly disabled by default, for some reason.
IF(MSVC) 
  SET(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /Gm")
  SET(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MP")
  SET(CMAKE_CXX_FLAGS_MINSIZEREL "${CMAKE_CXX_FLAGS_MINSIZEREL} /MP")
  SET(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} /MP")
ENDIF()

ADD_LIBRARY(VulpesRender STATIC "include/vpr_stdafx.h" "src/vpr_stdafx.cpp" "include/ForwardDecl.hpp" 
                                "include/BaseScene.hpp" "src/BaseScene.cpp" "include/BaseSceneConfig.hpp" 
                                ${OBJECTS} ${IMGUI} ${GUI_ADDONS} ${UTIL} ${COMMON} ${COMMAND_FILES} 
                                ${CORE} ${RESOURCES} ${RENDER} ${GUI} ${SHADERS}
                                )

IF(APPLE OR UNIX)
FIND_LIBRARY(GLFW_3_LIB NAMES "libglfw3.a" HINTS "${PROJECT_SOURCE_DIR}/ext/lib")
  IF(APPLE)
  SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -framework Cocoa -framework Metal -framework IOKit -framework CoreVideo")
  ENDIF()
ENDIF()                          

IF(APPLE)
  TARGET_LINK_LIBRARIES(VulpesRender INTERFACE "${GLFW_3_LIB}" "${MoltenVK}")
ELSEIF(UNIX)
  TARGET_LINK_LIBRARIES(VulpesRender INTERFACE "${GLFW_3_LIB}" "$ENV{VULKAN_SDK}/lib/libvulkan.so" "dl" "pthread" "X11" "Xi" "Xrandr")
ELSEIF(WIN32)
  TARGET_LINK_LIBRARIES(VulpesRender INTERFACE "$ENV{VULKAN_SDK}/Lib/vulkan-1.lib" "${PROJECT_SOURCE_DIR}/ext/lib/glfw3.lib")
ENDIF()
                        
ADD_DEPENDENCIES(VulpesRender glm glfw3)

# GLM doesn't detect the compiler version correctly: defined as public since it will occur in files using this library if undefined.
TARGET_COMPILE_DEFINITIONS(VulpesRender PUBLIC -DGLM_LANG_STL11_FORCED)

# Temporary: need help to fix this one. Vulpes builds as 32-bit, somehow, but glfw builds as 64 bit?
IF(UNIX)
  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m64")
ENDIF()

IF(MSVC) 
  TARGET_COMPILE_DEFINITIONS(VulpesRender PRIVATE -DUSE_EXPERIMENTAL_FILESYSTEM)
ENDIF()

IF(FORCE_ALLOCATOR_VALIDATION) 
  TARGET_COMPILE_DEFINITIONS(VulpesRender PRIVATE -DFORCE_ALLOCATOR_VALIDATION)
ENDIF()

TARGET_INCLUDE_DIRECTORIES(VulpesRender PUBLIC 
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:VulpesRender>)
TARGET_INCLUDE_DIRECTORIES(VulpesRender PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/ext/include>)
TARGET_INCLUDE_DIRECTORIES(VulpesRender PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/imgui>
    $<INSTALL_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/imgui>)

IF(APPLE)
  TARGET_INCLUDE_DIRECTORIES(VulpesRender PUBLIC 
      $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/ext/MoltenVK/include>)
ELSE()
  TARGET_INCLUDE_DIRECTORIES(VulpesRender PUBLIC 
      $<BUILD_INTERFACE:$ENV{VULKAN_SDK}/include>
      $<INSTALL_INTERFACE:$ENV{VULKAN_SDK}/include>)
ENDIF()


# Easier to gather headers this way, so we can more cleanly set where they end up.
INSTALL(DIRECTORY "include/" DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/VulpesRender/")
INSTALL(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/ext/include/GLFW" DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/GLFW")
INSTALL(TARGETS VulpesRender LIBRARY DESTINATION "lib" ARCHIVE DESTINATION "lib")

INSTALL(FILES "include/vpr_stdafx.h" "include/BaseScene.hpp" "include/ForwardDecl.hpp" "include/BaseSceneConfig.hpp" 
        DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/VulpesRender")

IF(BUILD_EXAMPLE_SCENES)

ADD_EXECUTABLE(Triangle "scenes/Triangle.cpp")
TARGET_LINK_LIBRARIES(Triangle VulpesRender)

ADD_EXECUTABLE(House "scenes/House.cpp")
TARGET_LINK_LIBRARIES(House VulpesRender)

ADD_EXECUTABLE(Arcball "scenes/ArcballScene.cpp")
TARGET_LINK_LIBRARIES(Arcball VulpesRender)

ENDIF()
