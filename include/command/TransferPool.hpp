#pragma once
#ifndef VULPES_VK_TRANSFER_POOL_H  
#define VULPES_VK_TRANSFER_POOL_H
#include "vpr_stdafx.h"
#include "ForwardDecl.hpp"
#include "CommandPool.hpp"

namespace vpr {

    /** Transfer pool is just a specialized command pool 
    *   with certain flags pre-set, along with a fence protecting
    *  the transfer submission and a mutex for thread safety.
    *
    *   This will be reset after each submission.
    *   \ingroup Command
    */

    constexpr static VkCommandPoolCreateInfo transfer_pool_info{
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

        ~TransferPool();

        /** Returns the command buffer associated with this pool, call VkBeginCommandBuffer
        *   before doing so. Locks the mutex as well.
        */
        VkCommandBuffer& Begin();

        /** Calls VkEndCommandBuffer on this object's command buffer, then proceeds to submit it. This 
        *   transfer will wait on the VkFence object attached to this pool, resetting the owned command 
        *   buffer once done and unlocks the mutex.
        */
        void Submit();

    protected:
        std::mutex transferMutex;
        VkFence fence;
        VkQueue queue;
    };

}

#endif //!VULPES_VK_TRANSFER_POOL_H