#pragma once
#ifndef VULPESRENDER_TRIANGLE_MESH_HPP
#define VULPESRENDER_TRIANGLE_MESH_HPP

#include "vpr_stdafx.h"
#include "resource/Buffer.hpp"
#include "vertex_t.hpp"

namespace vulpes {    

    class TriangleMesh {
        TriangleMesh(const TriangleMesh&) = delete;
        TriangleMesh& operator=(const TriangleMesh&) = delete;
    public:

        TriangleMesh() = default;
        TriangleMesh(const glm::vec3& _position, const glm::vec3& scale = glm::vec3(1.0f), const glm::vec3& rotation = glm::vec3(0.0f));
        virtual ~TriangleMesh();

        // pass by value used here as vertex_t and uint32_t are cheap to move
        // and are always copied into this object's private containers.
        uint32_t AddVertex(vertex_t vert) noexcept;
        void AddIndex(uint32_t idx) noexcept;
        void AddTriangle(uint32_t i0, uint32_t i1, uint32_t i2) noexcept;

        const vertex_t& GetVertex(const uint32_t& index) const;

        void ReserveVertices(const size_t& reserve_amount) noexcept;
        void ReserveIndices(const size_t& reserve_amount) noexcept;

        size_t NumVertices() const noexcept;
        size_t NumIndices() const noexcept;

        void CreateBuffers(const Device* dvc);

        void RecordTransferCommands(const VkCommandBuffer& transfer_cmd);
        void Render(const VkCommandBuffer& draw_cmd) const noexcept;

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

    protected:

        void updateModelMatrix();

        glm::mat4 model;
        glm::vec3 position, scale, rotation;

        std::vector<vertex_t> vertices;
        std::vector<uint32_t> indices;

        std::unique_ptr<Buffer> vertexPositions() noexcept;
        std::unique_ptr<Buffer> vertexNormals() noexcept;

        std::unique_ptr<Buffer> vbo;
        std::unique_ptr<Buffer> ebo;
        const Device* device;
    };

}

#endif //!VULPESRENDER_TRIANGLE_MESH_HPP