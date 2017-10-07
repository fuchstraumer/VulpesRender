#pragma once
#ifndef VULPES_VK_BASE_SCENE_H
#define VULPES_VK_BASE_SCENE_H

#include "vpr_stdafx.h"

#include "core/Instance.hpp"
#include "core/LogicalDevice.hpp"
#include "core/PhysicalDevice.hpp"
#include "render/Swapchain.hpp"
#include "render/Renderpass.hpp"
#include "render/Framebuffer.hpp"
#include "gui/imguiWrapper.hpp"
#include "command/CommandPool.hpp"
#include "render/DepthStencil.hpp"
#include "render/Multisampling.hpp"
#include "resource/PipelineCache.hpp"
#include "BaseSceneConfig.hpp"
#include "util/Camera.hpp"
#include "util/Arcball.hpp"

namespace vulpes {

    /** Scenes combine the various modules and objects of this library to create a fully renderable scene. Currently only the BaseScene class has been specified,
     *  but this class will simplify much of the setup work required and is extremely easy to extend and use. There is also currently a mostly-untested house demo
     *  scene, showing how this class can be derived from and extended.
     *  \defgroup Scenes
     */

    /** The BaseScene class takes care of setting up as much as it can, leaving only the calling of the framebuffer and renderpass creation methods to the user. It 
     *  calls suitable virtual methods to signal window resize and swapchain recreation events, and takes care of things like framerate locking (defaults to 60fps) to
     *  avoid overwhelming the CPU. 
     *  An important note is that this uses MSAA by default. Overriding this will require overriding the SetupRenderpass method, to avoid creating the 
     *  various subpasses, attachments, and other Vulkan objects required for this particular type of renderpass setup.
     *  \ingroup Scenes
     *  \todo Find a way to remove the requirement of initializing the renderpass and framebuffers in derived classes, to avoid confusion. 
     */
	class BaseScene {
	public:

        /** Creates a new BaseScene, initializing and setting up all but the Framebuffers and the Renderpass. 
         *  \param num_secondary_buffers: the amount of secondary command buffers to allocate. Using one secondary buffer per object is a solid bet.
         *  However, note that internally the actual allocated quantity of command buffers is what is passed in multiplied by the quantity of swapchain images.
         *  \param width: initial width of spawned GLFW window.
         *  \param height: initial height of spawned GLFW window.
        */
		BaseScene(const size_t& num_secondary_buffers, const uint32_t& width, const uint32_t& height);

		~BaseScene();

        virtual void CreateCommandPools();
        
        /** This must be called by derived classes.
         *  \param sample_count: the sampling level of the MSAA attachments. 
        */
		virtual void SetupRenderpass(const VkSampleCountFlagBits& sample_count);
		virtual void SetupDepthStencil();
		virtual void SetupFramebuffers();

        virtual void RecreateSwapchain();
        
        /** This method will be called when GLFW receives a window resize event. At this point, all resources created by a derived class should be destroyed.
         *  This includes things like DescriptorPool's, PipelineCache's, and any custom renderable objects that have been used.
         */
        virtual void WindowResized() = 0;
        /** This method is called after WindowResized, and after RecreateSwapchain. At this point, everything is back in order and the Scene is ready to continue rendering.
         *  Now is the time to re-init resources and objects, ensuring to re-upload their data to the device.  
         */
		virtual void RecreateObjects() = 0;

        /** This is called from the RenderLoop() method, and is where commands are recorded into VkCommandBuffer objects that describe the rendering to be done. See the example
         *  in HouseScene for clarification. 
         */
		virtual void RecordCommands() = 0;
        virtual void RenderLoop();

        virtual void UpdateMovement(const float & delta_time);

        void SetCameraTarget(const glm::vec3& target_pos);

        // updates mouse actions via ImGui.
		virtual void UpdateMouseActions();

		static void PipelineCacheCreated(const uint16_t& cache_id);

        glm::mat4 GetViewMatrix() const noexcept;
        glm::mat4 GetProjectionMatrix() const noexcept;
        const glm::vec3& GetCameraPosition() const noexcept;
        
        void SetCameraPosition(const glm::vec3& new_position) noexcept;

        static bool CameraLock;
        static vulpesSceneConfig SceneConfiguration;

	protected:

        void setupGUI();
        void destroyGUI();
        virtual void createGraphicsCmdPool();
        virtual void createTransferCmdPool();
        virtual void createSecondaryCmdPool();

