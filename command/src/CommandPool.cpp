#include "vpr_stdafx.h"
#include "CommandPool.hpp"
#include "vkAssert.hpp"
#include "CreateInfoBase.hpp"
#include "easylogging++.h"
#if !defined(VPR_BUILD_STATIC)
INITIALIZE_EASYLOGGINGPP
#endif
#include <vector>
namespace vpr {

    void SetLoggingRepository_VprCommand(void* repo) {
        el::Helpers::setStorage(*(el::base::type::StoragePointer*)repo);
        LOG(INFO) << "Updating easyloggingpp storage pointer in vpr_command module...";
    }

    struct CommandBuffers  {
        std::vector<VkCommandBuffer> Data;
    };

    CommandPool::CommandPool(const VkDevice _parent, const VkCommandPoolCreateInfo & create_info) : parent(_parent), handle(VK_NULL_HANDLE), cmdBuffers(std::make_unique<CommandBuffers>()) {
        vkCreateCommandPool(parent, &create_info, nullptr, &handle);
    }

    void CommandPool::ResetCmdPool(const VkCommandPoolResetFlagBits& command_pool_reset_flags) {
        vkResetCommandPool(parent, handle, command_pool_reset_flags);
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

    void CommandPool::destroy(){
        if (!cmdBuffers->Data.empty()) {
            FreeCommandBuffers();
        }
        if (handle != VK_NULL_HANDLE) {
            vkDestroyCommandPool(parent, handle, nullptr);
            LOG_IF(VERBOSE_LOGGING, INFO) << "Command Pool " << handle << " destroyed.";
            handle = VK_NULL_HANDLE;
        }
    }

    void CommandPool::AllocateCmdBuffers(const uint32_t & num_buffers, const VkCommandBufferLevel& cmd_buffer_level){

        if (!cmdBuffers->Data.empty()) {
            return;
        }

        cmdBuffers->Data.resize(num_buffers);
        VkCommandBufferAllocateInfo alloc_info = vk_command_buffer_allocate_info_base;
        alloc_info.commandPool = handle;
        alloc_info.commandBufferCount = num_buffers;
        alloc_info.level = cmd_buffer_level;
        VkResult result = vkAllocateCommandBuffers(parent, &alloc_info, cmdBuffers->Data.data());
        LOG_IF(VERBOSE_LOGGING, INFO) << std::to_string(num_buffers) << " command buffers allocated for command pool " << handle;
        VkAssert(result);
    }

    void CommandPool::FreeCommandBuffers(){
        vkFreeCommandBuffers(parent, handle, static_cast<uint32_t>(cmdBuffers->Data.size()), cmdBuffers->Data.data());
        LOG_IF(VERBOSE_LOGGING, INFO) << std::to_string(cmdBuffers->Data.size()) << " command buffers freed.";
        cmdBuffers->Data.clear();
        cmdBuffers->Data.shrink_to_fit();
    }

    void CommandPool::ResetCmdBuffer(const size_t & idx, const VkCommandBufferResetFlagBits& command_buffer_reset_flag_bits) {
        vkResetCommandBuffer(cmdBuffers->Data[idx], command_buffer_reset_flag_bits);
    }
    
    const VkCommandPool & CommandPool::vkHandle() const noexcept{
        return handle;
    }

    const VkCommandBuffer* CommandPool::GetCommandBuffers(const size_t& offset) const {
        return &cmdBuffers->Data[offset];
    }

    VkCommandBuffer & CommandPool::operator[](const size_t & idx) {
        return cmdBuffers->Data[idx];
    }

    VkCommandBuffer & CommandPool::GetCmdBuffer(const size_t & idx) {
        return cmdBuffers->Data[idx];
    }

    VkCommandBuffer CommandPool::StartSingleCmdBuffer(){
        VkCommandBufferAllocateInfo allocInfo = {};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandPool = handle;
        allocInfo.commandBufferCount = 1;

        VkCommandBuffer commandBuffer;
        vkAllocateCommandBuffers(parent, &allocInfo, &commandBuffer);

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
        return cmdBuffers->Data.size();
    }

    const VkCommandBuffer* CommandPool::Data() const noexcept {
        return cmdBuffers->Data.data();
    }

}
