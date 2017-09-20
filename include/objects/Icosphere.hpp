#pragma once
#ifndef VULPESRENDER_ICOSPHERE_HPP
#define VULPESRENDER_ICOSPHERE_HPP

#include "vpr_stdafx.h"
#include "TriangleMesh.hpp"
#include "resource/Texture.hpp"
#include "resource/Buffer.hpp"
#include "resource/ShaderModule.hpp"
#include "resource/DescriptorSet.hpp"
#include "resource/PipelineLayout.hpp"
#include "resource/PipelineCache.hpp"
#include "render/GraphicsPipeline.hpp"

namespace vulpes {

    class Icosphere : public TriangleMesh {
        Icosphere(const Icosphere&) = delete;
        Icosphere& operator=(const Icosphere&) = delete;
    public:

        Icosphere(const size_t& detail_level, const glm::vec3& position, const glm::vec3& scale = glm::vec3(1.0f), const glm::vec3& rotation = glm::vec3(0.0f));
        ~Icosphere();

        void Init(const Device* dvc, const glm::mat4& projection, const VkRenderPass& renderpass, TransferPool* transfer_pool);
        void CreateShaders(const std::string& vertex_shader_path, const std::string& fragment_shader_path);
        void UpdateUBO(const glm::mat4& view) noexcept;
        void SetColor(const glm::vec3& new_color) noexcept;
        const glm::vec3& GetColor() const noexcept;

    private:

        void createMesh(const size_t& subdivision_level);
        void uploadData(TransferPool* transfer_pool);
        void createPipelineCache();
        void createPipelineLayout();
        void setPipelineStateInfo();
        void createGraphicsPipeline(const VkRenderPass& render_pass);

        struct ubo_data_t {
            glm::mat4 model;
            glm::mat4 view;
            glm::mat4 projection;
            glm::vec4 color;
        } uboData;

        constexpr static VkVertexInputBindingDescription bindingDescription{ 0, sizeof(vertex_t), VK_VERTEX_INPUT_RATE_VERTEX };
        constexpr static VkVertexInputAttributeDescription attributeDescription{ 0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0 };

        std::unique_ptr<Texture<gli::texture_cube>> texture;
        std::unique_ptr<ShaderModule> vert, frag;
        std::unique_ptr<DescriptorSet> descriptorSet;
        std::unique_ptr<PipelineCache> pipelineCache;
        std::unique_ptr<PipelineLayout> pipelineLayout;
        std::unique_ptr<GraphicsPipeline> graphicsPipeline;

        GraphicsPipelineInfo pipelineStateInfo;
        VkGraphicsPipelineCreateInfo pipelineCreateInfo;
        size_t subdivisionLevel;
    };


}

#endif //!VULPESRENDER_ICOSPHERE_HPP