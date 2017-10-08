#pragma once
#ifndef VULPES_VK_ARCBALL_H
#define VULPES_VK_ARCBALL_H
#include "Camera.hpp"
#include "utilitySphere.hpp"
#include "glm/gtc/quaternion.hpp"
#include "glm/gtx/quaternion.hpp"


namespace vulpes {

    class ArcballCamera : public PerspectiveCamera {
    public:

        ArcballCamera(const size_t& pixel_width, const size_t& pixel_height, const float& field_of_view, const UtilitySphere& _sphere);

        void MouseDown(const int& button, const float& x, const float& y) override;
        void MouseUp(const int& button, const float& x, const float& y) override;
        void MouseDrag(const int& button, const float& x, const float& y) override;
        void MouseScroll(const int& button, const float& delta) override;

        void ResetOrientation();

        void SetArcballSphere(const UtilitySphere& new_arcball_sphere);

    private:

        void mouseToSphere(const glm::ivec2& pos, const glm::ivec2& window_size, glm::vec3& result_vec, float& angle_addition);
        UtilitySphere sphere;
        glm::quat initialOrientation;
        glm::ivec2 initialMousePos;
        glm::vec3 fromVector, toVector, constraintAxis;
        bool axisConstrained = false;
    };
    //inline void Arcball::MouseDrag(const int& button, const float & x, const float & y) {
    //    ImGuiIO& io = ImGui::GetIO();
    //    auto delta = io.MouseDelta;
    //    if (button == 0) {
    //        rotateAround(target, Up, -delta.x * 0.01f);
    //        rotateAround(target, glm::quat_cast(rotation) * Right, -delta.y * 0.01f);
    //    }
    //    else if (button == 1) {
    //        Position.x += (-delta.x * 0.1f);
    //        Position.z += (-delta.y * 0.1f);
    //        target.x += (-delta.x * 0.1f);
    //        target.z += (-delta.y * 0.1f);
    //    }

    //}

}

#endif //!VULPES_VK_ARCBALL_H
