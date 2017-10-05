#pragma once
#ifndef VULPES_SPHERE_UTILITY_HPP
#define VULPES_SPHERE_UTILITY_HPP
#include "vpr_stdafx.h"
#include "Ray.hpp"
namespace vulpes {

    struct UtilitySphere {

        UtilitySphere() = default;
        UtilitySphere(const glm::vec3& _center, const float& _radius);

        bool CheckRayIntersection(const Ray& _ray, float* intersection) const noexcept;
        glm::vec3 ClosestPointToRay(const Ray& _ray) const noexcept;
        UtilitySphere GetTransformedSphere(const glm::mat4& transformation_matrix) const noexcept;
        void CalculateSphereProjection(const float& focal_length, glm::vec2* out_center, glm::vec2* out_axis_a, glm::vec2* out_axis_b) const noexcept;
        void CalculateSphereProjection(const float& focal_length, const glm::ivec2& screen_size_in_pixels, glm::vec2* out_center, glm::vec2* out_axis_a, glm::vec2* out_axis_b) const noexcept;
        float CalcualteProjectedSphereArea(const float& focal_length, const glm::ivec2& screen_size_in_pixels) const noexcept;

        static UtilitySphere CalculateBoundingSphere(const std::vector<glm::vec3>& _points);
         
        glm::vec3 Center;
        float Radius;
    };

}

#endif // !VULPES_SPHERE_UTILITY_HPP
