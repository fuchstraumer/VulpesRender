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

