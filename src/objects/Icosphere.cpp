#include "vpr_stdafx.h"
#include <unordered_map>
#include "objects/Icosphere.hpp"
#include "core/LogicalDevice.hpp"
#include "command/TransferPool.hpp"
#include "core/PhysicalDevice.hpp"
#include "BaseScene.hpp"

namespace vulpes {

    constexpr float golden_ratio = 1.61803398875f;
    constexpr float FLOAT_PI = 3.14159265359f;
    
    static const std::array<vertex_t, 12> initial_vertices {
        vertex_t{ glm::vec3(-golden_ratio, 1.0f, 0.0f) },
        vertex_t{ glm::vec3( golden_ratio, 1.0f, 0.0f) },
        vertex_t{ glm::vec3(-golden_ratio,-1.0f, 0.0f) },
        vertex_t{ glm::vec3( golden_ratio,-1.0f, 0.0f) },
        
        vertex_t{ glm::vec3( 0.0f,-golden_ratio, 1.0f) },
        vertex_t{ glm::vec3( 0.0f, golden_ratio, 1.0f) },
        vertex_t{ glm::vec3( 0.0f,-golden_ratio,-1.0f) },
        vertex_t{ glm::vec3( 0.0f, golden_ratio,-1.0f) },

        vertex_t{ glm::vec3( 1.0f, 0.0f,-golden_ratio) },
        vertex_t{ glm::vec3( 1.0f, 0.0f, golden_ratio) },
        vertex_t{ glm::vec3(-1.0f, 0.0f,-golden_ratio) },
        vertex_t{ glm::vec3(-1.0f, 0.0f, golden_ratio) }
    };

