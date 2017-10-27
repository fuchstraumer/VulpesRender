#include "vpr_stdafx.h"
#include "objects/Skybox.hpp"
#include "core/Instance.hpp"
#include "core/LogicalDevice.hpp"
#include "command/TransferPool.hpp"
#include "BaseScene.hpp"
namespace vulpes {

    static const std::array<glm::vec3, 8> positions {
        glm::vec3(-1.0f, -1.0f, +1.0f), 
        glm::vec3(+1.0f, -1.0f, +1.0f), 
        glm::vec3(+1.0f, +1.0f, +1.0f), 
        glm::vec3(-1.0f, +1.0f, +1.0f),
        glm::vec3(+1.0f, -1.0f, -1.0f), 
        glm::vec3(-1.0f, -1.0f, -1.0f), 
        glm::vec3(-1.0f, +1.0f, -1.0f), 
        glm::vec3(+1.0f, +1.0f, -1.0f),
    };

    Skybox::Skybox(const Device* dvc) { device = dvc; }

    Skybox::~Skybox() {
        texture.reset();
        pipelineLayout.reset();
        frag.reset();
        vert.reset();
        descriptorSet.reset();
        pipelineCache.reset();
        graphicsPipeline.reset();
    }

    void Skybox::CreateTexture(const std::string& texture_filename, const VkFormat& texture_format) {
        createTexture(texture_filename, texture_format);
    }

    void Skybox::Create(const glm::mat4& projection, const VkRenderPass& render_pass, TransferPool* transfer_pool, DescriptorPool* descriptor_pool) {
        
        uboData.projection = projection;
        uboData.view = glm::mat4(1.0f);
        createMesh();
        uploadData(transfer_pool);
        createShaders();
        setupDescriptorSet(descriptor_pool);
        createPipelineCache();
        setupPipelineLayout();
        setPipelineStateInfo();
        createGraphicsPipeline(render_pass);

    }

