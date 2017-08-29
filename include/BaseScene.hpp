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
#include "command/CommandPool.hpp"
#include "render/DepthStencil.hpp"
#include "render/Multisampling.hpp"
#include "resource/PipelineCache.hpp"

namespace vulpes {

	class BaseScene {
	public:

		BaseScene(const size_t& num_secondary_buffers = 1, const uint32_t& width = DEFAULT_WIDTH, const uint32_t& height = DEFAULT_HEIGHT);

		~BaseScene();

		virtual void CreateCommandPools();
		virtual void SetupRenderpass(const VkSampleCountFlagBits& sample_count);
		virtual void SetupDepthStencil();
		virtual void SetupFramebuffers();

		virtual void RecreateSwapchain();
		virtual void WindowResized() = 0;
		virtual void RecreateObjects() = 0;

		virtual void RecordCommands() = 0;
		virtual void RenderLoop();

		// updates mouse actions via ImGui.
		virtual void UpdateMouseActions();

		static void PipelineCacheCreated(const uint16_t& cache_id);

	protected:

		virtual void limitFrame();
		// override in derived classes to perform extra work per frame. Does nothing by default.
		virtual uint32_t submitExtra(const uint32_t& frame_idx);
		virtual uint32_t submitFrame();
		// Call ImGui drawing functions (like ImGui::ShowMainMenuBar(), etc) here.
		virtual void imguiDrawcalls() = 0;
		virtual void renderGUI(VkCommandBuffer& gui_buffer, const VkCommandBufferBeginInfo& begin_info, const size_t& frame_idx) const;
		virtual void endFrame(const size_t& curr_idx) = 0;

		std::unique_ptr<Multisampling> msaa;
		std::unique_ptr<imguiWrapper> gui;
		uint32_t width, height;
		VkSemaphore semaphores[2];
		std::vector<VkSemaphore> renderCompleteSemaphores;
		std::unique_ptr<InstanceGLFW> instance;
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
	};

}

#endif // !VULPES_VK_BASE_SCENE_H