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
#include <mutex>
#include <map>
#include <iostream>
#include <fstream>

#if defined(_WIN32) || defined(__linux__)
#include "vulkan/vulkan.h"
#elif defined (__APPLE__)
#include "MoltenVK/mvk_vulkan.h"
#else
#pragma message("No valid platform detected for Vulkan!")
#endif

#define GLFW_INCLUDE_VULKAN
#include "GLFW/glfw3.h"
#if defined(_WIN32) 
#undef APIENTRY
#define GLFW_EXPOSE_NATIVE_WIN32
#define GLFW_EXPOSE_NATIVE_WGL
#include "GLFW/glfw3native.h"
#endif

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "glm/gtc/quaternion.hpp"
#include "gli/gli.hpp"

#pragma warning(pop)

// Disable warning of initialized local variable not referenced (occurs with VkAssert in release)
#ifdef NDEBUG
#pragma warning(disable : 4189)
#endif //!NDEBUG

#include "common/CreateInfoBase.hpp"
#include "common/vkAssert.hpp"
#include "common/vk_constants.hpp"

