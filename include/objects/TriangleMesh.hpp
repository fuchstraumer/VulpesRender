#ifndef VULPESRENDER_TRIANGLE_MESH_HPP
#define VULPESRENDER_TRIANGLE_MESH_HPP

#include "vpr_stdafx.h"
#include "resource/Buffer.hpp"

namespace vulpes {

    struct vertex_t {
        glm::vec3 pos = glm::vec3(0.0f);
        glm::vec3 normal = glm::vec3(0.0f);
        glm::vec3 tangent = glm::vec3(0.0f);
        glm::vec3 bitangent = glm::vec3(0.0f);
        glm::vec2 uv = glm::vec2(0.0f);
    };

    class TriangleMesh {
        TriangleMesh(const TriangleMesh&) = delete;
        TriangleMesh& operator=(const TriangleMesh&) = delete;
    public:

        TriangleMesh();
        ~TriangleMesh();

        uint32_t AddVertex(const vertex_t& vert) noexcept;
        uint32_t AddVertex(vertex_t&& vert) noexcept;

        void AddIndex(const uint32_t& idx) noexcept;
        void AddTriangle(const uint32_t& i0, const uint32_t& i1, const uint32_t& i2) noexcept;

        const vertex_t& GetVertex(const uint32_t& index) const;

        void ReserveVertices(const size_t& reserve_amount) noexcept;
        void ReserveIndices(const size_t& reserve_amount) noexcept;

        size_t NumVertices() const noexcept;
        size_t NumIndices() const noexcept;

        void CreateBuffers(const Device* dvc);

        void RecordTransferCommands(const VkCommandBuffer& transfer_cmd);
        void Render(const VkCommandBuffer& draw_cmd);

        void DestroyVulkanObjects();
        void FreeCpuData();

        const glm::mat4& GetModelMatrix() const noexcept;
        void SetModelMatrix(const glm::mat4& updated_model);

        void UpdatePosition(const glm::vec3& new_position);
        void UpdateScale(const glm::vec3& new_scale);
        void UpdateRotation(const glm::vec3& new_rotation);

        const glm::vec3& GetPosition() const noexcept;
        const glm::vec3& GetScale() const noexcept;
        const glm::vec3& GetRotation() const noexcept;

    private:

        void updateModelMatrix();

        glm::mat4 model;
        glm::vec3 position, scale, rotation;

        struct vertex_data_pool {
            std::vector<glm::vec3> positions;
            std::vector<glm::vec3> normals;
            std::vector<glm::vec3> bitangents;
            std::vector<glm::vec3> tangents;
            std::vector<glm::vec2> uvs;
        } vertices;

        std::vector<uint32_t> indices;

        std::unique_ptr<Buffer> vertexPositions() noexcept;
        std::unique_ptr<Buffer> vertexNormals() noexcept;

        using vertex_positions = std::integral_constant<size_t, 0>;
        using vertex_normals = std::integral_constant<size_t, 1>;
        using vertex_uvs = std::integral_constant<size_t, 4>;
        using vertex_tangents = std::integral_constant<size_t, 3>;
        using vertex_bitangents = std::integral_constant<size_t, 2>; 

        std::array<std::unique_ptr<Buffer>, 5> vbos;
        std::unique_ptr<Buffer> ebo;
        const Device* device;
    };

}

#endif //!VULPESRENDER_TRIANGLE_MESH_HPP