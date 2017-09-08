#include "vpr_stdafx.h"
#include "BaseScene.hpp"
#include "core/Instance.hpp"
#include "resource/Buffer.hpp"
#include "resource/DescriptorPool.hpp"
#include "resource/DescriptorSet.hpp"
#include "resource/PipelineLayout.hpp"
#include "resource/ShaderModule.hpp"
#include "render/GraphicsPipeline.hpp"
#include "resource/Texture.hpp"
#include "resource/Allocator.hpp"
#include "resource/PipelineCache.hpp"
#include "tinyobj/tiny_obj_loader.h"
#include "stb/stb_image.h"

class HouseScene : public vulpes::BaseScene {
    
    struct vertex_t {
        glm::vec3 pos;
        glm::vec2 uv;
    };

    struct mesh {
        std::vector<uint32_t> indices;
        std::vector<vertex_t> vertices;
     } meshData;

     struct vs_ubo {
         glm::mat4 model, view, projection;
     } uboData;

public:

    HouseScene();
    ~HouseScene();

    virtual void WindowResized() override;
    virtual void RecreateObjects() override;
    virtual void RecordCommands() override;

private:

    virtual void endFrame(const size_t& idx) override;

    void create();
    void loadMeshTexture();
    void loadMeshData();
    void createMeshBuffers();
    void createDescriptorPool();
    void createDescriptorSet();
    void createPipelineLayout();
    void createShaders();
    void createPipelineCache();
    void setPipelineStateInfo();
    void createGraphicsPipeline();
    void destroy();
    

    std::unique_ptr<vulpes::Texture<vulpes::texture_2d_t>> texture;
    std::unique_ptr<vulpes::DescriptorPool> descriptorPool;
    std::unique_ptr<vulpes::Buffer> vbo, ebo;
    std::unique_ptr<vulpes::DescriptorSet> descriptorSet;
    std::unique_ptr<vulpes::PipelineLayout> pipelineLayout;
    std::unique_ptr<vulpes::ShaderModule> vert, frag;
    std::unique_ptr<vulpes::PipelineCache> pipelineCache;
    vulpes::GraphicsPipelineInfo pipelineStateInfo;
    VkGraphicsPipelineCreateInfo pipelineCreateInfo;
    std::unique_ptr<vulpes::GraphicsPipeline> graphicsPipeline;

};

int main() {

}

HouseScene::HouseScene() : BaseScene(1, 1440, 900) {
    create();
}

HouseScene::~HouseScene() {
    destroy();
}

void HouseScene::WindowResized() {
    destroy();
}

void HouseScene::RecreateObjects() {
    create();
}

void HouseScene::RecordCommands() {
    
}

void HouseScene::create() {
    loadMeshTexture();
    loadMeshData();
    createMeshBuffers();
    createDescriptorPool();
    createDescriptorSet();
    createPipelineLayout();
    createShaders();
    createPipelineCache();
    setPipelineStateInfo();
    createGraphicsPipeline();
}

void HouseScene::destroy() {
    graphicsPipeline.reset();
    texture.reset();
    vbo.reset();
    ebo.reset();
    pipelineLayout.reset();
    descriptorSet.reset();
    descriptorPool.reset();
    vert.reset();
    frag.reset();
    pipelineCache.reset();
}

void HouseScene::loadMeshTexture()  { 
    int texture_width, texture_height, texture_channels;
    stbi_uc* pixels = stbi_load("scene_resources/chalet.jpg", &texture_width, &texture_height, &texture_channels, STBI_rgb_alpha);
    VkDeviceSize image_size = texture_width * texture_height * 4;

    VkBuffer image_staging_buffer;
    vulpes::Allocation image_staging_alloc;
    vulpes::Buffer::CreateStagingBuffer(device.get(), image_size, image_staging_buffer, image_staging_alloc);

    void* mapped;
    image_staging_alloc.Map(image_size, 0, mapped);
        memcpy(mapped, pixels, image_size);
    image_staging_alloc.Unmap();

    texture = std::make_unique<vulpes::Texture<vulpes::texture_2d_t>>(device.get());
    const VkBufferImageCopy staging_copy_info{ 0, 0, 0, VkImageSubresourceLayers{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1}, VkOffset3D{ 0, 0, 0 }, VkExtent3D{ 4096, 4096, 1 } };
    texture->CreateFromBuffer(std::move(image_staging_buffer), VK_FORMAT_R8G8B8A8_UNORM, { staging_copy_info });

    auto& cmd = transferPool->Begin();
        texture->TransferToDevice(cmd);
    transferPool->End();
    transferPool->Submit();
}
    

