#pragma once
#ifndef VULPES_VK_BASE_SCENE_H
#define VULPES_VK_BASE_SCENE_H

#include "vpr_stdafx.h"

#include "core/Instance.h"
#include "core/LogicalDevice.h"
#include "core/PhysicalDevice.h"
#include "render/Swapchain.h"
#include "render/Renderpass.h"
#include "render/Framebuffer.h"
#include "command/CommandPool.h"
#include "render/DepthStencil.h"
#include "render/Multisampling.h"
#include "resource/PipelineCache.h"

namespace vulpes {

	class BaseScene {
	public:

		BaseScene(const size_t& num_secondary_buffers = 1, const uint32_t& width = DEFAULT_WIDTH, const uint32_t& height = DEFAULT_HEIGHT);

		~BaseScene();

		// updates mouse actions via ImGui.
		void UpdateMouseActions();

		virtual void CreateCommandPools(const size_t& num_secondary_buffers);

		virtual void SetupRenderpass(const VkSampleCountFlagBits& sample_count = VK_SAMPLE_COUNT_8_BIT);

		virtual void SetupDepthStencil();

		virtual void SetupFramebuffers();

		virtual void RecreateSwapchain();

		virtual void WindowResized() = 0;

		virtual void RecreateObjects() = 0;

		virtual void RecordCommands() = 0;

		virtual float GetFrameTime();

	protected:
		std::unique_ptr<Multisampling> msaa;
		std::unique_ptr<imguiWrapper> gui;
		uint32_t width, height;
		VkSemaphore semaphores[2];
		std::unique_ptr<InstanceGLFW> instance;
		std::unique_ptr<Device> device;
		std::unique_ptr<Swapchain> swapchain;
		std::vector<VkFramebuffer> framebuffers;
		std::unique_ptr<DepthStencil> depthStencil;
		std::unique_ptr<CommandPool> graphicsPool, secondaryPool;
		std::unique_ptr<TransferPool> transferPool;
		std::unique_ptr<Renderpass> renderPass;

		std::array<VkAttachmentDescription, 4> attachmentDescriptions;
		std::array<VkAttachmentReference, 3> attachmentReferences;
		std::array<VkSubpassDependency, 2> subpassDependencies;

		float frameTime;
		void createRenderTargetAttachment();
		void createResolveAttachment();
		void createMultisampleDepthAttachment();
		void createResolveDepthAttachment();
		void createAttachmentReferences();
		VkSubpassDescription createSubpassDescription();
		void createSubpassDependencies();
	};

}

#endif // !VULPES_VK_BASE_SCENE_H
