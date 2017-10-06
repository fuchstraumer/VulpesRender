#pragma once
#ifndef VULPES_VK_MATRIX_UTILS_HPP
#define VULPES_VK_MATRIX_UTILS_HPP

#include "vpr_stdafx.h"
#include "glm/gtx/norm.hpp"

namespace vulpes {

    glm::mat4 AlignZAxisWithTarget(glm::vec3& target_direction, glm::vec3& up_direction);

}
#endif // !VULPES_VK_MATRIX_UTILS_HPP
