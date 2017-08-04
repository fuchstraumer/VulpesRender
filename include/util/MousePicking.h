#ifndef VULPES_VK_MOUSE_PICKING_H
#define VULPES_VK_MOUSE_PICKING_H

#include "vpr_stdafx.h"
#include "core/Instance.h"
namespace vulpes {

    struct MousePicker {

		MousePicker() = default;
		MousePicker(const glm::mat4& projection) : Projection(projection) { }

        glm::vec3 GetRay(const float& x, const float& y, const glm::mat4& view) {
			glm::vec4 viewport = glm::vec4(0.0f, 0.0f, 
				static_cast<float>(Instance::VulpesInstanceConfig.DefaultWindowSize.extent.width), 
				static_cast<float>(Instance::VulpesInstanceConfig.DefaultWindowSize.extent.height));

			glm::vec3 v0 = glm::unProject(glm::vec3(x, y, 0.0f), view, Projection, viewport);
			glm::vec3 v1 = glm::unProject(glm::vec3(x, y, 1.0f), view, Projection, viewport);

			return glm::normalize(v1 - v0);
        }

        glm::mat4 Projection;
    };

}

#endif //!VULPES_VK_MOUSE_PICKING_H