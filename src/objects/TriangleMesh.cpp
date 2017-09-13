#include "objects/TriangleMesh.hpp"

namespace vulpes {

    uint32_t TriangleMesh::AddVertex(const vertex_t& vertex) noexcept {
        
    }

    void TriangleMesh::updateModelMatrix() {
        glm::mat4 scale_matrix = glm::scale(glm::mat4(1.0f), scale);
        glm::mat4 translation_matrix = glm::translate(glm::mat4(1.0f), position);
        glm::mat4 rotation_x_matrix = glm::rotate(glm::mat4(1.0f), rotation.x, glm::vec3(1.0f, 0.0f, 0.0f));
        glm::mat4 rotation_y_matrix = glm::rotate(glm::mat4(1.0f), rotation.y, glm::vec3(0.0f, 1.0f, 0.0f));
        glm::mat4 rotation_z_matrix = glm::rotate(glm::mat4(1.0f), rotation.z, glm::vec3(0.0f, 0.0f, 1.0f));
        model = translation_matrix * rotation_x_matrix * rotation_y_matrix * rotation_z_matrix * scale_matrix;
    }

}