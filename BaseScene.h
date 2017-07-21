#pragma once
#ifndef VULPES_VK_BASE_SCENE_H
#define VULPES_VK_BASE_SCENE_H

#include "stdafx.h"

#include "engine\renderer\core\Instance.h"
#include "engine\renderer\core\LogicalDevice.h"
#include "engine\renderer\core\PhysicalDevice.h"
#include "engine\renderer\render\Swapchain.h"
#include "engine\renderer\render\Renderpass.h"
#include "engine\renderer\render\Framebuffer.h"
#include "engine\renderer\command\CommandPool.h"
#include "engine\renderer\render\DepthStencil.h"
#include "engine\renderer\render\Multisampling.h"
#include "engine/renderer/resource/PipelineCache.h"

namespace vulpes {

	class BaseScene {
	public:

		BaseScene(const size_t& num_secondary_buffers = 1, const uint32_t& width = DEFAULT_WIDTH, const uint32_t& height = DEFAULT_HEIGHT);

		~BaseScene();

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
		imguiWrapper* gui;
		uint32_t width, height;
		VkSemaphore semaphores[2];
		InstanceGLFW* instance;
		Device* device;
		Swapchain* swapchain;
		std::vector<VkFramebuffer> framebuffers;
		DepthStencil* depthStencil;
		CommandPool *transferPool, *graphicsPool, *secondaryPool;
		Renderpass* renderPass;
		float frameTime;
	};

}

#endif // !VULPES_VK_BASE_SCENE_H
