#include "vpr_stdafx.h"
#include "BaseScene.hpp"
#include "common/CreateInfoBase.hpp"
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
#define TINYOBJLOADER_IMPLEMENTATION
#include "tinyobj/tiny_obj_loader.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"
#include "glm/gtx/hash.hpp"
#include <unordered_map>
INITIALIZE_EASYLOGGINGPP
/** This serves as a super-simple example of how to derive from a vulpes::BaseScene object. It doesn't render a GUI, however, so another 
 *  example is required to show how to use that.
 *  \ingroup Scenes
 */
class HouseScene : public vulpes::BaseScene {
public:  

    struct vertex_t {
        glm::vec3 pos;
        glm::vec2 uv;
        bool operator==(const vertex_t& other) const noexcept {
            return (pos == other.pos) && (uv == other.uv);
        }
    };

private:

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
    void updateUBO();

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

    VkViewport viewport;
    VkRect2D scissor;

};

// specialize hash() operator for vertex_t
namespace std {
    template<> struct hash<HouseScene::vertex_t> {
        size_t operator()(const HouseScene::vertex_t& vert) const {
            return (hash<glm::vec3>()(vert.pos) ^ (hash<glm::vec2>()(vert.uv) << 1));
        }
    };
}

HouseScene::HouseScene() : BaseScene(1, 1440, 900), viewport(vulpes::vk_default_viewport), scissor(vulpes::vk_default_viewport_scissor) {
    create();
}

HouseScene::~HouseScene() {
    destroy();
}

void HouseScene::WindowResized() {
    destroy();
}

void HouseScene::RecreateObjects() {
    LOG(INFO) << "Recreating HouseScene.";
    create();
}

