#pragma once
#ifndef VULPES_VK_COMMAND_POOL_H
#define VULPES_VK_COMMAND_POOL_H
#include "vpr_stdafx.h"
#include "../ForwardDecl.hpp"

namespace vulpes {

	class CommandPool {
		CommandPool(const CommandPool&) = delete;
		CommandPool& operator=(const CommandPool&) = delete;
	public:

		CommandPool(const Device* parent, const VkCommandPoolCreateInfo& create_info, bool primary);
		CommandPool(const Device* parent, bool primary);
		CommandPool(CommandPool&& other) noexcept;
		CommandPool& operator=(CommandPool&& other) noexcept;

		virtual ~CommandPool();

		void Destroy();
		void Create();

		void AllocateCmdBuffers(const uint32_t& num_buffers, const VkCommandBufferAllocateInfo& alloc_info = vk_command_buffer_allocate_info_base);
		void ResetCmdPool();
		void ResetCmdBuffer(const size_t& idx);
        void FreeCommandBuffers();
    
		const VkCommandPool& vkHandle() const noexcept;

		VkCommandBuffer& operator[](const size_t& idx);
        VkCommandBuffer& GetCmdBuffer(const size_t& idx);
        std::vector<VkCommandBuffer> GetCommandBuffers(const size_t& num, const size_t& offset);

		std::pair<size_t, VkCommandBuffer> GetAvailCmdBuffer();

		VkCommandBuffer StartSingleCmdBuffer();
		void EndSingleCmdBuffer(VkCommandBuffer& cmd_buffer, const VkQueue & queue);

		const size_t size() const noexcept;

	protected:

		std::vector<VkCommandBuffer> cmdBuffers;
		std::vector<bool> bufferInUse;
		VkCommandPool handle;
		VkCommandPoolCreateInfo createInfo;
		const Device* parent;
		const VkAllocationCallbacks* allocators = nullptr;
		bool primary;

	};


	
}
#endif // !VULPES_VK_COMMAND_POOL_H
