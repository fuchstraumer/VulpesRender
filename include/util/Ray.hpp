#pragma once
#ifndef VULPESRENDER_RAY_HPP
#define VULPESRENDER_RAY_HPP
#include "vpr_stdafx.h"

namespace vulpes {

    class Ray {
    public:

        Ray() = default;
        Ray(const glm::vec3& _origin, const glm::vec3& _dir);

        const glm::vec3& GetOrigin() const noexcept;
        const glm::vec3& GetDirection() const noexcept;
        const glm::vec3& GetInvDirection() const noexcept;

        void SetDirection(const glm::vec3& new_direction) noexcept;
        void SetOrigin(const glm::vec3& new_origin) noexcept;

        void TransformRay(const glm::mat4& transform_matrix) noexcept;
        Ray GetTransformedRay(const glm::mat4& transform_matrix) const noexcept;

        glm::vec3 CalculatePosition(const float& t) const noexcept;

        bool CheckTriangleIntersection(const glm::vec3& vert_pos0, const glm::vec3& vert_pos1, const glm::vec3& vert_pos2, float* distance_to_intersection = nullptr) const noexcept;
        bool CheckPlaneIntersection(const glm::vec3& plane_origin, const glm::vec3& plane_normal, float* distance_to_intersection = nullptr) const noexcept;

    protected:

        glm::vec3 origin, direction, invDirection;
        char signX, signY, signZ;
    };
    
}
#endif // !VULPESRENDER_RAY_HPP
