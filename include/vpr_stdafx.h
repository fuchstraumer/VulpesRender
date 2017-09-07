// vpr_stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once
#pragma warning(push, 0)
#define NOMINMAX
#include <string>
#include <vector>
#include <iostream>
#include <sstream>
#include <fstream>
#include <algorithm>
#include <array>
#include <memory>
#include <map>
#include <iostream>
#include <chrono>
#include <future>
#include <filesystem>
#include <condition_variable>
#include <regex>
#include <limits>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_SWIZZLE
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "glm/gtc/quaternion.hpp"
#include "glm/gtx/hash.hpp"

#include "gli/gli.hpp"

#include "vulkan/vulkan.h"

#define GLFW_INCLUDE_VULKAN
#include "glfw/glfw3.h"

#if defined(__linux__)
#include <wayland-client.h>
#elif defined(_WIN32) 
#undef APIENTRY
#define GLFW_EXPOSE_NATIVE_WIN32
#define GLFW_EXPOSE_NATIVE_WGL
#include "GLFW/glfw3native.h"
#endif

// Number of odd/broken defines in this, as it includes windows.h 

#include "util/easylogging++.h"


#include "assimp/Importer.hpp"
#include "assimp/postprocess.h"
#include "assimp/scene.h"

#include "SPIRV/GlslangToSpv.h"
#include "glslang/Public/ShaderLang.h"
#include "SPIRV/disassemble.h"

#pragma warning(pop)

// Disable warning of initialized local variable not referenced (occurs with VkAssert in release)
#ifdef NDEBUG
#pragma warning(push)
#pragma warning(disable : 4189)
#pragma warning(pop)
#endif //!NDEBUG

#include "common/CreateInfoBase.hpp"
#include "common/vkAssert.hpp"
#include "common/vk_constants.hpp"