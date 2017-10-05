#include "vpr_stdafx.h"
#include "util/utilitySphere.hpp"

namespace vulpes {

    constexpr float USPHERE_FLOAT_PI = 3.14159265359f;

    UtilitySphere::UtilitySphere(const glm::vec3 & _center, const float & _radius) : Center(_center), Radius(_radius) {}

    bool UtilitySphere::CheckRayIntersection(const Ray & _ray, float* intersection) const noexcept {
        const glm::vec3 dir_to_ray = _ray.GetOrigin() - Center;
        const float a = glm::dot(_ray.GetDirection(), _ray.GetDirection());
        const float b = 2.0f * glm::dot(dir_to_ray, _ray.GetDirection());
        const float c = glm::dot(dir_to_ray, dir_to_ray) - (Radius * Radius);
        const float disc = b * b - 4.0f * a * c;

        if (disc < 0.0f) {
            return false;
        }
        else {
            const float e = sqrtf(disc);
            const float denom = 2.0f * a;
            float t = ((-1.0f * b) - e) / denom;

            if (t > 1e-4f) {
                *intersection = t;
                return true;
            }

            t = ((-1.0f * b) + e) / denom;
            if (t > 1e-4f) {
                *intersection = t;
                return true;
            }
        }

        return false;
    }

    glm::vec3 UtilitySphere::ClosestPointToRay(const Ray & ray) const noexcept {
        const glm::vec3 dir_to_sphere = ray.GetOrigin() - Center;
        const float a = glm::dot(ray.GetDirection(), ray.GetDirection());
        const float b = 2.0f * glm::dot(dir_to_sphere, ray.GetDirection());
        const float c = glm::dot(dir_to_sphere, dir_to_sphere) - (Radius * Radius);
        const float disc = b * b - 4.0f * a * c;

        if (disc > 0.0f) {
            const float e = sqrtf(disc);
            const float denom = 2.0f * a;
            float t = ((-1.0f * b) - e) / denom;

            if (t > 1e-4f) {
                return ray.CalculatePosition(t);
            }

            t = ((-1.0f * b) + e) / denom;
            if (t > 1e-4) {
                return ray.CalculatePosition(t);
            }
        }

        const float t = glm::dot(-dir_to_sphere, glm::normalize(ray.GetDirection()));
        glm::vec3 pt_on_ray = ray.CalculatePosition(t);
        return Center + glm::normalize(pt_on_ray - Center) * Radius;
    }

    UtilitySphere UtilitySphere::GetTransformedSphere(const glm::mat4 & transformation_matrix) const noexcept {
        glm::vec4 result_center = transformation_matrix * glm::vec4(Center.x, Center.y, Center.z, 1.0f);
        glm::vec4 result_radius = transformation_matrix * glm::vec4(Radius, 0.0f, 0.0f, 0.0f);
        return UtilitySphere(glm::vec3(result_center.x, result_center.y, result_center.z), glm::length(result_radius));
    }

    void UtilitySphere::CalculateSphereProjection(const float & focal_length, glm::vec2 * out_center, glm::vec2 * out_axis_a, glm::vec2 * out_axis_b) const noexcept {
        const glm::vec3 oVec(-Center.x, Center.y, Center.z);
        const float rSq = Radius * Radius;
        const float zSq = oVec.z * oVec.z;
        const float lenSq = glm::dot(oVec, oVec);

        if (out_center != nullptr) {
            *out_center = focal_length * oVec.z * glm::vec2(oVec.x, oVec.y) / (zSq - rSq);
        }

        if (fabsf(zSq - lenSq) > 1e-5f) {
            if (out_axis_a != nullptr) {
                *out_axis_a = focal_length * sqrtf(-rSq * (rSq - lenSq) / ((lenSq - zSq)*(rSq - zSq)*(rSq - zSq))) * glm::vec2(oVec.x, oVec.y);
            }

            if (out_axis_b != nullptr) {
                *out_axis_b = focal_length * sqrtf(fabsf(-rSq * (rSq - lenSq) / ((lenSq - zSq) * (rSq - zSq) * (rSq - lenSq)))) * glm::vec2(-oVec.y, oVec.x);
            }
        }
        else {
            const float radius = focal_length * Radius / sqrtf(zSq - rSq);
            if (out_axis_a != nullptr) {
                *out_axis_a = glm::vec2(radius, 0.0f);
            }
            if (out_axis_b != nullptr) {
                *out_axis_b = glm::vec2(0.0f, radius);
            }
        }
    }

    void UtilitySphere::CalculateSphereProjection(const float & focal_length, const glm::ivec2 & screen_size_in_pixels, glm::vec2 * out_center, glm::vec2 * out_axis_a, glm::vec2 * out_axis_b) const noexcept {

        auto to_screen_pixels = [screen_size_in_pixels](const glm::vec2& v) {
            glm::vec2 result = v;
            result.x *= 1.0f / (screen_size_in_pixels.x / screen_size_in_pixels.y);
            result += glm::vec2(0.50f);
            result *= screen_size_in_pixels;
            return result;
        };

        glm::vec2 center(0.0f), axis_a(0.0f), axis_b(0.0f);
        CalculateSphereProjection(focal_length, &center, &axis_a, &axis_b);
        if (out_center != nullptr) {
            *out_center = to_screen_pixels(center);
        }
        if (out_axis_a != nullptr) {
            *out_axis_a = to_screen_pixels(center + axis_a * 0.50f) - to_screen_pixels(center - axis_a * 0.50f);
        }
        if (out_axis_b != nullptr) {
            *out_axis_b = to_screen_pixels(center + axis_b * 0.50f) - to_screen_pixels(center - axis_b * 0.50f);
        }

    }

    float UtilitySphere::CalcualteProjectedSphereArea(const float & focal_length, const glm::ivec2 & screen_size_in_pixels) const noexcept {
        const float rSq = Radius * Radius;
        const float zSq = Center.z * Center.z;
        const float lenSq = glm::dot(Center, Center);

        const float area = USPHERE_FLOAT_PI * focal_length * focal_length * rSq * sqrtf(fabsf((lenSq - rSq) / (rSq - zSq))) / (rSq - zSq);
        const float aspect_ratio = screen_size_in_pixels.x / screen_size_in_pixels.y;
        return area * screen_size_in_pixels.x * screen_size_in_pixels.y * 0.25f / aspect_ratio;
    }

}