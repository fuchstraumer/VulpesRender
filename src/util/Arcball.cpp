#include "vpr_stdafx.h"
#include "util/Arcball.hpp"
#include "util/Ray.hpp"
#include "util/MatrixUtils.hpp"
#include "util/MathUtils.hpp"
#include "imgui.h"
namespace vulpes {

    ArcballCamera::ArcballCamera(const size_t& pixel_width, const size_t& pixel_height, const float& field_of_view, const UtilitySphere & _sphere) : PerspectiveCamera(pixel_width, pixel_height, 70.0f), sphere(_sphere), initialOrientation(glm::quat()), fromVector(0.0f), toVector(0.0f), constraintAxis(0.0f) {}

    void ArcballCamera::MouseDown(const int & button, const float & x, const float & y) {
        if (button == 0) {
            ImGuiIO& io = ImGui::GetIO();
            glm::ivec2 curr_win_size(static_cast<int>(io.DisplaySize.x), static_cast<int>(io.DisplaySize.y));
            initialMousePos = glm::ivec2(static_cast<int>(io.MousePos.x), static_cast<int>(io.MousePos.y));
            initialOrientation = orientation;
            float tmp;
            mouseToSphere(initialMousePos, curr_win_size, fromVector, tmp);
        }
    }

    void ArcballCamera::MouseUp(const int & button, const float & x, const float & y) {

    }

    void ArcballCamera::MouseDrag(const int & button, const float & x, const float & y) {
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

            orientation = glm::normalize(rotation * initialOrientation);
            viewCached = false;
        }
    }

    void ArcballCamera::MouseScroll(const int & button, const float & delta)
    {
    }

    void ArcballCamera::ResetOrientation() {
        orientation = glm::quat();
    }

    void ArcballCamera::SetArcballSphere(const UtilitySphere & new_arcball_sphere) {
        sphere = new_arcball_sphere;
    }

    void ArcballCamera::mouseToSphere(const glm::ivec2 & pos, const glm::ivec2 & window_size, glm::vec3 & result_vec, float & angle_addition) {
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
            angle_addition = glm::distance(glm::vec2(pos.x, pos.y) / glm::vec2(window_size.x, window_size.y), closest_screenspace_point) / (screen_radius * 3.14159265359f);
        }
    }

}