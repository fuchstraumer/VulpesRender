#pragma once
#ifndef CAMERA_H
#define CAMERA_H
// Std. Includes
#include <vector>
#include "glm/ext.hpp"
// GL Includes
#include "stdafx.h"
#include <iostream>

namespace vulpes {

	// Defines several possible options for camera movement. Used as abstraction to stay away from window-system specific input methods
	enum class Direction {
		FORWARD,
		BACKWARD,
		LEFT,
		RIGHT,
		UP,
		DOWN,
	};

	// Default camera values
	const float YAW = -90.0f;
	const float PITCH = 0.0f;
	const float SPEED = 50.0f;
	const float SENSITIVTY = 0.25f;
	const float ZOOM = 45.0f;


	// An abstract camera class that processes input and calculates the corresponding Eular Angles, Vectors and Matrices for use in OpenGL
	class Camera {
	public:
		// Camera Attributes
		glm::vec3 Position;
		glm::vec3 Front;
		glm::vec3 Up;
		glm::vec3 Right;
		glm::vec3 WorldUp;
		// Eular Angles
		float Yaw;
		float Pitch;
		// Camera options
		float MovementSpeed;
		float MouseSensitivity;
		float Zoom;

		// Constructor with vectors
		Camera(glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f), float yaw = YAW, float pitch = PITCH) : Front(glm::vec3(0.0f, 0.0f, -1.0f)), MovementSpeed(SPEED), MouseSensitivity(SENSITIVTY), Zoom(ZOOM)
		{
			this->Position = position;
			this->WorldUp = up;
			this->Yaw = yaw;
			this->Pitch = pitch;
			this->updateCameraVectors();
		}
		// Constructor with scalar values
		Camera(float posX, float posY, float posZ, float upX, float upY, float upZ, float yaw, float pitch) : Front(glm::vec3(0.0f, 0.0f, -1.0f)), MovementSpeed(SPEED), MouseSensitivity(SENSITIVTY), Zoom(ZOOM)
		{
			this->Position = glm::vec3(posX, posY, posZ);
			this->WorldUp = glm::vec3(upX, upY, upZ);
			this->Yaw = yaw;
			this->Pitch = pitch;
			this->updateCameraVectors();
		}

		// Returns the view matrix calculated using Eular Angles and the LookAt Matrix
		glm::mat4 GetViewMatrix()
		{
			return glm::lookAt(this->Position, this->Position + this->Front, this->Up);
		}

		// Processes input received from any keyboard-like input system. Accepts input parameter in the form of camera defined ENUM (to abstract it from windowing systems)
		void ProcessKeyboard(Direction direction, float deltaTime)
		{
			float velocity = this->MovementSpeed * deltaTime;
			if (direction == Direction::FORWARD) {
				this->Position += this->Front * velocity;
			}
			if (direction == Direction::BACKWARD) {
				this->Position -= this->Front * velocity;
			}
			if (direction == Direction::LEFT) {
				this->Position -= this->Right * velocity;
			}
			if (direction == Direction::RIGHT) {
				this->Position += this->Right * velocity;
			}
			if (direction == Direction::UP) {
				this->Position += this->Up * velocity;
			}
			if (direction == Direction::DOWN) {
				this->Position -= this->Up * velocity;
			}
		}

		// Processes input received from a mouse input system. Expects the offset value in both the x and y direction.
		void ProcessMouseMovement(float xoffset, float yoffset, bool constrainPitch = true)
		{
			xoffset *= this->MouseSensitivity;
			yoffset *= this->MouseSensitivity;

			this->Yaw += xoffset;
			this->Pitch -= yoffset;

			// Make sure that when pitch is out of bounds, screen doesn't get flipped
			if (constrainPitch)
			{
				if (this->Pitch > 89.0f)
					this->Pitch = 89.0f;
				if (this->Pitch < -89.0f)
					this->Pitch = -89.0f;
			}

			// Update Front, Right and Up Vectors using the updated Eular angles
			this->updateCameraVectors();
		}

		// Processes input received from a mouse scroll-wheel event. Only requires input on the vertical wheel-axis
		void ProcessMouseScroll(float yoffset)
		{
			if (this->Zoom >= 1.0f && this->Zoom <= 45.0f)
				this->Zoom -= yoffset;
			if (this->Zoom <= 1.0f)
				this->Zoom = 1.0f;
			if (this->Zoom >= 45.0f)
				this->Zoom = 45.0f;
		}

	private:
		// Calculates the front vector from the Camera's (updated) Eular Angles
		void updateCameraVectors()
		{
			// Calculate the new Front vector
			glm::vec3 front;
			front.x = cos(glm::radians(this->Yaw)) * cos(glm::radians(this->Pitch));
			front.y = sin(glm::radians(this->Pitch));
			front.z = sin(glm::radians(this->Yaw)) * cos(glm::radians(this->Pitch));
			this->Front = glm::normalize(front);
			// Also re-calculate the Right and Up vector
			this->Right = glm::normalize(glm::cross(this->Front, this->WorldUp));  // Normalize the vectors, because their length gets closer to 0 the more you look up or down which results in slower movement.
			this->Up = glm::normalize(glm::cross(this->Right, this->Front));
		}
	};
}
#endif // !CAMERA_H