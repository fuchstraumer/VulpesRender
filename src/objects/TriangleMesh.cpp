#include "objects/TriangleMesh.hpp"

namespace vulpes {

    TriangleMesh::TriangleMesh(const glm::vec3& _position, const glm::vec3& _scale, const glm::vec3& _rotation) : position(_position), scale(_scale), rotation(_rotation) {}

    TriangleMesh::~TriangleMesh() {
        DestroyVulkanObjects();
    }

    uint32_t TriangleMesh::AddVertex(vertex_t vertex) noexcept {
        vertices.push_back(std::move(vertex));
        return static_cast<uint32_t>(vertices.size() - 1);
    }

    void TriangleMesh::AddIndex(uint32_t index) noexcept {
        indices.push_back(std::move(index));
    }

    void TriangleMesh::AddTriangle(uint32_t i0, uint32_t i1, uint32_t i2) noexcept {
        indices.insert(indices.cend(), { std::move(i0), std::move(i1), std::move(i2) });
    }

    const vertex_t& TriangleMesh::GetVertex(const uint32_t& index) const {
        return vertices[index];
    }

    void TriangleMesh::ReserveVertices(const size_t& reserve_amount) noexcept {
        vertices.reserve(reserve_amount);
    }

    void TriangleMesh::ReserveIndices(const size_t& reserve_amount) noexcept {
        indices.reserve(reserve_amount);
    }

    size_t TriangleMesh::NumVertices() const noexcept {
        return vertices.size();
    }

    size_t TriangleMesh::NumIndices() const noexcept {
        return indices.size();
    }

    void TriangleMesh::CreateBuffers(const Device* dvc) {
        
        device = dvc;

        vbo = std::make_unique<Buffer>(device);
        ebo = std::make_unique<Buffer>(device);

        vbo->CreateBuffer(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, sizeof(vertex_t) * NumVertices());
        ebo->CreateBuffer(VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, sizeof(uint32_t) * NumIndices());
        
    }

    void TriangleMesh::RecordTransferCommands(const VkCommandBuffer& transfer_cmd) {
        
        vbo->CopyTo(vertices.data(), transfer_cmd, sizeof(vertex_t) * NumVertices(), 0);
        ebo->CopyTo(indices.data(), transfer_cmd, sizeof(uint32_t) * NumIndices(), 0);
    
    }

    void TriangleMesh::Render(const VkCommandBuffer& draw_cmd) const noexcept {
    
        constexpr static VkDeviceSize offsets[1] { 0 };
        vkCmdBindVertexBuffers(draw_cmd, 0, 1, &vbo->vkHandle(), offsets);
        vkCmdBindIndexBuffer(draw_cmd, ebo->vkHandle(), 0, VK_INDEX_TYPE_UINT32);
        vkCmdDrawIndexed(draw_cmd, static_cast<uint32_t>(NumIndices()), 1, 0, 0, 0);

    }

    void TriangleMesh::DestroyVulkanObjects() {
        
        ebo.reset();
        vbo.reset();

    }

    void TriangleMesh::FreeCpuData() {

        vertices.clear();
        vertices.shrink_to_fit();
        indices.clear();
        indices.shrink_to_fit();

    }

    const glm::mat4& TriangleMesh::GetModelMatrix() const noexcept {
        return model;
    }

    void TriangleMesh::SetModelMatrix(const glm::mat4& updated_model) {
        model = updated_model;
    }

    void TriangleMesh::UpdatePosition(const glm::vec3& new_position) {
        position = new_position;
    }

    void TriangleMesh::UpdateScale(const glm::vec3& new_scale) {
        scale = new_scale;
    }

    void TriangleMesh::UpdateRotation(const glm::vec3& new_rotation) {
        rotation = new_rotation;
    }

    const glm::vec3& TriangleMesh::GetPosition() const noexcept {
        return position;
    }

    const glm::vec3& TriangleMesh::GetScale() const noexcept {
        return scale;
    }

    const glm::vec3& TriangleMesh::GetRotation() const noexcept {
        return rotation;
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