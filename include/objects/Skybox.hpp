#ifndef VULPESRENDER_SKYBOX_OBJECT_HPP
#define VULPESRENDER_SKYBOX_OBJECT_HPP

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

    class Skybox : public TriangleMesh {
        Skybox(const Skybox&) = delete;
        Skybox& operator=(const Skybox&) = delete;
    public:

        Skybox(const Device* dvc);
        ~Skybox();

        void CreateTexture(const std::string& texture_filename, const VkFormat& texture_format);
        void Create(const glm::mat4& projection, const VkRenderPass& render_pass, TransferPool* transfer_pool, DescriptorPool* descriptor_pool);
        void Render(const VkCommandBuffer& draw_cmd, const VkCommandBufferBeginInfo& begin_info);
        void UpdateUBO(const glm::mat4& view_matrix) noexcept;

    private:

        void createMesh();
        void createResources(TransferPool* transfer_pool);
        void uploadData(TransferPool* transfer_pool);
        void createBuffers();
        void createTexture(const std::string& filename, const VkFormat& file_format);
        void createShaders();
        void createPipelineCache();
        void setupDescriptorSet(DescriptorPool* descriptor_pool);
        void setupPipelineLayout();
        void setPipelineStateInfo();
        void createGraphicsPipeline(const VkRenderPass& render_pass);

        struct ubo_data_t {
            glm::mat4 view;
            glm::mat4 projection;
        } uboData;

        const VkVertexInputBindingDescription bind_descr{ 0, sizeof(vertex_t), VK_VERTEX_INPUT_RATE_VERTEX };
        const VkVertexInputAttributeDescription attr_descr{ 0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0 };

        std::unique_ptr<Texture<gli::texture_cube>> texture;
        std::unique_ptr<ShaderModule> vert, frag;
        std::unique_ptr<DescriptorSet> descriptorSet;
        std::unique_ptr<PipelineCache> pipelineCache;
        std::unique_ptr<PipelineLayout> pipelineLayout;
        std::unique_ptr<GraphicsPipeline> graphicsPipeline;

        GraphicsPipelineInfo graphicsPipelineStateInfo;
        VkGraphicsPipelineCreateInfo graphicsPipelineCreateInfo;
        const Device* device;

    };

}

#endif //!VULPESRENDER_SKYBOX_OBJECT_HPP