    constexpr static std::array<uint32_t, 60> initial_indices {
        0,11, 5, 
        0, 5, 1, 
        0, 1, 7, 
        0, 7,10, 
        0,10,11,

        5,11, 4, 
        1, 5, 9, 
        7, 1, 8,
       10, 7, 6,
       11,10, 2,

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

    Icosphere::Icosphere(const size_t& subdivision_level, const glm::vec3& pos, const glm::vec3& scale, const glm::vec3& rotation) : TriangleMesh(pos, scale, rotation), subdivisionLevel(subdivision_level) {
        updateModelMatrix();
    }

    Icosphere::~Icosphere() {
        vbo.reset();
        frag.reset();
        vert.reset();
        pipelineLayout.reset();
        pipelineCache.reset();
        descriptorSet.reset();
        graphicsPipeline.reset();
    }

    void Icosphere::SetTexture(const char* filename) {
        texturePath = std::string(filename);
    }

    void Icosphere::SetTexture(const char* filename, const VkFormat& compressed_texture_format) {
        texturePath = std::string(filename);
        textureFormat = compressed_texture_format;
    }

    void Icosphere::Init(const Device* dvc, const glm::mat4& projection, const VkRenderPass& renderpass, TransferPool* transfer_pool, DescriptorPool* descriptor_pool) {
        
        device = dvc;
        uboData.projection = projection;
        
        createMesh(subdivisionLevel);
        createTexture();
        uploadData(transfer_pool);
        createPipelineCache();
        createDescriptorSet(descriptor_pool);
        createPipelineLayout();
        createShadersImpl();
        setPipelineStateInfo();
        createGraphicsPipeline(renderpass);

    }

    void Icosphere::Render(const VkCommandBuffer& cmd_buffer, const VkCommandBufferBeginInfo& begin_info, const VkViewport& viewport, const VkRect2D& scissor) {
        vkBeginCommandBuffer(cmd_buffer, &begin_info);
            vkCmdBindPipeline(cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline->vkHandle());
            vkCmdSetViewport(cmd_buffer, 0, 1, &viewport);
            vkCmdSetScissor(cmd_buffer, 0, 1, &scissor);
            vkCmdBindDescriptorSets(cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout->vkHandle(), 0, 1, &descriptorSet->vkHandle(), 0, nullptr);
            vkCmdPushConstants(cmd_buffer, pipelineLayout->vkHandle(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4) * 3, &uboData);
            // pass pointer to first member, taken as offset into ubodata.
            vkCmdPushConstants(cmd_buffer, pipelineLayout->vkHandle(), VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(glm::mat4) * 3, sizeof(glm::vec4) * 3, &uboData.lightPosition); 
            TriangleMesh::Render(cmd_buffer);
        vkEndCommandBuffer(cmd_buffer);
    }

    void Icosphere::CreateShaders(const std::string & vertex_shader_path, const std::string & fragment_shader_path) {
        vertPath = vertex_shader_path;
        fragPath = fragment_shader_path;
    }

    static inline glm::vec3 midpoint(const vertex_t& v0, const vertex_t v1) noexcept {
        return (v0.pos + v1.pos) / 2.0f;
    }

    void Icosphere::createShadersImpl() {

        vert = std::make_unique<ShaderModule>(device, vertPath, VK_SHADER_STAGE_VERTEX_BIT);
        frag = std::make_unique<ShaderModule>(device, fragPath, VK_SHADER_STAGE_FRAGMENT_BIT);
    
    }

    void Icosphere::UpdateUBO(const glm::mat4 & view, const glm::vec3& viewer_position) noexcept {
        uboData.view = view;
        uboData.viewerPosition = glm::vec4(viewer_position.x, viewer_position.y, viewer_position.z, 1.0f);
    }

    void Icosphere::UpdateLightPosition(const glm::vec3& new_light_pos) noexcept {
        uboData.lightPosition = glm::vec4(new_light_pos.x, new_light_pos.y, new_light_pos.z, 1.0f);
    }

    void Icosphere::SetModelMatrix(const glm::mat4 & model) {
        uboData.model = model;
    }

    void Icosphere::createMesh(const size_t& subdivision_level) {
        subdivide(subdivision_level);
        calculateUVs();
        uboData.model = GetModelMatrix();
        CreateBuffers(device);
    }

    void Icosphere::subdivide(const size_t& subdivision_level) {

        indices.assign(initial_indices.cbegin(), initial_indices.cend());
        vertices.assign(initial_vertices.cbegin(), initial_vertices.cend());

        for (int j = 0; j < subdivision_level; ++j) {
            const size_t num_triangles = NumIndices() / 3;
            for (uint32_t i = 0; i < num_triangles; ++i) {
                uint32_t i0 = indices[i * 3 + 0];
                uint32_t i1 = indices[i * 3 + 1];
                uint32_t i2 = indices[i * 3 + 2];

                uint32_t i3 = static_cast<uint32_t>(vertices.size());
                uint32_t i4 = i3 + 1;
                uint32_t i5 = i4 + 1;

                indices[i * 3 + 1] = i3;
                indices[i * 3 + 2] = i5;

                indices.insert(indices.cend(), { i3, i1, i4, i5, i3, i4, i5, i4, i2 });

                const glm::vec3 midpoint0 = 0.5f * (vertices[i0].pos + vertices[i1].pos);
                const glm::vec3 midpoint1 = 0.5f * (vertices[i1].pos + vertices[i2].pos);
                const glm::vec3 midpoint2 = 0.5f * (vertices[i2].pos + vertices[i0].pos);

                AddVertex(vertex_t{ midpoint0, midpoint0 });
                AddVertex(vertex_t{ midpoint1, midpoint1 });
                AddVertex(vertex_t{ midpoint2, midpoint2 });
            }
        }
       
        for (auto& vert : vertices) {
            vert.pos = glm::normalize(vert.pos);
            vert.normal = glm::normalize(vert.pos);
        }
    }

    void Icosphere::calculateUVs() {
        
        for(size_t i = 0; i < vertices.size(); ++i) {
            const glm::vec3& norm = vertices[i].normal;
            vertices[i].uv.x = 0.5f - 0.5f * glm::atan(norm.x, -norm.z) / FLOAT_PI;
            vertices[i].uv.y = glm::acos(norm.y) / FLOAT_PI;
        }

        auto add_vertex_w_uv = [&](const size_t& i, const glm::vec2& uv) {
            const uint32_t& idx = indices[i];
            indices[i] = static_cast<uint32_t>(vertices.size());
            vertices.push_back(vertex_t{ vertices[idx].pos, vertices[idx].normal, uv });
        };

        /*const size_t num_triangles = indices.size() / 3;
        for(size_t i = 0; i < num_triangles; ++i) {
            const glm::vec2& uv0 = vertices[indices[i * 3]].uv;
            const glm::vec2& uv1 = vertices[indices[i * 3 + 1]].uv;
            const glm::vec2& uv2 = vertices[indices[i * 3 + 2]].uv;

            const float d1 = uv1.x - uv0.x;
            const float d2 = uv2.x - uv0.x;

            if(std::abs(d1) > 0.5f && std::abs(d2) > 0.5f){
                add_vertex_w_uv(i * 3, uv0 + glm::vec2((d1 > 0.0f) ? 1.0f : -1.0f, 0.0f));
            }
            else if(std::abs(d1) > 0.5f) {
                add_vertex_w_uv(i * 3 + 1, uv1 + glm::vec2((d1 < 0.0f) ? 1.0f : -1.0f, 0.0f));
            }
            else if(std::abs(d2) > 0.5f) {
                add_vertex_w_uv(i * 3 + 2, uv2 + glm::vec2((d2 < 0.0f) ? 1.0f : -1.0f, 0.0f));
            }
        }*/
    }

    void Icosphere::uploadData(TransferPool* transfer_pool) {

        auto& cmd = transfer_pool->Begin();
        RecordTransferCommands(cmd);
        if(texture) {
            texture->TransferToDevice(cmd);
        }
        transfer_pool->Submit();

    }

    void Icosphere::createPipelineCache() {
        pipelineCache = std::make_unique<PipelineCache>(device, static_cast<uint16_t>(typeid(Icosphere).hash_code()));
    }

    void Icosphere::createDescriptorSet(DescriptorPool* descriptor_pool) {
        descriptorSet = std::make_unique<DescriptorSet>(device);
        descriptorSet->AddDescriptorBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0);
        descriptorSet->AddDescriptorInfo(texture->GetDescriptor(), 0);
        descriptorSet->Init(descriptor_pool);
    }

