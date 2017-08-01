#include "vpr_stdafx.h"
#include "util/Arcball.h"

namespace vulpes {

	Arcball::Arcball(const size_t & window_width, const size_t & window_height) : cameraBase(glm::vec3(0.0f, 0.0f, -10.0f)), windowWidth(window_width), windowHeight(window_height), 
		angle(0.0f), cameraAxis(0.0f, 1.0f, 0.0f), rollSpeed(0.2f), prevPos(toScreenCoordinates(windowWidth / 2, windowHeight / 2)) {
		LastView = glm::lookAt(Position, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		viewDirection = glm::vec3(0.0f, 0.0f, -1.0f);
	}

	void Arcball::updateVectors() {
		// translate to rotation center
		glm::mat4 translate = glm::translate(glm::mat4(1.0f), Position);
		glm::mat4 inv_translate = glm::translate(glm::mat4(1.0f), -1.0f * Position);
		glm::mat3 view_to_world = glm::mat3(glm::inverse(LastView));
		glm::vec3 world_axis = view_to_world * cameraAxis;
		glm::quat rotation = glm::angleAxis(-angle, glm::normalize(world_axis));
		glm::mat4 final = translate * glm::mat4_cast(rotation) * inv_translate;

		Position = glm::vec3(final * glm::vec4(Position.x, Position.y, Position.z, 1.0f));
		viewDirection = rotation * viewDirection;
		WorldUp = rotation * WorldUp;

		Right = glm::normalize(glm::cross(viewDirection, WorldUp));
		angle = 0.0f;
	}

	glm::mat4 Arcball::GetViewMatrix() {
		updateVectors();
		return glm::lookAt(Position, Position + viewDirection, Up);
	}

	glm::mat4 Arcball::GetModelRotationMatrix(const glm::mat4 & view_matrix) {
		glm::vec3 axis = glm::inverse(glm::mat3(view_matrix)) * cameraAxis;
		return glm::rotate(view_matrix, angle * rollSpeed, axis);
	}

	void Arcball::MouseUp(const int& button, const float& x, const float& y) {}

	void Arcball::UpdateMousePos(const float& x, const float& y) {}

	void Arcball::MouseDown(const int& button, const float& x, const float& y) {
		if (button == 0) {
			prevPos = toScreenCoordinates(x, y);
		}
		if (button == 1) {
			prevMouse = glm::vec2(x, y);
		}	
	}

	void Arcball::MouseDrag(const int& button, const float & x, const float & y) {
		if (button == 0) {
			currPos = toScreenCoordinates(x, y);
			angle = std::acos(std::min(1.0f, glm::dot(prevPos, currPos)));
			angle *= 0.1f;
			cameraAxis = glm::cross(prevPos, currPos);
		}
		else if (button == 1) {
			glm::vec2 d_mouse = glm::vec2(x, y) - prevMouse;
			Position.xz += (d_mouse * 0.01f);	
		}

	}

	void Arcball::MouseScroll(const int& button, const float& scroll) {
		Position.y += 0.2f * scroll;
	}

	glm::vec3 Arcball::toScreenCoordinates(const float & x, const float & y) const {

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

	glm::vec3 Arcball::mouseToArcball(const float&x , const float& y) const {
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
	
	glm::vec3 Arcball::constrainToAxis(const glm::vec3& unconstrained) const {
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
