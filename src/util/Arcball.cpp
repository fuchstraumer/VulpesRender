#include "vpr_stdafx.h"
#include "util/Arcball.hpp"
#include "util/Ray.hpp"
#include "util/MatrixUtils.hpp"
#include "imgui.h"
namespace vulpes {

    Arcball::Arcball(const size_t& pixel_width, const size_t& pixel_height, const float& field_of_view, const UtilitySphere & _sphere) : PerspectiveCamera(pixel_width, pixel_height, 70.0f), sphere(_sphere), initialOrientation(glm::quat()), currOrientation(glm::quat()), fromVector(0.0f), toVector(0.0f), constraintAxis(0.0f) {}

    void Arcball::MouseDown(const int & button, const float & x, const float & y) {
        if (button == 0) {
            ImGuiIO& io = ImGui::GetIO();
            glm::ivec2 curr_win_size(static_cast<int>(io.DisplaySize.x), static_cast<int>(io.DisplaySize.y));
            initialMousePos = glm::ivec2(static_cast<int>(io.MousePos.x), static_cast<int>(io.MousePos.y));
            initialOrientation = currOrientation;
            float tmp;
            mouseToSphere(initialMousePos, curr_win_size, fromVector, tmp);
        }
    }

    void Arcball::MouseUp(const int & button, const float & x, const float & y) {

    }

    void Arcball::MouseDrag(const int & button, const float & x, const float & y) {
        if (button == 0) {
            float addition;
            ImGuiIO& io = ImGui::GetIO();
            glm::ivec2 curr_win_size(static_cast<int>(io.DisplaySize.x), static_cast<int>(io.DisplaySize.y));
            glm::ivec2 pos(static_cast<int>(io.MousePos.x), static_cast<int>(io.MousePos.y));
            mouseToSphere(pos, curr_win_size, toVector, addition);

            glm::quat rotation(fromVector, toVector);
            glm::vec3 axis = glm::axis(rotation);
            float angle = glm::angle(rotation);
            rotation = glm::angleAxis(angle + addition, axis);

            currOrientation = glm::normalize(rotation * initialOrientation);
        }
    }

    void Arcball::MouseScroll(const int & button, const float & delta)
    {
    }

    void Arcball::ResetOrientation() {
        currOrientation = glm::quat();
    }

    const glm::quat & Arcball::GetOrientation() const noexcept {
        return currOrientation;
    }

    void Arcball::SetOrientation(const glm::quat & new_orientation) {
        currOrientation = new_orientation;
    }

    void Arcball::mouseToSphere(const glm::ivec2 & pos, const glm::ivec2 & window_size, glm::vec3 & result_vec, float & angle_addition) {
        float ray_t;
        Ray _ray = GetCameraRay(pos, window_size);
        if (sphere.CheckRayIntersection(_ray, &ray_t)) {
            result_vec = glm::normalize(_ray.CalculatePosition(ray_t) - sphere.Center);
            angle_addition = 0.0f;
        }
        else {
            UtilitySphere cameraspace_sphere(glm::vec3(GetViewMatrix() * glm::vec4(sphere.Center, 1.0f)), sphere.Radius);
            glm::vec2 center, axis_a, axis_b;
            cameraspace_sphere.CalculateSphereProjection(GetFocalLength(), &center, &axis_a, &axis_b);
            glm::vec2 closest_screenspace_point = GetClosestPointOnEllipse(center, axis_a, axis_b, pos);
            Ray new_ray = GetCameraRay(closest_screenspace_point, window_size);
            glm::vec3 closest_point_on_sphere = sphere.ClosestPointToRay(new_ray);
            result_vec = glm::normalize(closest_point_on_sphere - sphere.Center);
            float screen_radius = std::max(glm::length(axis_a), glm::length(axis_b));
            angle_addition = glm::distance(glm::vec2(pos.x, pos.y), closest_screenspace_point) / (screen_radius * 3.14159265359f);
        }
    }

}