    void Icosphere::createPipelineLayout() {
        pipelineLayout = std::make_unique<PipelineLayout>(device);
        static const std::vector<VkPushConstantRange> push_constants{
            VkPushConstantRange{ VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4) * 3 },
            VkPushConstantRange{ VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(glm::mat4) * 3, sizeof(glm::vec4) * 3 }
        };
        pipelineLayout->Create({ descriptorSet->vkLayout() }, push_constants);
    }

    void Icosphere::setPipelineStateInfo() {

        constexpr static VkDynamicState dynamic_states[2]{ VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
        pipelineStateInfo.DynamicStateInfo.dynamicStateCount = 2;
        pipelineStateInfo.DynamicStateInfo.pDynamicStates = dynamic_states;

        pipelineStateInfo.MultisampleInfo.sampleShadingEnable = BaseScene::SceneConfiguration.EnableMSAA;
        if (pipelineStateInfo.MultisampleInfo.sampleShadingEnable) {
            pipelineStateInfo.MultisampleInfo.rasterizationSamples = BaseScene::SceneConfiguration.MSAA_SampleCount;
        }

        pipelineStateInfo.VertexInfo.vertexBindingDescriptionCount = 1;
        pipelineStateInfo.VertexInfo.pVertexBindingDescriptions = &vertex_t::bindingDescription;
        pipelineStateInfo.VertexInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertex_t::attributeDescriptions.size());
        pipelineStateInfo.VertexInfo.pVertexAttributeDescriptions = vertex_t::attributeDescriptions.data();

    }

    void Icosphere::createGraphicsPipeline(const VkRenderPass& renderpass) {

        if(!vert || !frag) {
            LOG(ERROR) << "Didn't specify shaders for an icosphere before attempting initilization!";
            throw std::runtime_error("No shaders for icosphere object found.");
        }

        pipelineCreateInfo = pipelineStateInfo.GetPipelineCreateInfo();
        const std::array<VkPipelineShaderStageCreateInfo, 2> shader_stages {
            vert->PipelineInfo(), frag->PipelineInfo()
        };

        pipelineCreateInfo.stageCount = static_cast<uint32_t>(shader_stages.size());
        pipelineCreateInfo.pStages = shader_stages.data();

        pipelineCreateInfo.layout = pipelineLayout->vkHandle();
        pipelineCreateInfo.renderPass = renderpass;
        pipelineCreateInfo.subpass = 0;
        pipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
        pipelineCreateInfo.basePipelineIndex = -1;

        graphicsPipeline = std::make_unique<GraphicsPipeline>(device);
        graphicsPipeline->Init(pipelineCreateInfo, pipelineCache->vkHandle());

    }

    void Icosphere::createTexture() {

        if(textureFormat == VK_FORMAT_R8G8B8A8_UNORM) {
            texture = std::make_unique<Texture<texture_2d_t>>(device);
            texture->CreateFromFile(texturePath.c_str());
        } 

    }

}