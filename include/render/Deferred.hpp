#pragma once
#ifndef VULPES_VK_DEFERRED_H
#define VULPES_VK_DEFERRED_H

#include "vpr_stdafx.h"
#include "../ForwardDecl.h"
#include "OffscreenFramebuffers.h"

namespace vulpes {

	class DeferredPass {
		DeferredPass(const DeferredPass&) = delete;
		DeferredPass& operator=(const DeferredPass&) = delete;
		friend class GBuffers;
	public:

		DeferredPass(const Device* device);

		void Init(VkCommandBuffer& deferred_buffer);

		void Begin(const size_t& frame_idx);
		void End();

		// resets command buffer and resets semaphore.
		void Reset();

		VkRenderPass Renderpass;
		VkRenderPassBeginInfo RenderpassBeginInfo;
		GBuffers RenderTarget;
		VkSemaphore Semaphore = VK_NULL_HANDLE;
		VkCommandBufferInheritanceInfo CmdBufferInheritanceInfo;
		VkCommandBuffer CommandBuffer;
		VkCommandBufferBeginInfo CmdBufferBeginInfo;

	private:

		void setClearValues();
		void setRenderpassBeginInfo();
		void setInheritanceInfo();

		std::vector<VkCommandBuffer> secondaryBuffers;
		std::vector<VkClearValue> clearValues;
		const Device* parent;
	};

	inline DeferredPass::DeferredPass(const Device * device) : Parent(device), RenderTarget(device, vk_framebuffer_create_info_base) {}
	
	void DeferredPass::Init(VkCommandBuffer& deferred_buffer) {

		CommandBuffer = deferred_buffer;
		CmdBufferBeginInfo = vk_command_buffer_begin_info_base;
		CmdBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT | VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

		static const VkSemaphoreCreateInfo semaphore_info{ VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO, nullptr, 0 };
		VkResult result = vkCreateSemaphore(Parent->vkHandle(), &semaphore_info, nullptr, &Semaphore);
		VkAssert(result);

		setClearValues();
		setRenderpassBeginInfo();
		setInheritanceInfo();

	}


	void DeferredPass::setClearValues() {

		clearValues.resize(5);
		clearValues[0] = VkClearValue{ 0.0f, 0.0f, 0.0f, 0.0f };
		clearValues[1] = VkClearValue{ 0.0f, 0.0f, 0.0f, 0.0f };
		clearValues[2] = VkClearValue{ 0.0f, 0.0f, 0.0f, 0.0f };
		clearValues[3] = VkClearValue{ 0.0f, 0.0f, 0.0f, 0.0f };
		clearValues[4] = VkClearValue{ 1.0f, 0 };

	}

	void DeferredPass::setRenderpassBeginInfo() {

		RenderpassBeginInfo = vk_renderpass_begin_info_base;
		RenderpassBeginInfo.renderPass = RenderTarget.renderpass;
		RenderpassBeginInfo.renderArea.extent.width = RenderTarget.extents.width;
		RenderpassBeginInfo.renderArea.extent.height = RenderTarget.extents.height;
		RenderpassBeginInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
		RenderpassBeginInfo.pClearValues = clearValues.data();

	}

	void DeferredPass::setInheritanceInfo() {

		CmdBufferInheritanceInfo = vk_command_buffer_inheritance_info_base;
		CmdBufferInheritanceInfo.renderPass = RenderTarget.renderpass;
		CmdBufferInheritanceInfo.framebuffer = VK_NULL_HANDLE;
		CmdBufferInheritanceInfo.subpass = 0;

	};

	void DeferredPass::Begin(const size_t& frame_idx) {

		vkBeginCommandBuffer(CommandBuffer, &CmdBufferBeginInfo);
		CmdBufferInheritanceInfo.framebuffer = RenderTarget.framebuffers[frame_idx];
		vkCmdBeginRenderPass(CommandBuffer, &RenderpassBeginInfo, VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);

	}

	inline void DeferredPass::End() {

		vkCmdExecuteCommands(CommandBuffer, static_cast<uint32_t>(secondaryBuffers.size()), secondaryBuffers.data());
		vkCmdEndRenderPass(CommandBuffer);
		vkEndCommandBuffer(CommandBuffer);

	}

	inline void DeferredPass::Reset() {

		VkResult result = vkResetCommandBuffer(CommandBuffer, VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);
		VkAssert(result);

		secondaryBuffers.clear();
		secondaryBuffers.shrink_to_fit();

	}

}

#endif // !VULPES_VK_DEFERRED_H
