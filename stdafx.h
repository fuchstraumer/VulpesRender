// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#include <stdio.h>
#include <tchar.h>
#include <vector>
#include <array>
#include <type_traits>
#include <algorithm>
#include <string>
#include <ostream>
#include <fstream>
#include <sstream>
#include <iostream>
#include <stdint.h>
#include <set>
#include <chrono>
#include <memory>
#include <map>
#include <unordered_map>
#include <random>
#include <gli\gli.hpp>

#include "vulkan/vulkan.h"
#include "common/CreateInfoBase.h"

#define GLFW_DLL
#define GLFW_INCLUDE_VULKAN
#include "GLFW\glfw3.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include "glm\glm.hpp"
#include "glm\gtc\matrix_transform.hpp"
#include "glm\gtc\type_ptr.hpp"
#include "glm\gtc\quaternion.hpp"


#define VK_DEBUG_ENABLE

#include "assimp/Importer.hpp"
#include "assimp/postprocess.h"
#include "assimp/cimport.h"
#include "assimp/scene.h"

#include "common/vk_constants.h"

#include "common/vkAssert.h"

constexpr uint32_t DEFAULT_WIDTH = 1920, DEFAULT_HEIGHT = 1080;

// TODO: reference additional headers your program requires here