        void cleanupShaderCacheFiles();
        virtual void mouseDown(const int& button, const float& x, const float& y);
        virtual void mouseUp(const int& button, const float& x, const float& y);
        virtual void mouseDrag(const int& button, const float& dx, const float& dy);
        virtual void mouseScroll(const int& button, const float& scroll_amount);

		virtual void limitFrame();
		// override in derived classes to perform extra work per frame. Does nothing by default.
		virtual uint32_t submitExtra(const uint32_t& frame_idx);
		virtual uint32_t submitFrame();
		// Call ImGui drawing functions (like ImGui::ShowMainMenuBar(), etc) here.
        virtual void renderGUI(VkCommandBuffer& gui_buffer, const VkCommandBufferBeginInfo& begin_info, const size_t& frame_idx) const;
        
        /** This method is called from the RenderLoop() method as well, before resetting the command pools and continuing to the next frame. Use this
         *  to reset any frame resources, or to wait for fences that are used to guard frame-only commands. Use the frame_idx to ensure that only items
         *  belonging to the presented frame/framebuffer are reset. 
         */
		virtual void endFrame(const size_t& curr_idx) = 0;

		std::unique_ptr<Multisampling> msaa;
		std::unique_ptr<imguiWrapper> gui;
		uint32_t width, height;
		VkSemaphore semaphores[2];
		std::vector<VkSemaphore> renderCompleteSemaphores;
		std::unique_ptr<Instance> instance;
		std::unique_ptr<Device> device;
		std::unique_ptr<Swapchain> swapchain;
		std::vector<VkFramebuffer> framebuffers;
		std::unique_ptr<DepthStencil> depthStencil;
		std::unique_ptr<CommandPool> graphicsPool, secondaryPool;
		std::unique_ptr<TransferPool> transferPool;
		std::unique_ptr<Renderpass> renderPass;

		std::vector<VkAttachmentDescription> attachmentDescriptions;
		std::vector<VkAttachmentReference> attachmentReferences;
		std::vector<VkSubpassDependency> subpassDependencies;

		std::chrono::system_clock::time_point limiter_a, limiter_b;
		double desiredFrameTimeMs = 16.0;
		std::vector<VkFence> presentFences;
		VkFence acquireFence;

		float frameTime;
		virtual void createRenderTargetAttachment();
		virtual void createResolveAttachment();
		virtual void createMultisampleDepthAttachment();
		virtual void createResolveDepthAttachment();
		virtual void createAttachmentReferences();
		virtual VkSubpassDescription createSubpassDescription();
		virtual void createSubpassDependencies();

		static std::vector<uint16_t> pipelineCacheHandles;
		size_t numSecondaryBuffers;
        
        static PerspectiveCamera fpsCamera;
        static ArcballCamera arcballCamera;
	};


    inline glm::mat4 BaseScene::GetViewMatrix() const noexcept {
        if (SceneConfiguration.CameraType == cameraType::FPS) {
            return fpsCamera.GetViewMatrix();
        }
        else if (SceneConfiguration.CameraType == cameraType::ARCBALL) {
            return arcballCamera.GetViewMatrix();
        }
    }

    inline glm::mat4 BaseScene::GetProjectionMatrix() const noexcept {
        if (SceneConfiguration.CameraType == cameraType::FPS) {
            return fpsCamera.GetProjectionMatrix();
        }
        else if (SceneConfiguration.CameraType == cameraType::ARCBALL) {
            return arcballCamera.GetProjectionMatrix();
        }
    }

    inline const glm::vec3 & BaseScene::GetCameraPosition() const noexcept {
        if (SceneConfiguration.CameraType == cameraType::FPS) {
            return fpsCamera.GetEyeLocation();
        }
        else if (SceneConfiguration.CameraType == cameraType::ARCBALL) {
            return arcballCamera.GetEyeLocation();
        }
    }

    inline void BaseScene::SetCameraPosition(const glm::vec3& new_camera_pos) noexcept {
        if (SceneConfiguration.CameraType == cameraType::FPS) {
            fpsCamera.SetEyeLocation(new_camera_pos);
        }
        else if (SceneConfiguration.CameraType == cameraType::ARCBALL) {
            arcballCamera.SetEyeLocation(new_camera_pos);
        }
    }

    inline void BaseScene::SetCameraTarget(const glm::vec3& target_pos) {
        if (SceneConfiguration.CameraType == cameraType::FPS) {
            fpsCamera.LookAtTarget(target_pos);
        }
        else if (SceneConfiguration.CameraType == cameraType::ARCBALL) {
            arcballCamera.LookAtTarget(target_pos);
        }
    }

}

#endif // !VULPES_VK_BASE_SCENE_H
