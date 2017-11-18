#include "vpr_stdafx.h"
#include "BaseScene.hpp"
#include "core/Instance.hpp"
#include "core/LogicalDevice.hpp"
#include "core/PhysicalDevice.hpp"
#include "command/CommandPool.hpp"
#include "render/Framebuffer.hpp"
#include "render/Renderpass.hpp"
#include "render/Swapchain.hpp"
#include "render/DepthStencil.hpp"
#include "util/AABB.hpp"
#include <memory>
#ifdef USE_EXPERIMENTAL_FILESYSTEM
#include <experimental/filesystem>
#endif


namespace vulpes {

    bool BaseScene::CameraLock = false;
    PerspectiveCamera BaseScene::fpsCamera = PerspectiveCamera(1440, 900, 70.0f);
    ArcballCamera BaseScene::arcballCamera = ArcballCamera(1440, 900, 70.9f, UtilitySphere(glm::vec3(0.0f), 7.0f));

    std::vector<uint16_t> BaseScene::pipelineCacheHandles = std::vector<uint16_t>();

    vulpes::BaseScene::BaseScene(const size_t& num_secondary_buffers, const uint32_t& _width, const uint32_t& _height) : width(_width), height(_height), numSecondaryBuffers(num_secondary_buffers) {

        const bool verbose_logging = BaseScene::SceneConfiguration.VerboseLogging;

        VkInstanceCreateInfo create_info = vk_base_instance_info;
        instance = std::make_unique<Instance>(create_info, false, _width, _height);
        instance->GetWindow()->SetWindowUserPointer(this);

        LOG_IF(verbose_logging, INFO) << "VkInstance created.";

        Multisampling::SampleCount = BaseScene::SceneConfiguration.MSAA_SampleCount;
        LOG_IF(verbose_logging, INFO) << "MSAA level set to " << std::to_string(Multisampling::SampleCount);

        device = std::make_unique<Device>(instance.get(), instance->GetPhysicalDevice());

        swapchain = std::make_unique<Swapchain>();
        swapchain->Init(instance.get(), instance->GetPhysicalDevice(), device.get());


        LOG_IF(verbose_logging, INFO) << "Swapchain created.";

        CreateCommandPools();
        SetupDepthStencil();

        VkSemaphoreCreateInfo semaphore_info{ VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO, nullptr, 0 };
        VkResult result = vkCreateSemaphore(device->vkHandle(), &semaphore_info, nullptr, &semaphores[0]);
        VkAssert(result);
        result = vkCreateSemaphore(device->vkHandle(), &semaphore_info, nullptr, &semaphores[1]);
        VkAssert(result);

        renderCompleteSemaphores.resize(1);
        result = vkCreateSemaphore(device->vkHandle(), &semaphore_info, nullptr, &renderCompleteSemaphores[0]);
        VkAssert(result);

        presentFences.resize(swapchain->ImageCount);
    
        VkFenceCreateInfo fence_info = vk_fence_create_info_base;
        for (auto& fence : presentFences) {
            result = vkCreateFence(device->vkHandle(), &fence_info, nullptr, &fence);
            VkAssert(result);
        }

        result = vkCreateFence(device->vkHandle(), &fence_info, nullptr, &acquireFence);
        VkAssert(result);

        // init frame limiters.
        limiter_a = std::chrono::system_clock::now();
        limiter_b = std::chrono::system_clock::now();

        SetupRenderpass(SceneConfiguration.MSAA_SampleCount);
        SetupFramebuffers();

        input_handler::LastX = swapchain->Extent.width / 2.0f;
        input_handler::LastY = swapchain->Extent.height / 2.0f;

        if (SceneConfiguration.CameraType == cameraType::FPS) {
            fpsCamera.SetNearClipPlaneDistance(0.1f);
            fpsCamera.SetFarClipPlaneDistance(2000.0f);
        }
        else if (SceneConfiguration.CameraType == cameraType::ARCBALL) {
            arcballCamera.SetNearClipPlaneDistance(0.1f);
            arcballCamera.SetFarClipPlaneDistance(2000.0f);
        }

        setupGUI();

        LOG_IF(verbose_logging, INFO) << "BaseScene setup complete.";


    }

