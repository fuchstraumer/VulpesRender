#pragma once
#ifndef VULPES_VK_TRANSFER_POOL_H  
#define VULPES_VK_TRANSFER_POOL_H
#include "vpr_stdafx.h"
#include "ForwardDecl.hpp"
#include "CommandPool.hpp"
#include <mutex>

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

    class VPR_API TransferPool : public CommandPool {
        TransferPool(TransferPool&& other) = delete;
        TransferPool& operator=(TransferPool&& other) = delete;
    public:

        TransferPool(const Device* _parent);
        TransferPool(const Device* _parent, const size_t& num_buffers);

        ~TransferPool();

        /** Returns the command buffer associated with this pool, call VkBeginCommandBuffer
        *   before doing so. Locks the mutex as well.
        */
        const VkCommandBuffer& Begin() const;

        /** Begins all command buffers attached to this object. Intended for use when one plans
         *  to record transfer commands until multiple buffers in parallel, then submit all these
         *  transfers at once.
         */
        const VkCommandBuffer* BeginAll() const;

        /** Calls VkEndCommandBuffer on this object's command buffer, then proceeds to submit it. This 
        *   transfer will wait on the VkFence object attached to this pool, resetting the owned command 
        *   buffer once done and unlocks the mutex.
        */
        void Submit() const;

        /** Submits but waits for the given semaphore to be signalled
         */
        void SubmitWait(const VkSemaphore wait_semaphore, const VkPipelineStageFlags wait_flags) const;

        /** Submits all command buffers contained by this transfer pool. Waits on a fence and uses a mutex,
         *  just like the method for singular command buffers.
         */
        void SubmitAll() const;

    protected:
        
        /** Just a convienience method wrapping the command code required to reset the fence, reset the 
         *  command buffer(s), and make sure to unlock the transfer mutex.
         */
        void completeTransfer() const;
        mutable std::mutex transferMutex;
        VkFence fence;
        VkQueue queue;
    };

}

#endif //!VULPES_VK_TRANSFER_POOL_H