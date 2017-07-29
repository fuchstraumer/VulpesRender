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


	private:

		glm::vec3 toScreenCoordinates(const float& x, const float& y) const;

		size_t windowWidth, windowHeight;
		float rollSpeed, angle;
		glm::vec3 cameraAxis;

    };


}

#endif //!VULPES_VK_ARCBALL_H