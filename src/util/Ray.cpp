#include "vpr_stdafx.h"
#include "util/Ray.hpp"

namespace vulpes {



    Ray::Ray(const glm::vec3 & _origin, const glm::vec3& _dir) : direction(_dir), origin(_origin), invDirection(1.0f / _dir), signX((_dir.x < 0.0f) ? 1 : 0), signY((_dir.y < 0.0f) ? 1 : 0), signZ((_dir.z < 0.0f) ? 1 : 0) {}

    const glm::vec3 & Ray::GetOrigin() const noexcept {
        return origin;
    }

    const glm::vec3& Ray::GetDirection() const noexcept {
        return direction;
    }

    const glm::vec3& Ray::GetInvDirection() const noexcept {
        return invDirection;
    }

    void Ray::SetDirection(const glm::vec3& new_direction) noexcept {
        direction = new_direction;
        invDirection = 1.0f / direction;
        signX = (direction.x < 0.0f) ? 1 : 0;
        signY = (direction.y < 0.0f) ? 1 : 0;
        signZ = (direction.z < 0.0f) ? 1 : 0;
    }

    void Ray::SetOrigin(const glm::vec3 & new_origin) noexcept {
        origin = new_origin;
    }

    void Ray::TransformRay(const glm::mat4& transformation_matrix) noexcept {
        origin = glm::vec3(transformation_matrix * glm::vec4(origin.x, origin.y, origin.z, 1.0f));
        SetDirection(glm::mat3(transformation_matrix) * direction);
    }

    Ray Ray::GetTransformedRay(const glm::mat4 & transform_matrix) const noexcept {
        Ray result(origin, direction);
        result.TransformRay(transform_matrix);
        return result;
    }

    glm::vec3 Ray::CalculatePosition(const float & t) const noexcept {
        return glm::vec3(origin + (direction * t));
    }

    bool Ray::CheckTriangleIntersection(const glm::vec3 & vert_pos0, const glm::vec3 & vert_pos1, const glm::vec3 & vert_pos2, float * distance_to_intersection) const noexcept {
        glm::vec3 edge0, edge1, tvec, pvec, qvec;
        float dot_product;
        float u, v;
        constexpr float EPSILON = 1.0e-6f;

        edge0 = vert_pos1 - vert_pos0;
        edge1 = vert_pos2 - vert_pos0;

        pvec = glm::cross(direction, edge1);
        dot_product = glm::dot(edge0, pvec);

        if (dot_product > -EPSILON && dot_product < EPSILON) {
            return false;
        }

        float inv_dot_product = 1.0f / dot_product;
        tvec = origin - vert_pos0;

        u = glm::dot(tvec, pvec) * inv_dot_product;
        if (u < 0.0f || u > 1.0f) {
            return false;
        }

        qvec = glm::cross(tvec, edge0);

        v = glm::dot(direction, qvec) * inv_dot_product;
        if (v < 0.0f || v > 1.0f) {
            return false;
        }

        if (distance_to_intersection != nullptr) {
            *distance_to_intersection = glm::dot(edge1, qvec) * inv_dot_product;
        }

        return true;
    }

    bool Ray::CheckPlaneIntersection(const glm::vec3 & plane_origin, const glm::vec3 & plane_normal, float * distance_to_intersection) const noexcept {
        float denom = glm::dot(plane_normal, direction);
        if (denom != 0.0f) {
            if (distance_to_intersection != nullptr) {
                *distance_to_intersection = glm::dot(plane_normal, plane_origin - origin) / denom;
            }
            return true;
        }
        return false;
    }

}