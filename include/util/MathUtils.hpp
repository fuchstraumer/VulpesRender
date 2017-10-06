#pragma once
#ifndef VULPES_MATH_UTILITIES_HPP
#define VULPES_MATH_UTILITIES_HPP

#include "vpr_stdafx.h"

namespace vulpes {

    float PointOnEllipseBisector(const size_t& num_components, const glm::vec2& extents, const glm::vec2& y, glm::vec2& x);
    float PointOnEllipseSqrDistanceImpl(const glm::vec2& extents, const glm::vec2& y, glm::vec2& x);
    float PointOnEllipseSqrDistance(const glm::vec2& extents, const glm::vec2& y, glm::vec2& x);
    glm::vec2 GetClosestPointOnEllipse(const glm::vec2& center, const glm::vec2& axis_a, const glm::vec2& axis_b, const glm::vec2& test_point);

}

#endif // !VULPES_MATH_UTILITIES_HPP