    void BaseScene::setupGUI() {

        if (!SceneConfiguration.EnableGUI) {
            return;
        }

        gui = std::make_unique<imguiWrapper>();
        gui->Init(device.get(), renderPass->vkHandle());
        gui->UploadTextureData(transferPool.get());

    }
    
    void BaseScene::destroyGUI() {

        if (!SceneConfiguration.EnableGUI) {
            return;
        }

        gui.reset();

    }

#ifdef USE_EXPERIMENTAL_FILESYSTEM
    void BaseScene::cleanupShaderCacheFiles() {
        std::experimental::filesystem::path pipeline_cache_path("rsrc/shader_cache");
        
        if (std::experimental::filesystem::exists(pipeline_cache_path) && !pipelineCacheHandles.empty()) {
            auto dir_iter = std::experimental::filesystem::directory_iterator(pipeline_cache_path);
            for (auto& p : dir_iter) {
                
                bool id_used = false;
                
                if (p.path().extension().string() == ".vkdat") {
                    
                    std::string cache_name = p.path().filename().string();
                    size_t idx = cache_name.find_last_of('.');
                    cache_name = cache_name.substr(0, idx);
                    
                    uint16_t id = static_cast<uint16_t>(std::stoi(cache_name));
                    
                    for(uint16_t& curr : pipelineCacheHandles) {
                        if (curr == id) {
                            id_used = true;
                            continue;
                        }
                    }
                    
                }
                
                if (!id_used) {
                    bool erased = std::experimental::filesystem::remove(p.path());
                    if (!erased) {
                        LOG(WARNING) << "Failed to erase a pipeline cache with ID " << p.path().filename().string();
                    }
                }
                
            }
        }
    }
#endif
    
    vulpes::BaseScene::~BaseScene() {

        destroyGUI();
        depthStencil.reset();
        graphicsPool.reset();
        transferPool.reset();
        secondaryPool.reset();
        swapchain.reset();
        msaa.reset();
        
        for (const auto& fbuf : framebuffers) {
            vkDestroyFramebuffer(device->vkHandle(), fbuf, nullptr);
        }

        for (const auto& fence : presentFences) {
            vkDestroyFence(device->vkHandle(), fence, nullptr);
        }

        vkDestroyFence(device->vkHandle(), acquireFence, nullptr);

        renderPass.reset();

        vkDestroySemaphore(device->vkHandle(), semaphores[1], nullptr);
        vkDestroySemaphore(device->vkHandle(), semaphores[0], nullptr);

#ifdef USE_EXPERIMENTAL_FILESYSTEM
        cleanupShaderCacheFiles();
#endif

    }
    
    

    void BaseScene::UpdateMouseActions() {

        ImGuiIO& io = ImGui::GetIO();

        if (!io.WantCaptureMouse) {

            if (ImGui::IsMouseDragging(0)) {
                mouseDrag(0, io.MousePos.x, io.MousePos.y);
            }
            else if (ImGui::IsMouseDragging(1)) {
                mouseDrag(1, io.MousePos.x, io.MousePos.y);
            }

            if (ImGui::IsMouseDown(0) && !ImGui::IsMouseDragging(0)) {
                mouseDown(0, io.MouseClickedPos[0].x, io.MouseClickedPos[0].y);
            }

            if (ImGui::IsMouseDown(1) && !ImGui::IsMouseDragging(1)) {
                mouseDown(1, io.MouseClickedPos[1].x, io.MouseClickedPos[1].y);
            }

            if (ImGui::IsMouseReleased(0)) {
                mouseUp(0, io.MousePos.x, io.MousePos.y);
            }

            mouseScroll(0, io.MouseWheel);
        }

        input_handler::MouseDx = 0.0f;
        input_handler::MouseDy = 0.0f;
    }

    void BaseScene::mouseDrag(const int& button, const float & rot_x, const float & rot_y) {
        if (SceneConfiguration.CameraType == cameraType::FPS) {
            fpsCamera.MouseDrag(button, rot_x, rot_y);
        }
        else if (SceneConfiguration.CameraType == cameraType::ARCBALL) {
            arcballCamera.MouseDrag(button, rot_x, rot_y);
        }

        ImGui::ResetMouseDragDelta();
    }

    void BaseScene::mouseScroll(const int& button, const float & zoom_delta) {
        if (SceneConfiguration.CameraType == cameraType::FPS) {
            fpsCamera.MouseScroll(button, zoom_delta);
        }
        else if (SceneConfiguration.CameraType == cameraType::ARCBALL) {
            arcballCamera.MouseScroll(button, zoom_delta);
        }
    }

