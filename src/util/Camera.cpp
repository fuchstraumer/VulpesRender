#include "vpr_stdafx.h"
#include "util/Camera.h"
#include "core/Instance.h"
namespace vulpes {

	cameraBase::cameraBase(const glm::vec3 & position, const glm::vec3 & up, const glm::vec3& front) : Position(position), Up(up), Front(front), WorldUp(0.0f, 1.0f, 0.0f) {}

	void cameraBase::ProcessKeyboard(const Direction & dir, const float & delta_time) {
		float velocity = Instance::VulpesInstanceConfig.MovementSpeed * delta_time;
		switch (dir) {
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

	Camera::Camera(const glm::vec3& position, const glm::vec3& up, const float& yaw, const float& pitch) : cameraBase(position, up), Zoom(ZOOM), Yaw(yaw), Pitch(pitch) {
		updateCameraVectors();
	}

	void Camera::MouseDrag(const int& button, const float & xoffset, const float & yoffset) {
		
	}

	void Camera::MouseScroll(const int& button, const float & yoffset) {
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

	void Camera::UpdateMousePos(const float & x, const float & y) {
		float x_offset = x * Instance::VulpesInstanceConfig.MouseSensitivity;
		float y_offset = y * Instance::VulpesInstanceConfig.MouseSensitivity;

		Yaw += x_offset;
		Pitch -= y_offset;

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

	void Camera::MouseDown(const int& button, const float & x, const float & y)
	{
	}

	void Camera::MouseUp(const int& button, const float & x, const float & y)
	{
	}

	glm::mat4 Camera::GetViewMatrix() {
		return glm::lookAt(Position, Position + Front, Up);
	}

	void Camera::updateCameraVectors() {

		glm::vec3 front;
		front.x = cos(glm::radians(Yaw)) * cos(glm::radians(Pitch));
		front.y = sin(glm::radians(Pitch));
		front.z = sin(glm::radians(Yaw)) * cos(glm::radians(Pitch));
		Front = glm::normalize(front);

		Right = glm::normalize(glm::cross(Front, WorldUp));
		Up = glm::normalize(glm::cross(Right, Front));

	}

}
