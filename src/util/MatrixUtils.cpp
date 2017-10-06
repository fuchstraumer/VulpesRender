#include "vpr_stdafx.h"
#include "util/MatrixUtils.hpp"

namespace vulpes {

    glm::mat4 AlignZAxisWithTarget(glm::vec3 & target_direction, glm::vec3 & up_direction) {

        if (glm::length2(target_direction) == 0.0f) {
            target_direction = glm::vec3(0.0f, 0.0f, 1.0f);
        }

        if (glm::length2(up_direction) == 0.0f) {
            up_direction = glm::vec3(0.0f, 1.0f, 0.0f);
        }

        if (glm::length2(glm::cross(up_direction, target_direction)) == 0.0f) {
            up_direction = glm::cross(target_direction, glm::vec3(1.0f, 0.0f, 0.0f));
        }
        if (glm::length2(up_direction) == 0.0f) {
            up_direction = glm::cross(target_direction, glm::vec3(0.0f, 0.0f, 1.0f));
        }

        glm::vec3 target_perspective_dir = glm::cross(up_direction, target_direction);
        glm::vec3 target_up_dir = glm::cross(target_direction, target_perspective_dir);

        std::array<glm::vec3, 3> matrix_rows{};
        matrix_rows[0] = glm::normalize(target_perspective_dir);
        matrix_rows[1] = glm::normalize(target_up_dir);
        matrix_rows[2] = glm::normalize(target_direction);

        const std::array<float, 16> result{
            matrix_rows[0].x, matrix_rows[0].y, matrix_rows[0].z, 0.0f,
            matrix_rows[1].x, matrix_rows[1].y, matrix_rows[1].z, 0.0f,
            matrix_rows[2].x, matrix_rows[2].y, matrix_rows[2].z, 0.0f,
            0.0f,             0.0f,             0.0f, 1.0f,
        };

        return glm::make_mat4(result.data());
    }


}