    void BaseScene::mouseDown(const int& button, const float& x, const float& y) {
        if (SceneConfiguration.CameraType == cameraType::FPS) {
            fpsCamera.MouseDown(button, x, y);
        }
        else if (SceneConfiguration.CameraType == cameraType::ARCBALL) {
            arcballCamera.MouseDown(button, x, y);
        }
    }

    void BaseScene::mouseUp(const int& button, const float & x, const float & y) {
        if (SceneConfiguration.CameraType == cameraType::FPS) {
            fpsCamera.MouseUp(button, x, y);
        }
        else if (SceneConfiguration.CameraType == cameraType::ARCBALL) {
            arcballCamera.MouseUp(button, x, y);
        }
    }

    void BaseScene::PipelineCacheCreated(const uint16_t & cache_id) {
        pipelineCacheHandles.push_back(cache_id);
    }
    
    void BaseScene::createGraphicsCmdPool() {

        VkCommandPoolCreateInfo pool_info = vk_command_pool_info_base;
        pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        pool_info.queueFamilyIndex = device->QueueFamilyIndices.Graphics;
        graphicsPool = std::make_unique<CommandPool>(device.get(), pool_info, true);
        graphicsPool->AllocateCmdBuffers(swapchain->ImageCount, VK_COMMAND_BUFFER_LEVEL_PRIMARY);
        
    }

    void BaseScene::createTransferCmdPool() {
        VkCommandPoolCreateInfo pool_info = vk_command_pool_info_base;
        pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        pool_info.flags = device->QueueFamilyIndices.Transfer;
        transferPool = std::make_unique<TransferPool>(device.get());
        transferPool->AllocateCmdBuffers(1, VK_COMMAND_BUFFER_LEVEL_PRIMARY);
    }

    void BaseScene::createSecondaryCmdPool() {

        LOG(INFO) << "Creating command pool for secondary command buffers. " << std::to_string(swapchain->ImageCount * static_cast<uint32_t>(numSecondaryBuffers)) << " buffers requested.";
        VkCommandPoolCreateInfo pool_info = vk_command_pool_info_base;
        pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        pool_info.queueFamilyIndex = device->QueueFamilyIndices.Graphics;
        secondaryPool = std::make_unique<CommandPool>(device.get(), pool_info, false);
        secondaryPool->AllocateCmdBuffers(swapchain->ImageCount * static_cast<uint32_t>(numSecondaryBuffers), VK_COMMAND_BUFFER_LEVEL_SECONDARY);
        
    }

    void vulpes::BaseScene::CreateCommandPools() {

        LOG(INFO) << "Creating primary graphics and transfer command pools...";

        createGraphicsCmdPool();
        createTransferCmdPool();
        createSecondaryCmdPool();

    }


