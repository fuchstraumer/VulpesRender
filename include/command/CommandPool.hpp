#pragma once
#ifndef VULPES_VK_COMMAND_POOL_H
#define VULPES_VK_COMMAND_POOL_H
#include "vpr_stdafx.h"
#include "../ForwardDecl.hpp"

namespace vulpes {

    /**! The Command group encompasses classes related to recording commands, submitting commands, and allocating/freeing/resetting VkCommandBuffer objects.
    *   \defgroup Command
    */

    /**! The CommandPool class is the primary interface through which one will acquire VkCommandBuffer objects. The quantity of command buffers is not selected
    *    upon construction: only the command buffer level, primary or secondary. Before use, one must call AllocateCmdBuffers or risk exceptions. StartSingleCmdBuffer
    *    and EndSingleCmdBuffer can be used to retrieve (relatively wasteful, don't do it while rendering) single-shot command buffers for things like binding resources
    *    to sparse buffers, submitting transfers, or performing image layout transitions.'
    *    \todo Remove the bool "primary" index from the constructor, only accepting the VkCommandPoolCreateInfo struct
    *    \todo Remove redundant num_buffers parameter from AllocateCmdBuffers, make the primary/secondary level selected upon alloc not construction.
    *    \ingroup Command
    */
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
        
        /**!Resets the entire command pool via a call to VkResetCommandPool. Uses VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT by default, which releases
        *   all resources that the Vulkan implementation internally allocates. This may take time, and may require re-allocation upon reinitialization
        *   but also prevents memory fragmentation when using command pools for quite some time.
        *   \param command_pool_reset_flag_bits - Only current options are VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT, the default, or no flags (thus, no resources released)
        */
        void ResetCmdPool(const VkCommandPoolResetFlagBits& command_pool_reset_flag_bits = VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT);
        
        /**!Resets the single command buffer at the given index. Uses VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT by default, which returns the resources
        *   allocated by the Vulkan implementation for this particular command buffer back to the parent pool. Be careful using this in a pool that doesn't 
        *   have other buffers reset: this can cause memory fragmentation in the command buffer memory resources, which will increase the time it takes for 
        *   the Vulkan implementation to find a suitable memory location to use (or it will have to allocate more memory).
        *   \param command_buffer_reset_flag_bits - Only current options are VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT, the default, or no flags (thus, no resources released)
        */
		void ResetCmdBuffer(const size_t& idx, const VkCommandBufferResetFlagBits& command_buffer_reset_flag_bits = VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);
        
        /**!Frees the memory used for all command buffers in this pool, which effectively "deletes" them, unlike resetting a single command buffer or even resetting
        *   the entire pool. This will require calling AllocateCmdBuffers again, as it ultimately resets the object into its base state.
        */
        void FreeCommandBuffers();
    
		const VkCommandPool& vkHandle() const noexcept;

		VkCommandBuffer& operator[](const size_t& idx);
        VkCommandBuffer& GetCmdBuffer(const size_t& idx);

        /**!Gets a range of command buffers, starting at the given offset and extending "num" command buffers beyond it.
        *
        */
        std::vector<VkCommandBuffer> GetCommandBuffers(const size_t& num, const size_t& offset);

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
