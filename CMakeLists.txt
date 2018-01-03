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
ENDMACRO(ADD_PRECOMPILED_HEADER)

INCLUDE(ExternalProject)
CMAKE_MINIMUM_REQUIRED(VERSION 3.3)
PROJECT(VulpesRender)

SET_PROPERTY(GLOBAL PROPERTY USE_FOLDERS ON) 

SET(CMAKE_CXX_STANDARD 17)
SET(CMAKE_CXX_STANDARD_REQUIRED ON)

IF(NOT MSVC) # compile commands used for clang-format and clang-tidy
    SET(CMAKE_EXPORT_COMPILE_COMMANDS ON)
ENDIF()

IF(APPLE)
  FIND_LIBRARY(MoltenVK NAMES "libMoltenVK.dylib" HINTS "${CMAKE_CURRENT_SOURCE_DIR}/ext/MoltenVK/macOS")
ENDIF()

OPTION(FORCE_ALLOCATOR_VALIDATION "Force validation of Vulkan memory even in Release builds." OFF)
OPTION(FORCE_ASSERT "Forces assert macro checking for VkResult return codes" OFF)

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

FILE(GLOB UTIL "${CMAKE_CURRENT_SOURCE_DIR}/src/util/*.cc" "${CMAKE_CURRENT_SOURCE_DIR}/src/util/*.cpp" 
    "${CMAKE_CURRENT_SOURCE_DIR}/include/util/*.h" "${CMAKE_CURRENT_SOURCE_DIR}/include/util/*.hpp")
FILE(GLOB COMMON "${CMAKE_CURRENT_SOURCE_DIR}/src/common/*.h" "${CMAKE_CURRENT_SOURCE_DIR}/src/common/*.hpp")
FILE(GLOB COMMAND_FILES "${CMAKE_CURRENT_SOURCE_DIR}/src/command/*.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/include/command/*.hpp")
FILE(GLOB CORE "${CMAKE_CURRENT_SOURCE_DIR}/src/core/*.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/include/core/*.hpp")
FILE(GLOB RESOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/resource/*.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/include/resource/*.hpp")
FILE(GLOB RENDER "${CMAKE_CURRENT_SOURCE_DIR}/src/render/*.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/include/render/*.hpp" "${CMAKE_CURRENT_SOURCE_DIR}/include/render/*.inl")

FILE(GLOB PROJECT_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/util/*.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/src/command/*.cpp" 
        "${CMAKE_CURRENT_SOURCE_DIR}/src/core/*.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/src/render/*.cpp" 
        "${CMAKE_CURRENT_SOURCE_DIR}/src/resource/*.cpp")
ADD_PRECOMPILED_HEADER("vpr_stdafx.h" "${CMAKE_CURRENT_SOURCE_DIR}/src/vpr_stdafx.cpp" ${PROJECT_SOURCES})

SOURCE_GROUP("util" FILES ${UTIL})
SOURCE_GROUP("common" FILES ${COMMON})
SOURCE_GROUP("command" FILES ${COMMAND_FILES})
SOURCE_GROUP("core" FILES ${CORE})
SOURCE_GROUP("resource" FILES ${RESOURCES})
SOURCE_GROUP("render" FILES ${RENDER})
SOURCE_GROUP("shaders" FILES ${SHADERS})
SOURCE_GROUP("imgui" FILES ${IMGUI})

ADD_LIBRARY(VulpesRender STATIC "${CMAKE_CURRENT_SOURCE_DIR}/include/vpr_stdafx.h" "${CMAKE_CURRENT_SOURCE_DIR}/src/vpr_stdafx.cpp" 
    "${CMAKE_CURRENT_SOURCE_DIR}/include/ForwardDecl.hpp" ${UTIL} ${COMMON} ${COMMAND_FILES} ${IMGUI}
    ${CORE} ${RESOURCES} ${RENDER} ${SHADERS})

#Enable minimal rebuild, and manually set c++17 standard since it doesn't get set right otherwise.
IF(MSVC)
TARGET_COMPILE_OPTIONS(VulpesRender PUBLIC "/std:c++17" "/Gm")
ENDIF()

IF(APPLE)
FIND_LIBRARY(GLFW_3_LIB NAMES "libglfw3.a" HINTS "${PROJECT_SOURCE_DIR}/ext/lib")
  IF(APPLE)
  SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -framework Cocoa -framework Metal -framework IOKit -framework CoreVideo")
  ENDIF()
ENDIF() 

IF(APPLE)
  TARGET_LINK_LIBRARIES(VulpesRender INTERFACE "${GLFW_3_LIB}" "${MoltenVK}")
ELSEIF(UNIX)
  TARGET_LINK_LIBRARIES(VulpesRender INTERFACE "ext/lib/libglfw3.a" "$ENV{VULKAN_SDK}/lib/libvulkan.so" "dl" "pthread" "X11" "Xi" "Xrandr")  
ELSEIF(WIN32)
  TARGET_LINK_LIBRARIES(VulpesRender INTERFACE "$ENV{VULKAN_SDK}/Lib/vulkan-1.lib" "${PROJECT_SOURCE_DIR}/ext/lib/glfw3.lib")
ENDIF()
                        
ADD_DEPENDENCIES(VulpesRender glfw3)
# GLM doesn't detect the compiler version correctly: defined as public since it will occur in files using this library if undefined.
TARGET_COMPILE_DEFINITIONS(VulpesRender PUBLIC -DGLM_LANG_STL11_FORCED)

# Temporary: need help to fix this one. Vulpes builds as 32-bit, somehow, but glfw builds as 64 bit?
IF(UNIX)
TARGET_COMPILE_OPTIONS(VulpesRender PUBLIC "-m64")
#TARGET_COMPILE_OPTIONS(VulpesRender PRIVATE "-fPIC")
ENDIF()

IF(MSVC) 
  TARGET_COMPILE_DEFINITIONS(VulpesRender PRIVATE -DUSE_EXPERIMENTAL_FILESYSTEM)
ENDIF()

IF(FORCE_ALLOCATOR_VALIDATION) 
  TARGET_COMPILE_DEFINITIONS(VulpesRender PRIVATE -DFORCE_ALLOCATOR_VALIDATION)
ENDIF()

IF(FORCE_ASSERT) 
    TARGET_COMPILE_DEFINITIONS(VulpesRender PUBLIC -DVK_FORCE_ASSERT)
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
INSTALL(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/ext/include/GLFW" DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")
INSTALL(DIRECTORY "ext/include/glm" DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")
INSTALL(DIRECTORY "ext/include/gli" DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")
INSTALL(DIRECTORY "ext/include/stb" DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")
INSTALL(TARGETS VulpesRender LIBRARY DESTINATION "lib" ARCHIVE DESTINATION "lib")

INSTALL(FILES "include/vpr_stdafx.h" "include/ForwardDecl.hpp" 
        DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/VulpesRender")

