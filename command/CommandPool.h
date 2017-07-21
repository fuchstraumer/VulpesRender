#pragma once
#ifndef VULPES_VK_COMMAND_POOL_H
#define VULPES_VK_COMMAND_POOL_H
#include "stdafx.h"
#include "engine/renderer/ForwardDecl.h"
#include "engine/renderer/NonCopyable.h"

namespace vulpes {

	class CommandPool : public NonCopyable {
	public:

		CommandPool(const Device* parent, const VkCommandPoolCreateInfo& create_info, bool primary);
		CommandPool(const Device* parent, bool primary);
		CommandPool(CommandPool&& other) noexcept;
		CommandPool& operator=(CommandPool&& other) noexcept;

		virtual ~CommandPool();
		// Resets the CommandPool to the state it's in when constructed: invalid for use.
		void Destroy();

		// Creates command pool handle: does not allocate buffers.
		void Create();

		void AllocateCmdBuffers(const uint32_t& num_buffers, const VkCommandBufferAllocateInfo& alloc_info = vk_command_buffer_allocate_info_base);

		void ResetCmdPool();
		void ResetCmdBuffer(const size_t& idx);

		// Freeing command buffers makes them invalid, unlike resetting.
		void FreeCommandBuffers();

		const VkCommandPool& vkHandle() const noexcept;

		VkCommandBuffer& operator[](const size_t& idx);
		VkCommandBuffer& GetCmdBuffer(const size_t& idx);
		const std::vector<VkCommandBuffer>& GetCmdBuffers() const;

		VkCommandBuffer StartSingleCmdBuffer();
		void EndSingleCmdBuffer(VkCommandBuffer& cmd_buffer, const VkQueue & queue);

		const size_t size() const noexcept;

	protected:

		std::vector<VkCommandBuffer> cmdBuffers;
		VkCommandPool handle;
		VkCommandPoolCreateInfo createInfo;
		const Device* parent;
		const VkAllocationCallbacks* allocators = nullptr;
		bool primary;

	};


	
}
#endif // !VULPES_VK_COMMAND_POOL_H