    void Skybox::Render(const VkCommandBuffer& draw_cmd, const VkCommandBufferBeginInfo& begin_info, const VkViewport& viewport, const VkRect2D& scissor) {
        
        vkBeginCommandBuffer(draw_cmd, &begin_info);
            vkCmdBindPipeline(draw_cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline->vkHandle());
            vkCmdSetViewport(draw_cmd, 0, 1, &viewport);
            vkCmdSetScissor(draw_cmd, 0, 1, &scissor);
            vkCmdBindDescriptorSets(draw_cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout->vkHandle(), 0, 1, &descriptorSet->vkHandle(), 0, nullptr);
            vkCmdPushConstants(draw_cmd, pipelineLayout->vkHandle(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(ubo_data_t), &uboData);
            TriangleMesh::Render(draw_cmd);
        vkEndCommandBuffer(draw_cmd);

    }

    void Skybox::UpdateUBO(const glm::mat4& view_matrix) noexcept {
        uboData.view = glm::mat4(glm::mat3(view_matrix));
    }

    void Skybox::createMesh() {

        auto build_face = [this](const glm::vec3& p0, const glm::vec3& p1, const glm::vec3& p2, const glm::vec3& p3){

            uint32_t i0 = AddVertex(vertex_t{ p0 });
            uint32_t i1 = AddVertex(vertex_t{ p1 });
            uint32_t i2 = AddVertex(vertex_t{ p2 });
            uint32_t i3 = AddVertex(vertex_t{ p3 });

            AddTriangle(i0, i1, i2);
            AddTriangle(i0, i2, i3);
        };

        build_face(positions[0], positions[1], positions[2], positions[3]);
        build_face(positions[1], positions[4], positions[7], positions[2]);
        build_face(positions[3], positions[2], positions[7], positions[6]);
        build_face(positions[5], positions[0], positions[3], positions[6]);
        build_face(positions[5], positions[4], positions[1], positions[0]);
        build_face(positions[4], positions[5], positions[6], positions[7]);
        
        CreateBuffers(device);

    }

    void Skybox::createTexture(const std::string& filename, const VkFormat& file_format) {

        texture = std::make_unique<Texture<gli::texture_cube>>(device);
        texture->CreateFromFile(filename.c_str(), file_format);

    }

    void Skybox::uploadData(TransferPool* transfer_pool) {

        auto& cmd = transfer_pool->Begin();
        texture->TransferToDevice(cmd);
        RecordTransferCommands(cmd);
        
        transfer_pool->Submit();

    }

    void Skybox::createShaders() {


        vert = std::make_unique<ShaderModule>(device, BaseScene::SceneConfiguration.ResourcePathPrefixStr + "rsrc/shaders/skybox/skybox.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
        frag = std::make_unique<ShaderModule>(device, BaseScene::SceneConfiguration.ResourcePathPrefixStr + "rsrc/shaders/skybox/skybox.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);

    }

    void Skybox::createPipelineCache() {

        pipelineCache = std::make_unique<PipelineCache>(device, static_cast<uint16_t>(typeid(Skybox).hash_code()));

    }

    void Skybox::setupDescriptorSet(DescriptorPool* descriptor_pool) {
        
        descriptorSet = std::make_unique<DescriptorSet>(device);
        descriptorSet->AddDescriptorBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0);
        descriptorSet->AddDescriptorInfo(texture->GetDescriptor(), 0);
        descriptorSet->Init(descriptor_pool);

    }

    void Skybox::setupPipelineLayout() {

        pipelineLayout = std::make_unique<PipelineLayout>(device);
        pipelineLayout->Create({ descriptorSet->vkLayout() }, { VkPushConstantRange{ VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(ubo_data_t) }});

    }

    void Skybox::setPipelineStateInfo() {

        constexpr static VkDynamicState dynamic_states[2]{ VK_DYNAMIC_STATE_SCISSOR, VK_DYNAMIC_STATE_VIEWPORT };

        graphicsPipelineStateInfo.DynamicStateInfo.dynamicStateCount = 2;
        graphicsPipelineStateInfo.DynamicStateInfo.pDynamicStates = dynamic_states;

        graphicsPipelineStateInfo.MultisampleInfo.sampleShadingEnable = BaseScene::SceneConfiguration.EnableMSAA;
        if(BaseScene::SceneConfiguration.EnableMSAA) {
            graphicsPipelineStateInfo.MultisampleInfo.rasterizationSamples = BaseScene::SceneConfiguration.MSAA_SampleCount;
        }

        graphicsPipelineStateInfo.VertexInfo.vertexBindingDescriptionCount = 1;
        graphicsPipelineStateInfo.VertexInfo.vertexAttributeDescriptionCount = 1;
        graphicsPipelineStateInfo.VertexInfo.pVertexBindingDescriptions = &bind_descr;
        graphicsPipelineStateInfo.VertexInfo.pVertexAttributeDescriptions = &attr_descr;

        graphicsPipelineStateInfo.DepthStencilInfo.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;

    }

    void Skybox::createGraphicsPipeline(const VkRenderPass& render_pass) {

        graphicsPipelineCreateInfo = graphicsPipelineStateInfo.GetPipelineCreateInfo();
        const std::array<VkPipelineShaderStageCreateInfo, 2> shader_stages {
            vert->PipelineInfo(), frag->PipelineInfo()
        };

        graphicsPipelineCreateInfo.stageCount = static_cast<uint32_t>(shader_stages.size());
        graphicsPipelineCreateInfo.pStages = shader_stages.data();

        graphicsPipelineCreateInfo.layout = pipelineLayout->vkHandle();
        graphicsPipelineCreateInfo.renderPass = render_pass;
        graphicsPipelineCreateInfo.subpass = 0;
        graphicsPipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
        graphicsPipelineCreateInfo.basePipelineIndex = -1;

        graphicsPipeline = std::make_unique<GraphicsPipeline>(device);
        graphicsPipeline->Init(graphicsPipelineCreateInfo, pipelineCache->vkHandle());

    }

}