    void BaseScene::createRenderTargetAttachment() {

        attachmentDescriptions[0] = vk_attachment_description_base;
        attachmentDescriptions[0].format = swapchain->ColorFormat;
        attachmentDescriptions[0].samples = BaseScene::SceneConfiguration.MSAA_SampleCount;
        attachmentDescriptions[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachmentDescriptions[0].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachmentDescriptions[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachmentDescriptions[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachmentDescriptions[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        attachmentDescriptions[0].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    }

    void BaseScene::createResolveAttachment() {

        attachmentDescriptions[1] = vk_attachment_description_base;
        attachmentDescriptions[1].format = swapchain->ColorFormat;
        attachmentDescriptions[1].samples = VK_SAMPLE_COUNT_1_BIT;
        attachmentDescriptions[1].loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachmentDescriptions[1].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachmentDescriptions[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachmentDescriptions[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachmentDescriptions[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        attachmentDescriptions[1].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    }

    void BaseScene::createMultisampleDepthAttachment() {

        attachmentDescriptions[2] = vk_attachment_description_base;
        attachmentDescriptions[2].format = device->FindDepthFormat();
        attachmentDescriptions[2].samples = BaseScene::SceneConfiguration.MSAA_SampleCount;
        attachmentDescriptions[2].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachmentDescriptions[2].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachmentDescriptions[2].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachmentDescriptions[2].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachmentDescriptions[2].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        attachmentDescriptions[2].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;


    }

    void BaseScene::createResolveDepthAttachment() {

        attachmentDescriptions[3] = vk_attachment_description_base;
        attachmentDescriptions[3].format = device->FindDepthFormat();
        attachmentDescriptions[3].samples = VK_SAMPLE_COUNT_1_BIT;
        attachmentDescriptions[3].loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachmentDescriptions[3].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachmentDescriptions[3].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachmentDescriptions[3].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachmentDescriptions[3].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        attachmentDescriptions[3].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    }

    void BaseScene::createAttachmentReferences() {

        attachmentReferences.resize(3);

        attachmentReferences[0] = VkAttachmentReference{ 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
        attachmentReferences[1] = VkAttachmentReference{ 2, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL };
        attachmentReferences[2] = VkAttachmentReference{ 1, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };

    }

    VkSubpassDescription BaseScene::createSubpassDescription() {

        VkSubpassDescription result = vk_subpass_description_base;
        result.colorAttachmentCount = 1;
        result.pColorAttachments = &attachmentReferences[0];
        result.pResolveAttachments = &attachmentReferences[2];
        result.pDepthStencilAttachment = &attachmentReferences[1];
        return result;

    }

    void BaseScene::createSubpassDependencies() {

        subpassDependencies.resize(2);
        
        subpassDependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
        subpassDependencies[0].dstSubpass = 0;
        subpassDependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
        subpassDependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        subpassDependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
        subpassDependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        subpassDependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

        subpassDependencies[1].srcSubpass = 0;
        subpassDependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
        subpassDependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        subpassDependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
        subpassDependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        subpassDependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
        subpassDependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    }

    void vulpes::BaseScene::SetupRenderpass(const VkSampleCountFlagBits& sample_count) {

        LOG(INFO) << "Creating renderpass and MSAA attachments now...";

        msaa = std::make_unique<Multisampling>(device.get(), swapchain.get(), sample_count, swapchain->Extent.width, swapchain->Extent.height);

        attachmentDescriptions.resize(4);
        createRenderTargetAttachment();
        createResolveAttachment();
        createMultisampleDepthAttachment();
        createResolveDepthAttachment();

        createAttachmentReferences();
        VkSubpassDescription subpass_descr = createSubpassDescription();
        createSubpassDependencies();

        VkRenderPassCreateInfo renderpass_create_info = vk_render_pass_create_info_base;
        renderpass_create_info.attachmentCount = static_cast<uint32_t>(attachmentDescriptions.size());
        renderpass_create_info.pAttachments = attachmentDescriptions.data();
        renderpass_create_info.subpassCount = 1;
        renderpass_create_info.pSubpasses = &subpass_descr;
        renderpass_create_info.dependencyCount = static_cast<uint32_t>(subpassDependencies.size());
        renderpass_create_info.pDependencies = subpassDependencies.data();

        renderPass = std::make_unique<Renderpass>(device.get(), renderpass_create_info);

        LOG_IF(BaseScene::SceneConfiguration.VerboseLogging, INFO) << "Renderpass created.";
        
        static const std::vector<VkClearValue> clear_values {
            { 0.025f, 0.025f, 0.065f, 1.0f },
            { 0.025f, 0.025f, 0.065f, 1.0f },
            { 1.0f, 0 }
        };

        renderPass->SetupBeginInfo(clear_values, swapchain->Extent);

    }

    void vulpes::BaseScene::SetupDepthStencil() {

        LOG(INFO) << "Creating depth stencil...";
        VkQueue depth_queue = device->GraphicsQueue(0);
        depthStencil = std::make_unique<DepthStencil>(device.get(), VkExtent3D{ swapchain->Extent.width, swapchain->Extent.height, 1 }, graphicsPool.get(), depth_queue);

    }

    void vulpes::BaseScene::SetupFramebuffers() {

        LOG(INFO) << "Creating framebuffers...";
        std::array<VkImageView, 4> attachments{ msaa->ColorBufferMS->View(), VK_NULL_HANDLE, msaa->DepthBufferMS->View(), depthStencil->View() };
        VkFramebufferCreateInfo framebuffer_create_info = vk_framebuffer_create_info_base;
        framebuffer_create_info.attachmentCount = static_cast<uint32_t>(attachments.size());
        framebuffer_create_info.pAttachments = attachments.data();
        framebuffer_create_info.renderPass = renderPass->vkHandle();
        framebuffer_create_info.width = swapchain->Extent.width;
        framebuffer_create_info.height = swapchain->Extent.height;
        framebuffer_create_info.layers = 1;

        for (uint32_t i = 0; i < swapchain->ImageCount; ++i) {
            attachments[1] = swapchain->ImageViews[i];
            VkFramebuffer new_fbuff;
            VkResult result = vkCreateFramebuffer(device->vkHandle(), &framebuffer_create_info, nullptr, &new_fbuff);
            VkAssert(result);
            framebuffers.push_back(std::move(new_fbuff));
        }

    }

    void vulpes::BaseScene::RecreateSwapchain() {

        LOG_IF(BaseScene::SceneConfiguration.VerboseLogging, INFO) << "Began to recreate swapchain...";

        // First wait to make sure nothing is in use.
        vkDeviceWaitIdle(device->vkHandle());

        destroyGUI();
        depthStencil.reset();
        framebuffers.clear();
        framebuffers.shrink_to_fit();

        transferPool.reset();
        secondaryPool.reset();
        graphicsPool.reset();

        WindowResized();
        msaa.reset();
        renderPass.reset();

        device->vkAllocator->Recreate();

        swapchain->Recreate();
        

        /*
            Done destroying resources, recreate resources and objects now
        */

        ImGuiIO& io = ImGui::GetIO();
        io.DisplaySize.x = static_cast<float>(swapchain->Extent.width);
        io.DisplaySize.y = static_cast<float>(swapchain->Extent.height);
        BaseScene::fpsCamera = PerspectiveCamera(swapchain->Extent.width, swapchain->Extent.height, 70.0f);
        BaseScene::arcballCamera = ArcballCamera(swapchain->Extent.width, swapchain->Extent.height, 70.0f, UtilitySphere(glm::vec3(0.0f), 7.0f));
        input_handler::LastX = swapchain->Extent.width / 2.0f;
        input_handler::LastY = swapchain->Extent.height / 2.0f;
        if (SceneConfiguration.CameraType == cameraType::FPS) {
            fpsCamera.SetNearClipPlaneDistance(0.1f);
            fpsCamera.SetFarClipPlaneDistance(2000.0f);
        }
        else if (SceneConfiguration.CameraType == cameraType::ARCBALL) {
            arcballCamera.SetNearClipPlaneDistance(0.1f);
            arcballCamera.SetFarClipPlaneDistance(2000.0f);
        }

        CreateCommandPools();
        SetupRenderpass(BaseScene::SceneConfiguration.MSAA_SampleCount);
        setupGUI();
        SetupDepthStencil();
        SetupFramebuffers();
        RecreateObjects();
        vkDeviceWaitIdle(device->vkHandle());

        LOG_IF(BaseScene::SceneConfiguration.VerboseLogging, INFO) << "Swapchain recreation successful.";

    }

    void BaseScene::RenderLoop() {

        frameTime = static_cast<float>(BaseScene::SceneConfiguration.FrameTimeMs / 1000.0);
        LOG(INFO) << "Entering rendering loop.";

        while(!glfwWindowShouldClose(instance->GetWindow()->glfwWindow())) {

            limitFrame();

            glfwPollEvents();
            
            UpdateMouseActions();
            if(SceneConfiguration.EnableGUI) {
                gui->NewFrame(instance.get(), false);
            }

            UpdateMovement(static_cast<float>(BaseScene::SceneConfiguration.FrameTimeMs));
            
            RecordCommands();
            auto idx = submitFrame();
            submitExtra(idx);
            endFrame(idx);

            vkResetCommandPool(device->vkHandle(), secondaryPool->vkHandle(), VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT);
            vkResetCommandPool(device->vkHandle(), graphicsPool->vkHandle(), VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT);

            Buffer::DestroyStagingResources(device.get());

        }
    }

    void BaseScene::UpdateMovement(const float& delta_time) {

        frameTime = delta_time;
        ImGuiIO& io = ImGui::GetIO();
        io.DeltaTime = delta_time;

        if (io.WantCaptureKeyboard) {
            return;
        }

    }

    void BaseScene::limitFrame() {

        if(!BaseScene::SceneConfiguration.LimitFramerate) {
            return;
        }

        limiter_a = std::chrono::system_clock::now();
        std::chrono::duration<double, std::milli> work_time = limiter_a - limiter_b;
        
        if(work_time.count() < BaseScene::SceneConfiguration.FrameTimeMs) {
            std::chrono::duration<double, std::milli> delta_ms(BaseScene::SceneConfiguration.FrameTimeMs - work_time.count());
            auto delta_ms_dur = std::chrono::duration_cast<std::chrono::milliseconds>(delta_ms);
            std::this_thread::sleep_for(std::chrono::milliseconds(delta_ms_dur.count()));
        }

        limiter_b = std::chrono::system_clock::now();

    }

    uint32_t BaseScene::submitExtra(const uint32_t & frame_idx) {
        return frame_idx;
    }

    uint32_t BaseScene::submitFrame() {

        uint32_t image_idx;
        VkResult result = vkAcquireNextImageKHR(device->vkHandle(), swapchain->vkHandle(), std::numeric_limits<uint64_t>::max(), semaphores[0], acquireFence, &image_idx);
        VkAssert(result);
        result = vkWaitForFences(device->vkHandle(), 1, &acquireFence, VK_TRUE, vk_default_fence_timeout);
        VkAssert(result);

        VkSubmitInfo submit_info = vk_submit_info_base;
        VkPipelineStageFlags wait_stages[] = { VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT };
        submit_info.waitSemaphoreCount = 1;
        submit_info.pWaitSemaphores = &semaphores[0];
        submit_info.pWaitDstStageMask = wait_stages;
        submit_info.commandBufferCount = 1;
        submit_info.pCommandBuffers = &graphicsPool->GetCmdBuffer(image_idx);
        submit_info.signalSemaphoreCount = static_cast<uint32_t>(renderCompleteSemaphores.size());
        submit_info.pSignalSemaphores = renderCompleteSemaphores.data();
        result = vkQueueSubmit(device->GraphicsQueue(), 1, &submit_info, presentFences[image_idx]);
        switch(result) {
            case VK_ERROR_DEVICE_LOST:
                LOG(WARNING) << "vkQueueSubmit returned VK_ERROR_DEVICE_LOST";
                break;
            default:
                VkAssert(result);
                break;
        }

        VkPresentInfoKHR present_info{ VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
        present_info.waitSemaphoreCount = 1;
        present_info.pWaitSemaphores = &renderCompleteSemaphores[0];
        present_info.swapchainCount = 1;
        present_info.pSwapchains = &swapchain->vkHandle();
        present_info.pImageIndices = &image_idx;
        present_info.pResults = nullptr;

        result = vkQueuePresentKHR(device->GraphicsQueue(), &present_info);
        VkAssert(result);
        result = vkWaitForFences(device->vkHandle(), 1, &presentFences[image_idx], VK_TRUE, vk_default_fence_timeout);
        VkAssert(result);
        result = vkResetFences(device->vkHandle(), 1, &acquireFence);
        VkAssert(result);

        return image_idx;
    }

    void BaseScene::renderGUI(VkCommandBuffer& gui_buffer, const VkCommandBufferBeginInfo& begin_info, const size_t& frame_idx) const {

        if(!SceneConfiguration.EnableGUI) {
            LOG(ERROR) << "Tried to render the GUI, when the GUI is disabled in the SceneConfiguration!";
            throw std::runtime_error("Tried to render the GUI, when the GUI is disabled in the SceneConfiguration!");
        }

        ImGui::Render();
            
        if (device->MarkersEnabled) {
            device->vkCmdInsertDebugMarker(graphicsPool->GetCmdBuffer(frame_idx), "Update GUI", glm::vec4(0.6f, 0.6f, 0.0f, 1.0f));
        }

        gui->UpdateBuffers();

        vkBeginCommandBuffer(gui_buffer, &begin_info);

        if (device->MarkersEnabled) {
            device->vkCmdBeginDebugMarkerRegion(gui_buffer, "Draw GUI", glm::vec4(0.6f, 0.7f, 0.0f, 1.0f));
        }

        gui->DrawFrame(gui_buffer);

        if (device->MarkersEnabled) {
            device->vkCmdEndDebugMarkerRegion(gui_buffer);
        }

        vkEndCommandBuffer(gui_buffer);
    }

}
