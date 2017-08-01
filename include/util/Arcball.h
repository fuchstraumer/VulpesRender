#ifndef VULPES_VK_ARCBALL_H
#define VULPES_VK_ARCBALL_H
#include "vpr_stdafx.h"
#include "Camera.h"

namespace vulpes {

    class Arcball : public cameraBase {
	public:

		Arcball(const size_t& window_width, const size_t& window_height);


		
		glm::mat4 GetViewMatrix() override;
		glm::mat4 GetModelRotationMatrix(const glm::mat4& view_matrix);

		void UpdateMousePos(const float & x, const float & y);
		void MouseDown(const int& button, const float & x, const float & y) override;
		void MouseUp(const int& button, const float & x, const float & y) override;
		void MouseDrag(const int& button, const float& x, const float& y) override;
		void MouseScroll(const int& button, const float& scroll) override;

		glm::vec3 Center;

	private:

		glm::vec3 toScreenCoordinates(const float& x, const float& y) const;
		glm::vec3 mouseToArcball(const float & x, const float & y) const;
		glm::vec3 constrainToAxis(const glm::vec3 & unconstrained) const;
		void updateVectors();

		size_t windowWidth, windowHeight;
		float rollSpeed, angle;
		glm::vec3 cameraAxis;
		glm::vec3 currPos, prevPos;
		glm::vec3 viewDirection;
		glm::vec3 constraintAxis = glm::vec3(0.0f, 0.0f, 1.0f);
		bool constrained = true;
    };


}

#endif //!VULPES_VK_ARCBALL_H