void HouseScene::loadMeshData()  {  
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string err;

    if(!tinyobj::LoadObj(&attrib, &shapes, &materials, &err, "scene_resources/Chalet.obj")){
        LOG(ERROR) << "Loading obj file failed: " << err;
        throw std::runtime_error(err.c_str());
    }

    std::unordered_map<vertex_t, uint32_t> unique_vertices{};

    for(const auto& shape : shapes) {
        for(const auto& idx : shape.mesh.indices) {
            vertex_t vert{};

            vert.pos = {
                attrib.vertices[3 * idx.vertex_index],
                attrib.vertices[3 * idx.vertex_index + 1],
                attrib.vertices[3 * idx.vertex_index + 2]
            };

            vert.uv = {
                attrib.texcoords[2 * idx.texcoord_index],
                1.0f - attrib.texcoords[2 * idx.texcoord_index + 1]
            };

            if(unique_vertices.count(vert) == 0) {
                unique_vertices[vert] = static_cast<uint32_t>(meshData.vertices.size());
                meshData.vertices.push_back(std::move(vert));
            }

            meshData.indices.push_back(unique_vertices[vert]);
        }
    }
}
    
void HouseScene::createMeshBuffers()  { 
    vbo = std::make_unique<vulpes::Buffer>(device.get());
    ebo = std::make_unique<vulpes::Buffer>(device.get());

    vbo->CreateBuffer(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, sizeof(vertex_t) * meshData.vertices.size());
    ebo->CreateBuffer(VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, sizeof(uint32_t) * meshData.indices.size());

    auto& cmd = transferPool->Begin();
        vbo->CopyTo(meshData.vertices.data(), cmd, sizeof(vertex_t) * meshData.vertices.size(), 0);
        ebo->CopyTo(meshData.indices.data(), cmd, sizeof(uint32_t) * meshData.indices.size(), 0);
    transferPool->End();
    transferPool->Submit();
}

void HouseScene::createDescriptorPool()  {
    descriptorPool = std::make_unique<vulpes::DescriptorPool>(device.get(), 1);
    descriptorPool->Create();
    descriptorPool->AddResourceType(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1);
}

void HouseScene::createDescriptorSet()  {
    descriptorSet = std::make_unique<vulpes::DescriptorSet>(device.get());
    descriptorSet->AddDescriptorBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0);
    descriptorSet->AddDescriptorInfo(texture->GetDescriptor(), 0);
    descriptorSet->Init(descriptorPool.get());
}

void HouseScene::createPipelineLayout()  {
    pipelineLayout = std::make_unique<vulpes::PipelineLayout>(device.get());
    pipelineLayout->Create({ descriptorSet->vkLayout() }, { VkPushConstantRange{ VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(vs_ubo) } });
}

void HouseScene::createShaders() {
    vert = std::make_unique<vulpes::ShaderModule>(device.get(), "scene_resources/shaders/house.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
    frag = std::make_unique<vulpes::ShaderModule>(device.get(), "scene_resources/shaders/house.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
}

void HouseScene::setPipelineStateInfo() {

    static const std::array<VkVertexInputAttributeDescription, 2> attr{
        VkVertexInputAttributeDescription{ 0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0 },
        VkVertexInputAttributeDescription{ 0, 1, VK_FORMAT_R32G32_SFLOAT, sizeof(glm::vec3) }
    };

    static const VkVertexInputBindingDescription bind{ 0, sizeof(vertex_t), VK_VERTEX_INPUT_RATE_VERTEX };

    pipelineStateInfo.VertexInfo.vertexBindingDescriptionCount = 1;
    pipelineStateInfo.VertexInfo.pVertexBindingDescriptions = &bind;
    pipelineStateInfo.VertexInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attr.size());
    pipelineStateInfo.VertexInfo.pVertexAttributeDescriptions = attr.data();

    pipelineStateInfo.MultisampleInfo.rasterizationSamples = vulpes::Instance::VulpesInstanceConfig.MSAA_SampleCount;
    
    constexpr static VkDynamicState dynamic_states[2] { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
    pipelineStateInfo.DynamicStateInfo.dynamicStateCount = 2;
    pipelineStateInfo.DynamicStateInfo.pDynamicStates = dynamic_states;

}

void HouseScene::createGraphicsPipeline() {

    pipelineCreateInfo = pipelineStateInfo.GetPipelineCreateInfo();
    VkPipelineShaderStageCreateInfo shader_stages[2]{ vert->PipelineInfo(), frag->PipelineInfo() };
    pipelineCreateInfo.stageCount = 2;
    pipelineCreateInfo.pStages = shader_stages;
    pipelineCreateInfo.layout = pipelineLayout->vkHandle();
    pipelineCreateInfo.renderPass = renderPass->vkHandle();
    pipelineCreateInfo.subpass = 0;
    pipelineCreateInfo.basePipelineIndex = -1;
    pipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;

    graphicsPipeline = std::make_unique<vulpes::GraphicsPipeline>(device.get());
    graphicsPipeline->Init(pipelineCreateInfo, pipelineCache->vkHandle());

}

    