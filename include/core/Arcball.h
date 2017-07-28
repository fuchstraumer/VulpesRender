#ifndef VULPES_VK_ARCBALL_H
#define VULPES_VK_ARCBALL_H
#include "vpr_stdafx.h"
#include "Camera.h"

namespace vulpes {

    class Arcball : public cameraBase {
	public:
		Arcball(const size_t& window_width, const size_t& window_height, const float& roll_speed = 1.0f);
		glm::vec3 ToScreenCoordinates(const float& x, const float& y) const;

		glm::mat4 GetViewMatrix() override;
		glm::mat4 GetModelRotationMatrix(const glm::mat4& view_matrix);

		void RegisterMouseEvent(const bool& released, const float& pos_x, const float& pos_y);

	private:

    };


}

#endif //!VULPES_VK_ARCBALL_H