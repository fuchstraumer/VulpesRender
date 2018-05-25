#include "vpr_stdafx.h"
#include "command/CommandPool.hpp"
#include "core/Instance.hpp"
#include "core/LogicalDevice.hpp"
#include "easylogging++.h"

namespace vpr {

    CommandPool::CommandPool(const Device * _parent, const VkCommandPoolCreateInfo & create_info) : parent(_parent), handle(VK_NULL_HANDLE) {
        vkCreateCommandPool(parent->vkHandle(), &create_info, nullptr, &handle);
    }


    void CommandPool::ResetCmdPool(const VkCommandPoolResetFlagBits& command_pool_reset_flags) {
        vkResetCommandPool(parent->vkHandle(), handle, command_pool_reset_flags);
    }

    CommandPool::CommandPool(CommandPool && other) noexcept{
        handle = std::move(other.handle);
        cmdBuffers = std::move(other.cmdBuffers);
        parent = std::move(other.parent);
        other.handle = VK_NULL_HANDLE;
    }

    CommandPool & CommandPool::operator=(CommandPool && other) noexcept{
        handle = std::move(other.handle);
        cmdBuffers = std::move(other.cmdBuffers);
        parent = std::move(other.parent);
        other.handle = VK_NULL_HANDLE;
        return *this;
    }

    CommandPool::~CommandPool(){
        destroy();
    }

    CommandPool::CommandPool(const Device * _parent) : parent(_parent), handle(VK_NULL_HANDLE) {
    }

    void CommandPool::destroy(){
        if (!cmdBuffers.empty()) {
            FreeCommandBuffers();
        }
        if (handle != VK_NULL_HANDLE) {
            vkDestroyCommandPool(parent->vkHandle(), handle, nullptr);
            LOG(INFO) << "Command Pool " << handle << " destroyed.";
            handle = VK_NULL_HANDLE;
        }
    }

    void CommandPool::AllocateCmdBuffers(const uint32_t & num_buffers, const VkCommandBufferLevel& cmd_buffer_level){

        if (!cmdBuffers.empty()) {
            return;
        }

        cmdBuffers.resize(num_buffers);
        VkCommandBufferAllocateInfo alloc_info = vk_command_buffer_allocate_info_base;
        alloc_info.commandPool = handle;
        alloc_info.commandBufferCount = num_buffers;
        alloc_info.level = cmd_buffer_level;
        VkResult result = vkAllocateCommandBuffers(parent->vkHandle(), &alloc_info, cmdBuffers.data());
        LOG(INFO) << std::to_string(num_buffers) << " command buffers allocated for command pool " << handle;
        VkAssert(result);
    }

    void CommandPool::FreeCommandBuffers(){
        vkFreeCommandBuffers(parent->vkHandle(), handle, static_cast<uint32_t>(cmdBuffers.size()), cmdBuffers.data());
        LOG(INFO) << std::to_string(cmdBuffers.size()) << " command buffers freed.";
        cmdBuffers.clear();
        cmdBuffers.shrink_to_fit();
    }

    void CommandPool::ResetCmdBuffer(const size_t & idx, const VkCommandBufferResetFlagBits& command_buffer_reset_flag_bits) {
        vkResetCommandBuffer(cmdBuffers[idx], command_buffer_reset_flag_bits);
    }
    
    const VkCommandPool & CommandPool::vkHandle() const noexcept{
        return handle;
    }

    const VkCommandBuffer* CommandPool::GetCommandBuffers(const size_t& offset) const {
        return &cmdBuffers[offset];
    }

    VkCommandBuffer & CommandPool::operator[](const size_t & idx) {
        return cmdBuffers[idx];
    }

    VkCommandBuffer & CommandPool::GetCmdBuffer(const size_t & idx) {
        return cmdBuffers[idx];
    }

    VkCommandBuffer CommandPool::StartSingleCmdBuffer(){
        VkCommandBufferAllocateInfo allocInfo = {};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandPool = handle;
        allocInfo.commandBufferCount = 1;

        VkCommandBuffer commandBuffer;
        vkAllocateCommandBuffers(parent->vkHandle(), &allocInfo, &commandBuffer);

        VkCommandBufferBeginInfo beginInfo = {};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        beginInfo.pInheritanceInfo = nullptr;

        vkBeginCommandBuffer(commandBuffer, &beginInfo);

        return commandBuffer;
    }

    void CommandPool::EndSingleCmdBuffer(VkCommandBuffer& cmd_buffer, const VkQueue & queue) {
        VkResult result = vkEndCommandBuffer(cmd_buffer);
        VkAssert(result);

        VkSubmitInfo submitInfo = {};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &cmd_buffer;

        result = vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
        VkAssert(result);
        vkQueueWaitIdle(queue);
        
        result = vkResetCommandBuffer(cmd_buffer, VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);
        VkAssert(result);
    }

    const size_t CommandPool::size() const noexcept{
        return cmdBuffers.size();
    }

    const VkCommandBuffer * CommandPool::Data() const noexcept {
        return cmdBuffers.data();
    }

}
