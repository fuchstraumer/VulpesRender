#pragma once
#ifndef VULPES_VK_ARCBALL_H
#define VULPES_VK_ARCBALL_H
#include "Camera.hpp"
#include "imgui.h"
namespace vulpes {

    class Arcball : public cameraBase {
	public:

		Arcball(const size_t& window_width, const size_t& window_height) : cameraBase(glm::vec3(0.0f, 0.0f, -10.0f)), windowWidth(window_width), windowHeight(window_height),
            angle(0.0f), cameraAxis(0.0f, 1.0f, 0.0f), rollSpeed(0.2f), prevPos(toScreenCoordinates(static_cast<float>(windowWidth / 2), static_cast<float>(windowHeight / 2))) {
            viewDirection = glm::vec3(0.0f, 0.0f, -1.0f);
            target = Position + viewDirection;
            LastView = glm::lookAt(Position, Position + viewDirection, glm::vec3(0.0f, 1.0f, 0.0f));
        }

		
		glm::mat4 GetViewMatrix();
		glm::mat4 GetModelRotationMatrix(const glm::mat4& view_matrix);

		void UpdateMousePos(const float & x, const float & y);
		void MouseDown(const int& button, const float & x, const float & y) override;
		void MouseUp(const int& button, const float & x, const float & y) override;
		void MouseDrag(const int& button, const float& x, const float& y) override;
		void MouseScroll(const int& button, const float& scroll) override;

		void SetTarget(const glm::vec3& new_target);

		void RotateUp(const float& delta_time);
		void RotateDown(const float& delta_time);
		void RotateLeft(const float& delta_time);
		void RotateRight(const float& delta_time);

		void TranslateUp(const float& delta_time);
		void TranslateDown(const float& delta_time);

		glm::vec3 Center;

	private:

		glm::vec3 toScreenCoordinates(const float& x, const float& y) const;
		glm::vec3 mouseToArcball(const float & x, const float & y) const;
		glm::vec3 constrainToAxis(const glm::vec3 & unconstrained) const;
		void updateVectors();
		void rotateAround(const glm::vec3& pt, const glm::vec3& axis, const float& angle);

		size_t windowWidth, windowHeight;
		float rollSpeed, angle;
		glm::vec3 target;
		glm::vec3 cameraAxis;
		glm::vec3 currPos, prevPos;
		glm::vec3 viewDirection;
		glm::vec3 constraintAxis = glm::vec3(0.0f, 0.0f, 0.0f);
		bool constrained = true;
    };

    inline void Arcball::updateVectors() {
        auto rot = glm::quat_cast(rotation);
        Up = rot * Up;
        Right = rot * Right;
        angle = 0.0f;
    }

    inline glm::mat4 Arcball::GetViewMatrix() {
        updateVectors();
        return glm::lookAt(Position, target, Up);
    }

    inline glm::mat4 Arcball::GetModelRotationMatrix(const glm::mat4 & view_matrix) {
        glm::vec3 axis = glm::inverse(glm::mat3(view_matrix)) * cameraAxis;
        return glm::rotate(view_matrix, angle * rollSpeed, axis);
    }

    inline void Arcball::MouseUp(const int& button, const float& x, const float& y) {

    }

    inline void Arcball::UpdateMousePos(const float& x, const float& y) {}

    inline void Arcball::MouseDown(const int& button, const float& x, const float& y) {
        if (button == 0) {
            prevPos = toScreenCoordinates(x, y);
        }
        if (button == 1) {
            prevMouse = glm::vec2(x, y);
        }
    }

    inline void Arcball::rotateAround(const glm::vec3& pt, const glm::vec3& axis, const float& _angle) {

        glm::vec3 dir = Position - pt;
        auto quat = glm::angleAxis(_angle, axis);
        dir = quat * dir;
        Position = pt + dir;
        rotation = glm::mat4_cast(quat) * rotation;

    }