void HouseScene::RecordCommands() {
    
    static VkCommandBufferBeginInfo primary_cmd_buffer_begin_info {
        VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        nullptr,
        VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT | VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
        nullptr
    };

    static VkCommandBufferInheritanceInfo secondary_cmd_buffer_inheritance_info = vulpes::vk_command_buffer_inheritance_info_base;
    secondary_cmd_buffer_inheritance_info.renderPass = renderPass->vkHandle();
    secondary_cmd_buffer_inheritance_info.subpass = 0;

    static VkCommandBufferBeginInfo secondary_cmd_buffer_begin_info {
        VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        nullptr,
        VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT | VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT | VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT,
        &secondary_cmd_buffer_inheritance_info
    };

    updateUBO();

    for(size_t i = 0; i < graphicsPool->size(); ++i) {

        secondary_cmd_buffer_inheritance_info.framebuffer = framebuffers[i];
        renderPass->UpdateBeginInfo(framebuffers[i]);

        viewport.width = static_cast<float>(swapchain->Extent.width);
        viewport.height = static_cast<float>(swapchain->Extent.height);

        scissor.extent.width = swapchain->Extent.width;
        scissor.extent.height = swapchain->Extent.height;

        VkResult err = vkBeginCommandBuffer(graphicsPool->GetCmdBuffer(i), &primary_cmd_buffer_begin_info);
        VkAssert(err);

            vkCmdBeginRenderPass(graphicsPool->GetCmdBuffer(i), &renderPass->BeginInfo(), VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);
            auto& secondary_cmd_buffer = secondaryPool->GetCmdBuffer(i);
            vkBeginCommandBuffer(secondary_cmd_buffer, &secondary_cmd_buffer_begin_info);
                vkCmdBindPipeline(secondary_cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline->vkHandle());
                vkCmdSetViewport(secondary_cmd_buffer, 0, 1, &viewport);
                vkCmdSetScissor(secondary_cmd_buffer, 0, 1, &scissor);
                vkCmdBindDescriptorSets(secondary_cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout->vkHandle(), 0, 1, &descriptorSet->vkHandle(), 0, nullptr);
                vkCmdPushConstants(secondary_cmd_buffer, pipelineLayout->vkHandle(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(vs_ubo), &uboData);
                vkCmdBindIndexBuffer(secondary_cmd_buffer, ebo->vkHandle(), 0, VK_INDEX_TYPE_UINT32);
                constexpr static VkDeviceSize offsets[1] { 0 }; // required, even if set to zero
                vkCmdBindVertexBuffers(secondary_cmd_buffer, 0, 1, &vbo->vkHandle(), offsets);
                vkCmdDrawIndexed(secondary_cmd_buffer, static_cast<uint32_t>(meshData.indices.size()), 1, 0, 0, 0);
            vkEndCommandBuffer(secondary_cmd_buffer);

            vkCmdExecuteCommands(graphicsPool->GetCmdBuffer(i), 1, &secondaryPool->GetCmdBuffer(i));
            vkCmdEndRenderPass(graphicsPool->GetCmdBuffer(i));

        err = vkEndCommandBuffer(graphicsPool->GetCmdBuffer(i));
        VkAssert(err);

    }

}

void HouseScene::endFrame(const size_t& idx) {
    vkResetFences(device->vkHandle(), 1, &presentFences[idx]);
    secondaryPool->ResetCmdBuffer(idx);
    graphicsPool->ResetCmdBuffer(idx);
}

void HouseScene::updateUBO() {

    uboData.view = glm::lookAt(glm::vec3(2.0f), glm::vec3(0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    uboData.projection = GetProjectionMatrix();

    static auto start_time = std::chrono::high_resolution_clock::now();
    auto curr_time = std::chrono::high_resolution_clock::now();
    float diff = std::chrono::duration_cast<std::chrono::milliseconds>(curr_time - start_time).count() / 10000.0f;
    uboData.model = glm::rotate(glm::mat4(1.0f), diff * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f)); // pivot house around center axis based on time.

}

void HouseScene::create() {
    SetCameraTarget(glm::vec3(0.0f));
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
    LOG(INFO) << "Destroying HouseScene objects...";
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
    std::string texture_path = SceneConfiguration.ResourcePathPrefixStr + "scenes/scene_resources/chalet.jpg";
    stbi_uc* pixels = stbi_load(texture_path.c_str(), &texture_width, &texture_height, &texture_channels, STBI_rgb_alpha);
    LOG(INFO) << "Loaded Chalet object texture.";
    VkDeviceSize image_size = texture_width * texture_height * 4;

    LOG(INFO) << "Creating Vulkan objects for Chalet texture...";
    VkBuffer image_staging_buffer;
    vulpes::Allocation image_staging_alloc;
    vulpes::Buffer::CreateStagingBuffer(device.get(), image_size, image_staging_buffer, image_staging_alloc);

    void* mapped;
    VkResult err = vkMapMemory(device->vkHandle(), image_staging_alloc.Memory(), 0, image_size, 0, &mapped);
    VkAssert(err);
        LOG(INFO) << "Copying Chalet texture data to Vulkan mapped memory now.";
        memcpy(mapped, pixels, static_cast<size_t>(image_size));
    image_staging_alloc.Unmap();
    LOG(INFO) << "Copied texture data successfully to Vulkan mapped memory, creating Texture object...";

    texture = std::make_unique<vulpes::Texture<vulpes::texture_2d_t>>(device.get());
    const VkBufferImageCopy staging_copy_info{ 0, 0, 0, VkImageSubresourceLayers{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1}, VkOffset3D{ 0, 0, 0 }, VkExtent3D{ 4096, 4096, 1 } };
    LOG(INFO) << "Creating texture backing objects from staging buffer...";
    texture->CreateFromBuffer(std::move(image_staging_buffer), VK_FORMAT_R8G8B8A8_UNORM, { staging_copy_info });
    LOG(INFO) << "Uploading texture to device...";
    auto& cmd = transferPool->Begin();
        texture->TransferToDevice(cmd);
    transferPool->Submit();

    stbi_image_free(pixels);
}
    

void HouseScene::loadMeshData()  {  
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string err;
    LOG(INFO) << "Importing .obj file.";
    std::string obj_path = SceneConfiguration.ResourcePathPrefixStr + "scenes/scene_resources/Chalet.obj";
    if(!tinyobj::LoadObj(&attrib, &shapes, &materials, &err, obj_path.c_str())){
        LOG(ERROR) << "Loading obj file failed: " << err;
        throw std::runtime_error(err.c_str());
    }

    std::unordered_map<vertex_t, uint32_t> unique_vertices{};
    LOG(INFO) << "Post-processing of imported .obj: Collapse and remove shared vertices.";
    size_t indices_initial_count = 0;
    for(const auto& shape : shapes) {
        for(const auto& idx : shape.mesh.indices) {
            vertex_t vert{};
            indices_initial_count += shape.mesh.indices.size();
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
    size_t vertices_final_count = meshData.vertices.size();
    LOG(INFO) << "Initial vertex count: " << std::to_string(indices_initial_count);
    LOG(INFO) << "Final vertex count: " << std::to_string(vertices_final_count);
}
    
void HouseScene::createMeshBuffers()  { 
    vbo = std::make_unique<vulpes::Buffer>(device.get());
    ebo = std::make_unique<vulpes::Buffer>(device.get());
    LOG(INFO) << "Creating Vulkan buffers for mesh...";
    vbo->CreateBuffer(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, sizeof(vertex_t) * meshData.vertices.size());
    ebo->CreateBuffer(VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, sizeof(uint32_t) * meshData.indices.size());
    LOG(INFO) << "Uploading mesh data to device...";
    auto& cmd = transferPool->Begin();
        vbo->CopyTo(meshData.vertices.data(), cmd, sizeof(vertex_t) * meshData.vertices.size(), 0);
        ebo->CopyTo(meshData.indices.data(), cmd, sizeof(uint32_t) * meshData.indices.size(), 0);
    transferPool->Submit();

}

void HouseScene::createDescriptorPool()  {
    descriptorPool = std::make_unique<vulpes::DescriptorPool>(device.get(), 1);
    descriptorPool->AddResourceType(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4);
    descriptorPool->Create();
}

void HouseScene::createDescriptorSet()  {
    descriptorSet = std::make_unique<vulpes::DescriptorSet>(device.get());
    descriptorSet->AddDescriptorBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0);
    const VkDescriptorImageInfo descriptor = texture->GetDescriptor();
    descriptorSet->AddDescriptorInfo(descriptor, 0);
    descriptorSet->Init(descriptorPool.get());
}

void HouseScene::createPipelineLayout()  {
    pipelineLayout = std::make_unique<vulpes::PipelineLayout>(device.get());
    pipelineLayout->Create({ descriptorSet->vkLayout() }, { VkPushConstantRange{ VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(vs_ubo) } });
}

void HouseScene::createShaders() {
    vert = std::make_unique<vulpes::ShaderModule>(device.get(), SceneConfiguration.ResourcePathPrefixStr + "scenes/scene_resources/shaders/house.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
    frag = std::make_unique<vulpes::ShaderModule>(device.get(), SceneConfiguration.ResourcePathPrefixStr + "scenes/scene_resources/shaders/house.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
}

void HouseScene::createPipelineCache() {

    pipelineCache = std::make_unique<vulpes::PipelineCache>(device.get(), static_cast<uint16_t>(typeid(HouseScene).hash_code()));
}

void HouseScene::setPipelineStateInfo() {

    static const std::array<VkVertexInputAttributeDescription, 2> attr{
        VkVertexInputAttributeDescription{ 0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0 },
        VkVertexInputAttributeDescription{ 1, 0, VK_FORMAT_R32G32_SFLOAT, sizeof(glm::vec3) }
    };

    static const VkVertexInputBindingDescription bind{ 0, sizeof(vertex_t), VK_VERTEX_INPUT_RATE_VERTEX };

    pipelineStateInfo.VertexInfo.vertexBindingDescriptionCount = 1;
    pipelineStateInfo.VertexInfo.pVertexBindingDescriptions = &bind;
    pipelineStateInfo.VertexInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attr.size());
    pipelineStateInfo.VertexInfo.pVertexAttributeDescriptions = attr.data();

    pipelineStateInfo.MultisampleInfo.rasterizationSamples = BaseScene::SceneConfiguration.MSAA_SampleCount;
    
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
    LOG(INFO) << "Creating graphics pipeline...";
    graphicsPipeline = std::make_unique<vulpes::GraphicsPipeline>(device.get());
    graphicsPipeline->Init(pipelineCreateInfo, pipelineCache->vkHandle());

}


int main() {
    #ifdef __linux__
    vulpes::BaseScene::SceneConfiguration.ResourcePathPrefixStr = std::string("../../");
    #endif
    vulpes::BaseScene::SceneConfiguration.ApplicationName = std::string("House DemoScene");
    vulpes::BaseScene::SceneConfiguration.EnableGUI = false;
    vulpes::BaseScene::SceneConfiguration.EnableMouseLocking = false;
    try {
        HouseScene scene;
        scene.RenderLoop();
    }
    catch(const std::exception& e) {
        LOG(ERROR) << "Exception encountered in HouseScene! Exception was: " << e.what();
        throw e;
    }
    return 0;
}