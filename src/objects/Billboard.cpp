#include "vpr_stdafx.h"
#include "objects/Billboard.hpp"
#include "core/Instance.hpp"
#include "core/LogicalDevice.hpp"
#include "command/TransferPool.hpp"

namespace vulpes {

    constexpr static std::array<float, 20> billboard_vertices {
        -0.5f,-0.5f, 0.0f, 0.0f, 1.0f,
         0.5f,-0.5f, 0.0f, 1.0f, 1.0f,
        -0.5f, 0.5f, 0.0f, 0.0f, 0.0f,
         0.5f, 0.5f, 0.0f, 1.0f, 0.0f
    };

    Billboard::Billboard(const glm::vec3& position_, const glm::vec3& scale_, const glm::mat4& projection_) : position(position_), scale(scale_) {
        uboData.projection = projection_;
    }

    Billboard::~Billboard() {
        vbo.reset();
        frag.reset();
        vert.reset();
        pipelineLayout.reset();
        pipelineCache.reset();
        pipeline.reset();
        texture.reset();
    }

    void Billboard::SetSurfaceTexture(const char* texture_filename) {

        texture = std::make_unique<Texture<texture_2d_t>>(device);
        texture->CreateFromFile(texture_filename);

    }

    void Billboard::Init(const Device* dvc, const VkRenderPass& renderpass, TransferPool* transfer_pool, DescriptorPool* descriptor_pool) {
        device = dvc;
        createBuffers();
    }

    void Billboard::createBuffers() {

        vbo = std::make_unique<Buffer>(device);
        vbo->CreateBuffer(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, sizeof(float) * billboard_vertices.size());
        
    }

    void Billboard::transferResources(TransferPool* transfer_pool_) {

        auto& cmd = transfer_pool_->Begin();
            vbo->CopyTo(const_cast<float*>(billboard_vertices.data()), cmd, sizeof(float) * 15, 0);
            texture->TransferToDevice(cmd);
        transfer_pool_->Submit();

    }

    void Billboard::createShaders() {

        vert = std::make_unique<ShaderModule>(device, "rsrc/shaders/billboard.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
        frag = std::make_unique<ShaderModule>(device, "rsrc/shaders/billboard.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);

    }

    void Billboard::createDescriptorSet(DescriptorPool* descriptor_pool) {

        descriptorSet = std::make_unique<DescriptorSet>(device);
        descriptorSet->AddDescriptorBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_VERTEX_BIT, 0);
        descriptorSet->AddDescriptorInfo(texture->GetDescriptor(), 0);
        descriptorSet->Init(descriptor_pool);

    }

    void Billboard::createPipelineLayout() {

        pipelineLayout = std::make_unique<PipelineLayout>(device);
        pipelineLayout->Create( { descriptorSet->vkLayout() }, { VkPushConstantRange{ VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(billboard_ubo_data_t) } });
    
    }

    void Billboard::createPipelineCache() {

        pipelineCache = std::make_unique<PipelineCache>(device, static_cast<uint16_t>(typeid(Billboard).hash_code()));

    }

    void Billboard::setPipelineStateInfo() {

        constexpr static VkDynamicState dynamic_states[2]{ VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
        pipelineStateInfo.DynamicStateInfo.dynamicStateCount = 2;
        pipelineStateInfo.DynamicStateInfo.pDynamicStates = dynamic_states;

        pipelineStateInfo.MultisampleInfo.sampleShadingEnable = Instance::VulpesInstanceConfig.EnableMSAA;
        if (pipelineStateInfo.MultisampleInfo.sampleShadingEnable) {
            pipelineStateInfo.MultisampleInfo.rasterizationSamples = Instance::VulpesInstanceConfig.MSAA_SampleCount;
        }

        pipelineStateInfo.VertexInfo.vertexBindingDescriptionCount = 1;
        pipelineStateInfo.VertexInfo.pVertexBindingDescriptions = &bindingDescription;
        pipelineStateInfo.VertexInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
        pipelineStateInfo.VertexInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

    }

    void Billboard::createGraphicsPipeline() {

    }

}