    inline void Arcball::MouseDrag(const int& button, const float & x, const float & y) {
        ImGuiIO& io = ImGui::GetIO();
        auto delta = io.MouseDelta;
        if (button == 0) {
            rotateAround(target, Up, -delta.x * 0.01f);
            rotateAround(target, glm::quat_cast(rotation) * Right, -delta.y * 0.01f);
        }
        else if (button == 1) {
            Position.x += (-delta.x * 0.1f);
            Position.z += (-delta.y * 0.1f);
            target.x += (-delta.x * 0.1f);
            target.z += (-delta.y * 0.1f);
        }

    }

    inline void Arcball::MouseScroll(const int& button, const float& scroll) {
        glm::vec3 target_dir = target - Position;
        Position += (target_dir * scroll * 0.1f);
    }

    inline void Arcball::SetTarget(const glm::vec3 & new_target) {
        target = new_target;
    }

    inline void Arcball::RotateUp(const float& delta_time) {
        Position -= glm::normalize(glm::cross(target, Right)) * (0.1f * delta_time);
    }

    inline void Arcball::RotateDown(const float& delta_time) {
        Position += glm::normalize(glm::cross(target, Right)) * (0.1f * delta_time);
    }

    inline void Arcball::RotateRight(const float& delta_time) {
        Position -= glm::normalize(glm::cross(target, Up)) * (0.3f * delta_time);
    }

    inline void Arcball::RotateLeft(const float& delta_time) {
        Position += glm::normalize(glm::cross(target, Up)) * (0.3f * delta_time);
    }

    inline void Arcball::TranslateDown(const float& delta_time) {
        Position.y -= 0.1f * delta_time;
        target.y -= 0.1f * delta_time;
    }

    inline void Arcball::TranslateUp(const float& delta_time) {
        Position.y += 0.1f * delta_time;
        target.y += 0.1f * delta_time;
    }

    inline glm::vec3 Arcball::toScreenCoordinates(const float & x, const float & y) const {

        glm::vec3 screen_coord(0.0f);

        screen_coord.x = (2 * x - windowWidth) / windowWidth;
        screen_coord.y = (2 * y - windowHeight) / windowHeight;

        screen_coord.x = glm::clamp(screen_coord.x, -1.0f, 1.0f);
        screen_coord.y = glm::clamp(screen_coord.y, -1.0f, 1.0f);

        float length_sq = (screen_coord.x * screen_coord.x) + (screen_coord.y * screen_coord.y);

        if (length_sq <= 1.0f) {
            screen_coord.z = sqrtf(1.0f - length_sq);
        }
        else {
            screen_coord = glm::normalize(screen_coord);
        }

        return screen_coord;
    }

    inline glm::vec3 Arcball::mouseToArcball(const float&x, const float& y) const {
        glm::vec3 p = glm::vec3(1.0f * x / static_cast<float>(windowWidth) * 2.0f - 1.0f,
            1.0f * y / static_cast<float>(windowHeight) * 2.0f - 1.0f, 0.0f);
        p.y *= -1.0f;

        float p_sq = p.x*p.x + p.y*p.y;
        if (p_sq < 1.0f) {
            p.z = sqrtf(1.0f - p_sq);
        }
        else {
            p = normalize(p);
        }

        return p;
    }

    inline glm::vec3 Arcball::constrainToAxis(const glm::vec3& unconstrained) const {
        float norm;
        glm::vec3 on_plane = unconstrained - constraintAxis * glm::dot(constraintAxis, unconstrained);
        norm = glm::length(on_plane);
        if (norm > 0.0f) {
            if (on_plane.z < 0.0f) {
                on_plane *= -1.0f;
            }
            return (on_plane * (1.0f / std::sqrtf(norm)));
        }

        if (glm::dot(constraintAxis, glm::vec3(0.0f, 0.0f, 1.0f)) < 1e-4f) {
            on_plane = glm::vec3(1.0f, 0.0f, 0.0f);
        }
        else {
            on_plane = glm::normalize(glm::vec3(-constraintAxis.y, constraintAxis.x, 0.0f));
        }

        return on_plane;
    }


}

#endif //!VULPES_VK_ARCBALL_H