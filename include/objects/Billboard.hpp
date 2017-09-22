#pragma once
#ifndef VULPESRENDER_BILLBOARD_OBJECT_HPP
#define VULPESRENDER_BILLBOARD_OBJECT_HPP

#include "vpr_stdafx.h"
#include "resource/Texture.hpp"
#include "resource/Buffer.hpp"
#include "resource/ShaderModule.hpp"
#include "resource/DescriptorSet.hpp"
#include "resource/PipelineLayout.hpp"
#include "resource/PipelineCache.hpp"
#include "render/GraphicsPipeline.hpp"

namespace vulpes {

    class Billboard {
        Billboard(const Billboard&) = delete;
        Billboard& operator=(const Billboard&) = delete;
    public:

        Billboard(const glm::vec3& pos, const glm::vec3& scale, const glm::mat4& projection);
        ~Billboard();

        void SetSurfaceTexture(const char* texture_filename);
        void Init(const Device* dvc, const VkRenderPass& renderpass, TransferPool* transfer_pool, DescriptorPool* descriptor_pool);
        void Render(const VkCommandBuffer& cmd_, const VkCommandBufferBeginInfo& begin_info_, const VkViewport& viewport, const VkRect2D& scissor);
        void UpdateUBO(const glm::mat4& view);

    protected:

        void createBuffers();
        void transferResources(TransferPool* transfer_pool_);
        void createShaders();
        void createDescriptorSet(DescriptorPool* descriptor_pool);
        void createPipelineLayout();
        void createPipelineCache();
        void setPipelineStateInfo();
        void createGraphicsPipeline(const VkRenderPass& renderpass);

        constexpr static VkVertexInputBindingDescription bindingDescription {
            0, sizeof(float) * 5, VK_VERTEX_INPUT_RATE_VERTEX
        };

        constexpr static  std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions {
            VkVertexInputAttributeDescription{ 0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0 },
            VkVertexInputAttributeDescription{ 1, 0, VK_FORMAT_R32G32_SFLOAT, sizeof(float) * 3 }
        };

        glm::vec3 position, scale;
        const Device* device = nullptr;

        std::unique_ptr<Buffer> vbo;
        std::unique_ptr<DescriptorSet> descriptorSet;
        std::unique_ptr<PipelineLayout> pipelineLayout;
        std::unique_ptr<PipelineCache> pipelineCache;
        std::unique_ptr<ShaderModule> vert, frag;
        std::unique_ptr<GraphicsPipeline> pipeline;
        std::unique_ptr<Texture<texture_2d_t>> texture;

        GraphicsPipelineInfo pipelineStateInfo;
        VkGraphicsPipelineCreateInfo pipelineCreateInfo;

        struct billboard_ubo_data_t {
            glm::mat4 view;
            glm::mat4 projection;
            glm::vec4 center;
            glm::vec4 cameraUp;
            glm::vec4 cameraRight;
        } uboData;
    };

}


#endif //!VULPESRENDER_BILLBOARD_OBJECT_HPP