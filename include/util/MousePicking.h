#ifndef VULPES_VK_MOUSE_PICKING_H
#define VULPES_VK_MOUSE_PICKING_H

#include "vpr_stdafx.h"

namespace vulpes {

    struct MousePicker {

		MousePicker() = default;
		MousePicker(const glm::mat4& projection) : Projection(projection) {}

        glm::vec3 GetRay(const float& x, const float& y, const glm::mat4& view) {
            
            glm::vec4 ray_clip(x, y, -1.0f, 1.0f);
            
            glm::vec4 ray_eye = glm::inverse(Projection) * ray_clip;
            ray_eye.z = -1.0f;
            ray_eye.w = 0.0f;

            glm::vec3 ray_world = (glm::inverse(view) * ray_eye).xyz;
			ray_world = glm::normalize(ray_world);
            
            return ray_world;

        }

        glm::mat4 Projection;
    };

}

#endif //!VULPES_VK_MOUSE_PICKING_H