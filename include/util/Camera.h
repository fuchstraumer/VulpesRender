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

		cameraBase(const glm::vec3& position = glm::vec3(0.0f), const glm::vec3& up = glm::vec3(0.0f, 1.0f, 0.0f), const glm::vec3& front = glm::vec3(0.0f, 0.0f, -1.0f));
		virtual ~cameraBase() = default;
		virtual glm::mat4 GetViewMatrix() = 0;

		virtual void ProcessKeyboard(const Direction& dir, const float& delta_time);
		virtual void MouseDrag(const int& button, const float& x_offset, const float& y_offset) = 0;
		virtual void MouseScroll(const int& button, const float& y_scroll) = 0;
		virtual void MouseDown(const int& button, const float& x, const float& y) = 0;
		virtual void MouseUp(const int& button, const float& x, const float& y) = 0;
		virtual void UpdateMousePos(const float& x, const float& y) = 0;

		glm::vec3 Position;
		glm::vec3 Front, Up, WorldUp, Right;
		glm::mat4 LastView;

	};

	// An abstract camera class that processes input and calculates the corresponding Eular Angles, Vectors and Matrices for use in OpenGL
	class Camera : public cameraBase {
	public:

		float Yaw;
		float Pitch;
		float Zoom;

		Camera(const glm::vec3& position = glm::vec3(0.0f, 0.0f, 0.0f), const glm::vec3& up = glm::vec3(0.0f, 1.0f, 0.0f), const float& yaw = YAW, const float& pitch = PITCH);

		glm::mat4 GetViewMatrix() override;
		void MouseDrag(const int& button, const float& xoffset, const float& yoffset) override;
		void MouseScroll(const int& button, const float& yoffset) override;
		void UpdateMousePos(const float& x, const float& y) override;
		void MouseDown(const int& button, const float& x, const float& y) override;
		void MouseUp(const int& button, const float& x, const float& y) override;

	private:

		void updateCameraVectors();
	};
}
#endif // !VULPES_CAMERA_H