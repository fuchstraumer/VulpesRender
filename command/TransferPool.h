#ifndef VULPES_VK_TRANSFER_POOL_H  
#define VULPES_VK_TRANSFER_POOL_H

#include "stdafx.h"
#include "../ForwardDecl.h"
#include "CommandPool.h"

namespace vulpes {

    /*
        Transfer pool is just a specialized command pool 
        with certain flags pre-set, along with a fence and easy
        tie-ins to the submitter object.

        This will be reset after each submission, and can be notified
        when resources are transferred or when they need to be transferred.
    */

	constexpr VkCommandPoolCreateInfo transfer_pool_info{
		VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
		nullptr,
		VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
		0,
	};

    class TransferPool : public CommandPool {
		TransferPool(TransferPool&& other) = delete;
		TransferPool& operator=(TransferPool&& other) = delete;
	public:

		TransferPool(const Device* _parent);

		~TransferPool() = default;

		VkCommandBuffer& Begin();

		void End();

		void Submit();

		VkCommandBuffer& CmdBuffer() noexcept;

	protected:
		VkFence fence;
		VkQueue queue;
	};

}

#endif //!VULPES_VK_TRANSFER_POOL_H