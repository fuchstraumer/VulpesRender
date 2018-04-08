#include "vpr_stdafx.h"
#include "command/TransferPool.hpp"
#include "core/LogicalDevice.hpp"

namespace vpr {

    constexpr static VkCommandPoolCreateInfo transfer_pool_info{
        VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        nullptr,
        VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        0,
    };

    TransferPool::TransferPool(const Device * _parent) : CommandPool(_parent), queue(_parent->TransferQueue(0)) {
        createPool();
        AllocateCmdBuffers(1, VK_COMMAND_BUFFER_LEVEL_PRIMARY);
        createFence();
    }

    TransferPool::TransferPool(const Device * _parent, const size_t & num_buffers) : CommandPool(_parent), queue(_parent->TransferQueue(0)) {
        createPool();
        AllocateCmdBuffers(static_cast<uint32_t>(num_buffers), VK_COMMAND_BUFFER_LEVEL_PRIMARY);
        createFence();
    }

    TransferPool::~TransferPool() {
        if (fence != VK_NULL_HANDLE) {
            vkDestroyFence(parent->vkHandle(), fence, nullptr);
            fence = VK_NULL_HANDLE;
        }
    }

    const VkCommandBuffer& TransferPool::Begin() const {

        transferMutex.lock();
        VkCommandBufferBeginInfo beginInfo = {};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT | VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
        beginInfo.pInheritanceInfo = nullptr;

        vkBeginCommandBuffer(cmdBuffers.front(), &beginInfo);
        
        return cmdBuffers.front();
    }

    const VkCommandBuffer* TransferPool::BeginAll() const {
        assert(cmdBuffers.size() > 1);
        constexpr static VkCommandBufferBeginInfo begin_info{
            VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO, 
            nullptr,
            VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT | VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT,
            nullptr
        };

        for(auto& buff : cmdBuffers) {
            vkBeginCommandBuffer(buff, &begin_info);
        }

        return cmdBuffers.data();
    }
    
    void TransferPool::Submit() const {

        VkResult result = vkEndCommandBuffer(cmdBuffers.front());
        VkAssert(result);

        VkSubmitInfo submitInfo = {};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = cmdBuffers.data();

        result = vkQueueSubmit(queue, 1, &submitInfo, fence);
        VkAssert(result);

        completeTransfer();
        
    }

    void TransferPool::SubmitWait(const VkSemaphore semaphore, const VkPipelineStageFlags wait_flags) const {

        VkResult result = vkEndCommandBuffer(cmdBuffers.front());
        VkAssert(result);

        VkSubmitInfo submit_info{ 
            VK_STRUCTURE_TYPE_SUBMIT_INFO, nullptr, 
            1, &semaphore, &wait_flags,  
            1, &cmdBuffers.front(), 
            0, nullptr };

        result = vkQueueSubmit(queue, 1, &submit_info, fence);
        VkAssert(result);

        completeTransfer();
    }

    void TransferPool::SubmitAll() const {

        // Unlike the other singular submit technique, we expect
        // that other threads have been accessing members of this
        // class (the retrieved buffers) up to this point, so we
        // delay locking the mutex until before we submit.

        transferMutex.lock();

        for(auto& buff : cmdBuffers) {
            vkEndCommandBuffer(buff);
        }

        const VkSubmitInfo submit_info{
            VK_STRUCTURE_TYPE_SUBMIT_INFO,
            nullptr,
            0,
            nullptr,
            nullptr,
            static_cast<uint32_t>(cmdBuffers.size()),
            cmdBuffers.data(),
            0,
            nullptr
        };

        VkResult result = vkQueueSubmit(queue, 1, &submit_info, fence);
        VkAssert(result);

        completeTransfer();
        
    }

    void TransferPool::createPool() {
        VkCommandPoolCreateInfo create_info = vk_command_pool_info_base;
        create_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT | VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
        create_info.queueFamilyIndex = parent->QueueFamilyIndices.Transfer;
        VkResult result = vkCreateCommandPool(parent->vkHandle(), &create_info, nullptr, &handle);
        VkAssert(result);
    }

    void TransferPool::createFence() {
        VkResult result = vkCreateFence(parent->vkHandle(), &vk_fence_create_info_base, nullptr, &fence);
        VkAssert(result);
    }

    void TransferPool::completeTransfer() const {   
        VkResult result = vkWaitForFences(parent->vkHandle(), 1, &fence, VK_TRUE, static_cast<uint64_t>(2e9));
        VkAssert(result);
        result = vkResetFences(parent->vkHandle(), 1, &fence);
        vkResetCommandPool(parent->vkHandle(), handle, VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT);
        transferMutex.unlock();
    }

}