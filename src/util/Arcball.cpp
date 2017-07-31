#include "vpr_stdafx.h"
#include "util/Arcball.h"

namespace vulpes {

	Arcball::Arcball(const size_t & window_width, const size_t & window_height) : windowWidth(window_width), windowHeight(window_height), angle(0.0f), cameraAxis(0.0f, 1.0f, 0.0f), 
		rollSpeed(0.2f), prevPos(toScreenCoordinates(windowWidth / 2, windowHeight / 2)) {
		LastView = glm::lookAt(glm::vec3(0.0f, 0.0f, -10.0f), glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	}

	glm::mat4 Arcball::GetViewMatrix() {
		return LastView;
	}

	glm::mat4 Arcball::GetModelRotationMatrix(const glm::mat4 & view_matrix) {
		glm::vec3 axis = glm::inverse(glm::mat3(view_matrix)) * cameraAxis;
		return glm::rotate(view_matrix, angle * rollSpeed, axis);
	}

	void Arcball::MouseUp(const float& x, const float& y) {}

	void Arcball::UpdateMousePos(const float& x, const float& y) {}

	void Arcball::MouseDown(const float& x, const float& y) {
		prevPos = toScreenCoordinates(x, y);
	}

	void Arcball::MouseDrag(const float & x, const float & y) {
		currPos = toScreenCoordinates(x, y);
		angle = std::acos(std::min(1.0f, glm::dot(prevPos, currPos)));
		cameraAxis = glm::cross(prevPos, currPos);
		LastView = glm::rotate(LastView, angle * rollSpeed, cameraAxis);
	}

	void Arcball::MouseScroll(const float& scroll) {}

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
		screen_coord.y *= -1.0f;
		return screen_coord;
	}

}
