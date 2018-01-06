#pragma once
// Push this only for external libraries.
#pragma warning(push, 0)
#ifdef _WIN32
#define NOMINMAX
#endif //!_WIN32
#include <string>
#include <vector>
#include <array>
#include <memory>

#if defined(_WIN32) || defined(__linux__)
#include "vulkan/vulkan.h"
#elif defined (__APPLE__)
#include "MoltenVK/mvk_vulkan.h"
#else
#pragma message("No valid platform detected for Vulkan!")
#endif


#if defined(__GNUC__)
    #define EXPORT __attribute__((visibility("default")))
    #define IMPORT
#else
    #define EXPORT __declspec(dllexport)
    #define IMPORT __declspec(dllimport)
#endif

#ifdef VPR_DLL
#define VPR_API EXPORT
#elif defined(VPR_BUILD_DLL)
#define VPR_API IMPORT
#else
#define VPR_API
#endif

#define GLFW_INCLUDE_VULKAN

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#define GLM_LANG_STL11_FORCED
#include "glm/glm.hpp"

#pragma warning(pop)

#include "common/CreateInfoBase.hpp"
#include "common/vkAssert.hpp"
#include "common/vk_constants.hpp"


