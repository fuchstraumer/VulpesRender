#pragma once
#ifndef VULPESRENDER_TRIANGLE_MESH_HPP
#define VULPESRENDER_TRIANGLE_MESH_HPP

#include "vpr_stdafx.h"
#include "resource/Buffer.hpp"
#include "vertex_t.hpp"

namespace vulpes { 
    
    /** The objects group defines useful commonly used or "baseline" renderable objects. This includes a suitable base 
    *   class for objects meshed with triangles, to more complex objects like a pbr-compatible .obj model renderer and 
    *   a simple skybox class.
    *   \defgroup Objects
    */

    /** This class defines the bare minimum of methods and Vulkan objects required to render a triangle-based mesh. In case you 
    *   derive from this and change the vbo/ebo layout and/or the exact method of drawing, be sure to override the relevant methods.
    *   Additionally, don't call CreateBuffers until after you have added vertices and indices to the relevant internal containers.
    *   \ingroup Objects
    */
    class TriangleMesh {
        TriangleMesh(const TriangleMesh&) = delete;
        TriangleMesh& operator=(const TriangleMesh&) = delete;
    public:

        TriangleMesh() = default;
        TriangleMesh(const glm::vec3& _position, const glm::vec3& scale = glm::vec3(1.0f), const glm::vec3& rotation = glm::vec3(0.0f));
        virtual ~TriangleMesh();

        /** Adds a vertex, and returns its index. Useful for adding a few vertices, keeping the indices, then adding triangles.
        *   Note: Uses pass by value, but with std::move. The input object will always be copied and is cheap to move, so this
        *   approach results in the compiler using a move whenever possible and a copy whenever not without requiring two separate methods.
        */
        uint32_t AddVertex(vertex_t vert) noexcept;
        void AddIndex(uint32_t idx) noexcept;
        /** A slight specialization of AddIndex, but inserts the indices all at once via a call to insert (thus being slightly more efficient), along
        *   with using std::move as AddVertex does to move and/or copy optimally.  
        */
        void AddTriangle(uint32_t i0, uint32_t i1, uint32_t i2) noexcept;

        const vertex_t& GetVertex(const uint32_t& index) const;

        void ReserveVertices(const size_t& reserve_amount) noexcept;
        void ReserveIndices(const size_t& reserve_amount) noexcept;

        size_t NumVertices() const noexcept;
        size_t NumIndices() const noexcept;

        /** Uses the data from vertices + indices to create the internal vbo and ebo objects. If called with empty containers, this will throw.
        *   Does not transfer any resources.
        */
        void CreateBuffers(const Device* dvc);

        /** Records commands required to upload the VBO and EBO into the given command buffer: works well when called amid transfers for derived classes,
        *   to pool transfers together and save on the amount of required submissions.
        */
        virtual void RecordTransferCommands(const VkCommandBuffer& transfer_cmd);

        /** Calls bindBuffers and drawIndexed by default: requires a pipeline is bound before calling this method, along with any other calls ot bind resources
        *   or set dynamic parameters
        */
        virtual void Render(const VkCommandBuffer& draw_cmd) const noexcept;

        /** Destroys VBO + EBO.
        */
        void DestroyVulkanObjects();
        /** Calls clear() + shrink_to_fit() on vertex and index containers to free their memory. */
        void FreeCpuData();

        const glm::mat4& GetModelMatrix() noexcept;
        virtual void SetModelMatrix(const glm::mat4& updated_model);

        void UpdatePosition(const glm::vec3& new_position);
        void UpdateScale(const glm::vec3& new_scale);
        void UpdateRotation(const glm::vec3& new_rotation);

        const glm::vec3& GetPosition() const noexcept;
        const glm::vec3& GetScale() const noexcept;
        const glm::vec3& GetRotation() const noexcept;

    protected:

        /** Binds EBO+VBO to the passed command buffer. Override this if your derived class features additional buffers to bind.
        */
        virtual void bindBuffers(const VkCommandBuffer& draw_cmd_buffer) const noexcept;
        /** Calls vkCmdDrawIndexed for bound buffers using the quantity of vertices and indices found in the containers. This number is saved, however,
        *   so it will still function after a call to FreeCpuData. Override for different draw commands or for different VBO setup. 
        */
        virtual void drawIndexed(const VkCommandBuffer& draw_cmd_buffer) const noexcept;
        void updateModelMatrix();

        glm::mat4 model;
        glm::vec3 position, scale, rotation;
        bool modelMatrixCached = false;
        std::vector<vertex_t> vertices;
        std::vector<uint32_t> indices;

        uint32_t numVertices, numIndices;

        std::unique_ptr<Buffer> vbo;
        std::unique_ptr<Buffer> ebo;
        const Device* device;
    };

}

#endif //!VULPESRENDER_TRIANGLE_MESH_HPP