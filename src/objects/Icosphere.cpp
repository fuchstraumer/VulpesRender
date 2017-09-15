#include "vpr_stdafx.h"
#include <unordered_map>
#include "objects/Icosphere.hpp"
#include "core/Instance.hpp"
#include "core/LogicalDevice.hpp"
#include "command/TransferPool.hpp"

namespace vulpes {

    constexpr float golden_ratio = 1.61803398875f;
    
    static const std::array<vertex_t, 12> initial_vertices {
        vertex_t{ glm::vec3(-1.0f, golden_ratio, 0.0f) },
        vertex_t{ glm::vec3( 1.0f, golden_ratio, 0.0f) },
        vertex_t{ glm::vec3(-1.0f,-golden_ratio, 0.0f) },
        vertex_t{ glm::vec3( 1.0f,-golden_ratio, 0.0f) },
    
        vertex_t{ glm::vec3( 0.0f,-1.0f, golden_ratio) },
        vertex_t{ glm::vec3( 0.0f, 1.0f, golden_ratio) },
        vertex_t{ glm::vec3( 0.0f,-1.0f,-golden_ratio) },
        vertex_t{ glm::vec3( 0.0f, 1.0f,-golden_ratio) },
    
        vertex_t{ glm::vec3( golden_ratio, 0.0f,-1.0f) },
        vertex_t{ glm::vec3( golden_ratio, 0.0f, 1.0f) },
        vertex_t{ glm::vec3(-golden_ratio, 0.0f,-1.0f) },
        vertex_t{ glm::vec3(-golden_ratio, 0.0f, 1.0f) },
    };

    constexpr static std::array<uint32_t, 60> initial_indices {
        0,11, 5,
        0, 5, 1,
        0, 1, 7,
        0, 7,10,
        0,10,11,
    
        1, 5, 9,
        5,11, 4,
       11,10, 2,
       10, 7, 6,
        7, 1, 8,
    
        3, 9, 4,
        3, 4, 2,
        3, 2, 6,
        3, 6, 8,
        3, 8, 9,
    
        4, 9, 5,
        2, 4,11,
        6, 2,10,
        8, 6, 7,
        9, 8, 1
    };

    static inline glm::vec3 midpoint(const vertex_t& v0, const vertex_t v1) noexcept {
        return (v0.pos + v1.pos) / 2.0f;
    }

    void Icosphere::CreateShaders(const std::string& vertex_shader_path, const std::string& fragment_shader_path) {

        vert = std::make_unique<ShaderModule>(device, vertex_shader_path, VK_SHADER_STAGE_VERTEX_BIT);
        frag = std::make_unique<ShaderModule>(device, fragment_shader_path, VK_SHADER_STAGE_FRAGMENT_BIT);
    
    }

    void Icosphere::createMesh(const size_t& subdivision_level) {

        std::unordered_map<vertex_t, uint32_t> unique_vertices;

        uint32_t i = 0;
        for(const auto& vert : initial_vertices) {
            unique_vertices[vert] = i;
            ++i;
        }

        std::vector<uint32_t> index_buffer;
        indices.insert(indices.cend(), initial_indices.cbegin(), initial_indices.cend());

        for(size_t i = 0; i < subdivision_level; ++i) {
            index_buffer.reserve(NumIndices() * 4);
            for(size_t j = 0; j < NumIndices(); j += 3) {

                const auto& vert0 = vertices[j];
                const auto& vert1 = vertices[j + 1];
                const auto& vert2 = vertices[j + 2];

                const glm::vec3 midpoint_0_1 = midpoint(vert0, vert1);
                const glm::vec3 midpoint_1_2 = midpoint(vert1, vert2);
                const glm::vec3 midpoint_0_2 = midpoint(vert0, vert2);

                

                if(unique_vertices.count(midpoint_0_1) == 0) {
                    unique_vertices[midpoint_0_1] = AddVertex(midpoint_0_1);
                }

                const uint32_t& midpoint_0_1_idx = unique_vertices[midpoint_0_1];
                
                if(unique_vertices.count(midpoint_1_2) == 0) {
                    unique_vertices[midpoint_1_2] = AddVertex(midpoint_1_2);
                }

                const uint32_t& midpoint_1_2_idx = unique_vertices[midpoint_1_2];

                if(unique_vertices.count(midpoint_0_2) == 0) {
                    unique_vertices[midpoint_0_2] = AddVertex(midpoint_0_2);
                }

                const uint32_t& midpoint_0_2_idx = unique_vertices[midpoint_0_2];
                
                AddTriangle(indices[j], midpoint_0_1_idx, midpoint_0_2_idx);
                AddTriangle(midpoint_0_1_idx, indices[j + 1], midpoint_1_2_idx);
                AddTriangle(midpoint_0_2_idx, midpoint_1_2_idx, indices[j + 2]);
                AddTriangle(midpoint_0_2_idx, midpoint_1_2_idx, midpoint_0_2_idx);

            }
            
        }

        for(auto& vert : vertices) {
            vert.normal = glm::normalize(vert.pos - glm::vec3(0.0f));
        }

        CreateBuffers(device);
    }

    void Icosphere::uploadData(TransferPool* transfer_pool) {

        auto& cmd = transfer_pool->Begin();
        RecordTransferCommands(cmd);
        transfer_pool->End();
        transfer_pool->Submit();

    }

    void Icosphere::createPipelineCache() {
        pipelineCache = std::make_unique<PipelineCache>(device, static_cast<uint16_t>(typeid(Icosphere).hash_code()));
    }



}