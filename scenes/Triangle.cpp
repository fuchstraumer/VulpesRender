#include "vpr_stdafx.h"
#include "BaseScene.hpp"
#include <memory>
#include "core/Instance.hpp"
#include "core/LogicalDevice.hpp"
#include "core/PhysicalDevice.hpp"
#include "render/Swapchain.hpp"
#include "render/Renderpass.hpp"
#include "render/Framebuffer.hpp"
#include "command/CommandPool.hpp"
#include "render/DepthStencil.hpp"
INITIALIZE_EASYLOGGINGPP
namespace vulpes {

    class TriangleScene : public BaseScene {
    public:

        TriangleScene();
        ~TriangleScene();

        virtual void WindowResized() override;
        virtual void RecreateObjects() override;
        virtual void RecordCommands() override;

    private:

        void create();
        void destroy();
        void createShaders();
        void setPipelineStateInfo();
        void createPipelineLayout();
        void createGraphicsPipeline();

        virtual void endFrame(const size_t& idx) override;

        std::unique_ptr<ShaderModule> vert, frag;
        std::unique_ptr<GraphicsPipeline> pipeline;
        VkPipelineLayout pipelineLayout; // dummy layout to allow pipeline creation. Empty.
        GraphicsPipelineInfo pipelineStateInfo;
        VkGraphicsPipelineCreateInfo pipelineCreateInfo;

        VkViewport viewport;
        VkRect2D scissor;

    };

    TriangleScene::TriangleScene() : BaseScene(1, 1440, 900), viewport(vk_default_viewport), scissor(vk_default_viewport_scissor) {
        create();
    }

    TriangleScene::~TriangleScene() {
        destroy();
    }

    void TriangleScene::destroy() {
        vkDestroyPipelineLayout(device->vkHandle(), pipelineLayout, nullptr);
        pipeline.reset();
        vert.reset();
        frag.reset();
    }

    void TriangleScene::WindowResized() {
        destroy();
    }

    void TriangleScene::RecreateObjects() {
        create();
    }

    void TriangleScene::RecordCommands() {

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

        for(size_t i = 0; i < graphicsPool->size(); ++i) {

            secondary_cmd_buffer_inheritance_info.framebuffer = framebuffers[i];
            renderPass->UpdateBeginInfo(framebuffers[i]);
    
            viewport.width = static_cast<float>(swapchain->Extent.width);
            viewport.height = static_cast<float>(swapchain->Extent.height);
    
            scissor.extent.width = swapchain->Extent.width;
            scissor.extent.height = swapchain->Extent.height;
    
            VkResult err = vkBeginCommandBuffer(graphicsPool->GetCmdBuffer(i), &primary_cmd_buffer_begin_info);
            VkAssert(err);
                auto& secondary_cmd_buffer = secondaryPool->GetCmdBuffer(i);
                vkCmdBeginRenderPass(graphicsPool->GetCmdBuffer(i), &renderPass->BeginInfo(), VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);
                vkBeginCommandBuffer(secondary_cmd_buffer, &secondary_cmd_buffer_begin_info);
                    vkCmdBindPipeline(secondary_cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->vkHandle());
                    vkCmdSetViewport(secondary_cmd_buffer, 0, 1, &viewport);
                    vkCmdSetScissor(secondary_cmd_buffer, 0, 1, &scissor);
                    vkCmdDraw(secondary_cmd_buffer, 3, 1, 0, 0);
                vkEndCommandBuffer(secondary_cmd_buffer);

                vkCmdExecuteCommands(graphicsPool->GetCmdBuffer(i), 1, &secondary_cmd_buffer);
                vkCmdEndRenderPass(graphicsPool->GetCmdBuffer(i));

            err = vkEndCommandBuffer(graphicsPool->GetCmdBuffer(i));
            VkAssert(err);
        }
    }
    
    void TriangleScene::create() {
        createShaders();
        setPipelineStateInfo();
        SetupRenderpass(SceneConfiguration.MSAA_SampleCount);
        SetupFramebuffers();
        createPipelineLayout();
        createGraphicsPipeline();
    }

    void TriangleScene::createShaders() {
        vert = std::make_unique<ShaderModule>(device.get(), "../scenes/scene_resources/shaders/triangle.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
        frag = std::make_unique<ShaderModule>(device.get(), "../scenes/scene_resources/shaders/triangle.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
    }

    void TriangleScene::setPipelineStateInfo() {
        
        if(BaseScene::SceneConfiguration.EnableMSAA) {
            pipelineStateInfo.MultisampleInfo.sampleShadingEnable = VK_TRUE;
            pipelineStateInfo.MultisampleInfo.rasterizationSamples = BaseScene::SceneConfiguration.MSAA_SampleCount;
        }
        else {
            pipelineStateInfo.MultisampleInfo.sampleShadingEnable = VK_FALSE;
        }

        constexpr static VkDynamicState dynamic_states[2] { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
        pipelineStateInfo.DynamicStateInfo.dynamicStateCount = 2;
        pipelineStateInfo.DynamicStateInfo.pDynamicStates = dynamic_states;

        pipelineStateInfo.DepthStencilInfo.depthTestEnable = VK_FALSE;

    }

    void TriangleScene::createPipelineLayout() {

        VkPipelineLayoutCreateInfo layout_create_info = vk_pipeline_layout_create_info_base;
        VkResult err = vkCreatePipelineLayout(device->vkHandle(), &layout_create_info, nullptr, &pipelineLayout);
        VkAssert(err);

    }

    void TriangleScene::createGraphicsPipeline() {

        pipelineCreateInfo = pipelineStateInfo.GetPipelineCreateInfo();
        VkPipelineShaderStageCreateInfo shader_stages[2]{ vert->PipelineInfo(), frag->PipelineInfo() };
        pipelineCreateInfo.stageCount = 2;
        pipelineCreateInfo.pStages = shader_stages;
        pipelineCreateInfo.layout = pipelineLayout;
        pipelineCreateInfo.renderPass = renderPass->vkHandle();
        pipelineCreateInfo.subpass = 0;
        pipelineCreateInfo.basePipelineIndex = -1;
        pipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
    
        pipeline = std::make_unique<GraphicsPipeline>(device.get());
        pipeline->Init(pipelineCreateInfo, VK_NULL_HANDLE);

    }

    void TriangleScene::endFrame(const size_t& idx) {
        vkResetFences(device->vkHandle(), 1, &presentFences[idx]);
    }
}

int main() {
    vulpes::BaseScene::SceneConfiguration.ApplicationName = std::string("Triangle DemoScene");
    vulpes::BaseScene::SceneConfiguration.EnableGUI = false;
    vulpes::BaseScene::SceneConfiguration.EnableMouseLocking = false;
    vulpes::TriangleScene scene;
    scene.RenderLoop();
    return 0;
}