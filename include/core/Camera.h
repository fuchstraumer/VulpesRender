#pragma once
#ifndef VULPES_CAMERA_H
#define VULPES_CAMERA_H
#include "vpr_stdafx.h"

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

		virtual ~cameraBase();
		virtual glm::mat4 GetViewMatrix() = 0;

	};

	// An abstract camera class that processes input and calculates the corresponding Eular Angles, Vectors and Matrices for use in OpenGL
	class Camera : public cameraBase {
	public:

		glm::vec3 Position;
		glm::vec3 Front;
		glm::vec3 Up;
		glm::vec3 Right;

		float Yaw;
		float Pitch;

		float MovementSpeed;
		float MouseSensitivity;
		float Zoom;

		Camera(glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f), float yaw = YAW, float pitch = PITCH) : Front(glm::vec3(0.0f, 0.0f, -1.0f)), MovementSpeed(SPEED), MouseSensitivity(SENSITIVTY), Zoom(ZOOM),
			Position(position), Up(up), Yaw(yaw), Pitch(pitch) {
			updateCameraVectors();
		}

		glm::mat4 GetViewMatrix() override {
			return glm::lookAt(Position, Position + Front, Up);
		}

		void ProcessKeyboard(const Direction& direction, const float& deltaTime) {
			float velocity = MovementSpeed * deltaTime;
			switch (direction) {
			case Direction::FORWARD:
				Position += Front * velocity;
				break;
			case Direction::BACKWARD:
				Position -= Front * velocity;
				break;
			case Direction::LEFT:
				Position -= Right * velocity;
				break;
			case Direction::RIGHT:
				Position += Right * velocity;
				break;
			case Direction::UP:
				Position += Up * velocity;
				break;
			case Direction::DOWN:
				Position -= Up * velocity;
				break;
			}
		}

		// Processes input received from a mouse input system. Expects the offset value in both the x and y direction.
		void ProcessMouseMovement(const float& xoffset, const float& yoffset)
		{
			float x_offset = xoffset * MouseSensitivity;
			float y_offset = yoffset * MouseSensitivity;

			Yaw += x_offset;
			Pitch -= y_offset;

			// Make sure that when pitch is out of bounds, screen doesn't get flipped

			{
				if (Pitch > 89.0f)
					Pitch = 89.0f;
				if (Pitch < -89.0f)
					Pitch = -89.0f;
			}

			// Update Front, Right and Up Vectors using the updated Eular angles
			updateCameraVectors();
		}

		void ProcessMouseScroll(const float& yoffset) {
			if (Zoom >= 1.0f && Zoom <= 45.0f) {
				Zoom -= yoffset;
			}
			else if (Zoom <= 1.0f) {
				Zoom = 1.0f;
			}
			else if (Zoom >= 45.0f) {
				Zoom = 45.0f;
			}
		}

	private:

		void updateCameraVectors() {

			glm::vec3 front;
			front.x = cos(glm::radians(Yaw)) * cos(glm::radians(Pitch));
			front.y = sin(glm::radians(Pitch));
			front.z = sin(glm::radians(Yaw)) * cos(glm::radians(Pitch));
			Front = glm::normalize(front);

			Right = glm::normalize(glm::cross(Front, Up));  
			Up = glm::normalize(glm::cross(Right, Front));

		}
	};
}
#endif // !VULPES_CAMERA_H