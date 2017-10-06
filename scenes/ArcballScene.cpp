#include "vpr_stdafx.h"
#include "BaseScene.hpp"
#include "common/CreateInfoBase.hpp"
#include "core/Instance.hpp"
#include "resource/DescriptorPool.hpp"
#include "resource/Allocator.hpp"
#include "util/utilitySphere.hpp" 
#include "util/Ray.hpp"
#include "objects/Icosphere.hpp"
#include "objects/Skybox.hpp"
#include "util/Camera.hpp"
#include "util/Arcball.hpp"
#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"
#include "glm/gtx/hash.hpp"
#include "glm/gtc/quaternion.hpp"
INITIALIZE_EASYLOGGINGPP

namespace vulpes {

    class ArcballScene : public BaseScene {
    public:
        
        ArcballScene();
        ~ArcballScene();

        virtual void WindowResized() override;
        virtual void RecreateObjects() override;
        virtual void RecordCommands() override;

    private:

        virtual void endFrame(const size_t& idx) override;
        void create();
        void destroy();

        void createSkybox();
        void createIcosphere();
        void createDescriptorPool();
        void updateUBOs();

        std::unique_ptr<DescriptorPool> descriptorPool;
        std::unique_ptr<Skybox> skybox;
        std::unique_ptr<Icosphere> earthSphere;
        VkViewport viewport = vk_default_viewport;
        VkRect2D scissor = vk_default_viewport_scissor;

    };

    ArcballScene::ArcballScene() : BaseScene(3, 1440, 900) {
        create();
    }

    ArcballScene::~ArcballScene() {
        destroy();
    }

    void ArcballScene::WindowResized() {
        destroy();
    }

    void ArcballScene::RecreateObjects() {
        create();
    }

    void ArcballScene::RecordCommands() {

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
    
        updateUBOs();

        ImGui::BeginMainMenuBar();
        ImGui::EndMainMenuBar();
    
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
                    skybox->Render(secondaryPool->GetCmdBuffer(i * 3), secondary_cmd_buffer_begin_info, viewport, scissor);
                    earthSphere->Render(secondaryPool->GetCmdBuffer((i * 3) + 1), secondary_cmd_buffer_begin_info, viewport, scissor);
                    renderGUI(secondaryPool->GetCmdBuffer((i * 3) + 2), secondary_cmd_buffer_begin_info, i);
                VkCommandBuffer secondary_buffers[3]{ secondaryPool->GetCmdBuffer(i *3), secondaryPool->GetCmdBuffer((i * 3) + 1), secondaryPool->GetCmdBuffer((i * 3) + 2) };
                vkCmdExecuteCommands(graphicsPool->GetCmdBuffer(i), 3, secondary_buffers);
                vkCmdEndRenderPass(graphicsPool->GetCmdBuffer(i));
            
            err = vkEndCommandBuffer(graphicsPool->GetCmdBuffer(i));
            VkAssert(err);
            
        }
    }

    void ArcballScene::endFrame(const size_t & idx) {
        vkResetFences(device->vkHandle(), 1, &presentFences[idx]);
    }

    void ArcballScene::create() {
        SetCameraPosition(glm::vec3(0.0f, 1.0f, 3.0f));
        SetCameraTarget(glm::vec3(0.0f));
        arcballCamera.SetArcballSphere(UtilitySphere(glm::vec3(0.0f), 1.0f));
        createDescriptorPool();
        createSkybox();
        createIcosphere();
    }

    void ArcballScene::destroy() {
        earthSphere.reset();
        skybox.reset();
        descriptorPool.reset();
    }

    void ArcballScene::createSkybox() {
        LOG(INFO) << "Creating skybox...";
        skybox = std::make_unique<Skybox>(device.get());
        skybox->CreateTexture(BaseScene::SceneConfiguration.ResourcePathPrefixStr + "scenes/scene_resources/starbox.dds", VK_FORMAT_B8G8R8A8_UNORM);
        skybox->Create(GetProjectionMatrix(), renderPass->vkHandle(), transferPool.get(), descriptorPool.get());
    }

    void ArcballScene::createIcosphere() {
        earthSphere = std::make_unique<Icosphere>(4, glm::vec3(0.0f), glm::vec3(1.0f));
        earthSphere->CreateShaders(std::string(BaseScene::SceneConfiguration.ResourcePathPrefixStr + "scenes/scene_resources/shaders/earthsphere.vert.spv"), std::string(BaseScene::SceneConfiguration.ResourcePathPrefixStr + "scenes/scene_resources/shaders/earthsphere.frag.spv"));
        earthSphere->SetTexture(std::string(BaseScene::SceneConfiguration.ResourcePathPrefixStr + "scenes/scene_resources/earth.png").c_str());
        earthSphere->Init(device.get(), GetProjectionMatrix(), renderPass->vkHandle(), transferPool.get(), descriptorPool.get());
    }

    void ArcballScene::createDescriptorPool() {
        descriptorPool = std::make_unique<DescriptorPool>(device.get(), 2);
        descriptorPool->AddResourceType(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2);
        descriptorPool->Create();
    }

    void ArcballScene::updateUBOs() {
        skybox->UpdateUBO(GetViewMatrix());
        earthSphere->UpdateUBO(GetViewMatrix(), GetCameraPosition());

        static auto start_time = std::chrono::high_resolution_clock::now();
        auto curr_time = std::chrono::high_resolution_clock::now();
        float diff = std::chrono::duration_cast<std::chrono::milliseconds>(curr_time - start_time).count() / 10000.0f;
        glm::mat4 rot_model = glm::rotate(glm::mat4(1.0f), diff * glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f)); // pivot house around center axis based on time.
        earthSphere->SetModelMatrix(rot_model);

    }

}

int main() {
    vulpes::BaseScene::SceneConfiguration.EnableGUI = true;
    vulpes::BaseScene::SceneConfiguration.EnableMouseLocking = false;
    vulpes::BaseScene::SceneConfiguration.CameraType = vulpes::cameraType::ARCBALL;
#ifdef _WIN32
    vulpes::BaseScene::SceneConfiguration.ResourcePathPrefixStr = std::string("../");
#else
    vulpes::BaseScene::SceneConfiguration.ResourcePathPrefixStr = std::string("../");
#endif
    vulpes::ArcballScene scene;
    scene.RenderLoop();
    return 0;
}