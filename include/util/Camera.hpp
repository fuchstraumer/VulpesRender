#pragma once
#ifndef VULPES_CAMERA_H
#define VULPES_CAMERA_H
#include "vpr_stdafx.h"
#include "core/Instance.hpp"
#include "glm/gtc/quaternion.hpp"

namespace vulpes {

	// Defines several possible options for camera movement. Used as abstraction to stay away from window-system specific input methods
	enum class Direction : uint8_t {
		FORWARD,
		BACKWARD,
		LEFT,
		RIGHT,
		UP,
		DOWN,
	};

	// Default camera values
	constexpr static float YAW = -90.0f;
	constexpr static float PITCH = 0.0f;
	constexpr static float SPEED = 50.0f;
	constexpr static float SENSITIVTY = 0.25f;
	constexpr static float ZOOM = 45.0f;

	class cameraBase {
	public:

		cameraBase(const glm::vec3& position = glm::vec3(0.0f), const glm::vec3& up = glm::vec3(0.0f, 1.0f, 0.0f), const glm::vec3& front = glm::vec3(0.0f, 0.0f, -1.0f)) : Position(position), Up(up), Front(front), WorldUp(0.0f, 1.0f, 0.0f) {}
		virtual ~cameraBase() = default;

		virtual void ProcessKeyboard(const Direction& dir, const float& delta_time) {
			float velocity = Instance::VulpesInstanceConfig.MovementSpeed * delta_time;
			switch (dir) {
			case Direction::FORWARD:
				translate(Front, velocity);
				break;
			case Direction::BACKWARD:
				translate(-Front, velocity);
				break;
			case Direction::LEFT:
				translate(-Right, velocity);
				break;
			case Direction::RIGHT:
				translate(Right, velocity);
				break;
			case Direction::UP:
				translate(Up, velocity);
				break;
			case Direction::DOWN:
				translate(-Up, velocity);
				break;
			}
		}

		virtual void MouseDrag(const int& button, const float& x_offset, const float& y_offset) = 0;
		virtual void MouseScroll(const int& button, const float& y_scroll) = 0;
		virtual void MouseDown(const int& button, const float& x, const float& y) = 0;
		virtual void MouseUp(const int& button, const float& x, const float& y) = 0;
		virtual void UpdateMousePos(const float& x, const float& y) = 0;

		glm::vec3 Position;
		glm::vec3 Front, Up, WorldUp, Right;
		glm::mat4 LastView, rotation;
		glm::vec2 prevMouse;

	protected:

		virtual void translate(const glm::vec3& dir, const float& v) {
			Position += dir * v;
		}
	};

	// An abstract camera class that processes input and calculates the corresponding Eular Angles, Vectors and Matrices for use in OpenGL
	class Camera : public cameraBase {
	public:

		float Yaw;
		float Pitch;
		float Zoom;
		glm::mat4 view;

		Camera(const glm::vec3& position = glm::vec3(0.0f, 0.0f, 0.0f), const glm::vec3& up = glm::vec3(0.0f, 1.0f, 0.0f), const float& yaw = YAW, const float& pitch = PITCH) : cameraBase(position, up), Zoom(ZOOM), Yaw(yaw), Pitch(pitch) {
			updateCameraVectors();
		}

		glm::mat4 GetViewMatrix() const noexcept {
			return glm::lookAt(Position, Position + Front, Up);
		}

		void MouseDrag(const int& button, const float& xoffset, const float& yoffset) override {}

		void MouseScroll(const int& button, const float& yoffset) override {}

		void UpdateMousePos(const float& x, const float& y) override {
			float x_offset = x * Instance::VulpesInstanceConfig.MouseSensitivity;
			float y_offset = y * Instance::VulpesInstanceConfig.MouseSensitivity;

			Yaw += x_offset;
			Pitch += y_offset;

			// Make sure that when pitch is out of bounds, screen doesn't get flipped

			{
				if (Pitch > 89.0f) {
					Pitch = 89.0f;
				}
				if (Pitch < -89.0f) {
					Pitch = -89.0f;
				}
			}

			// Update Front, Right and Up Vectors using the updated Eular angles
			updateCameraVectors();

		}

		void MouseDown(const int& button, const float& x, const float& y) override {}

		void MouseUp(const int& button, const float& x, const float& y) override {}


	private:

		void updateCameraVectors() {

			glm::vec3 front;
			front.x = cos(glm::radians(Yaw)) * cos(glm::radians(Pitch));
			front.y = sin(glm::radians(Pitch));
			front.z = sin(glm::radians(Yaw)) * cos(glm::radians(Pitch));
			Front = glm::normalize(front);

			Right = glm::normalize(glm::cross(Front, WorldUp));
			Up = glm::normalize(glm::cross(Right, Front));

		}
	};

	class FreeCamera : public cameraBase {
	public:
		FreeCamera(const glm::vec3& _pos) : cameraBase(_pos) {};
		
		glm::mat4 GetViewMatrix() const noexcept {
			return glm::translate(glm::mat4_cast(orientation), Position);
		}



	protected:

		glm::quat orientation;
	};
}

#endif // !VULPES_CAMERA_H