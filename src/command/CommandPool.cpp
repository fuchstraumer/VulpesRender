#include "vpr_stdafx.h"
#include "command/CommandPool.h"
#include "core/Instance.h"
#include "core/LogicalDevice.h"

namespace vulpes {

	CommandPool::CommandPool(const Device * _parent, const VkCommandPoolCreateInfo & create_info, bool _primary) : parent(_parent), createInfo(create_info), primary(_primary) {
		Create();
	}

	CommandPool::CommandPool(const Device * _parent, bool _primary) : parent(_parent), primary(_primary) {}

	void CommandPool::Create() {
		assert(handle == VK_NULL_HANDLE);
		VkResult result = vkCreateCommandPool(parent->vkHandle(), &createInfo, allocators, &handle);
		VkAssert(result);
	}

	void CommandPool::ResetCmdPool() {
		assert(createInfo.flags & VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
		vkResetCommandPool(parent->vkHandle(), handle, VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT);
	}

	CommandPool::CommandPool(CommandPool && other) noexcept{
		handle = std::move(other.handle);
		cmdBuffers = std::move(other.cmdBuffers);
		parent = std::move(other.parent);
		allocators = std::move(other.allocators);
		createInfo = std::move(other.createInfo);
		other.handle = VK_NULL_HANDLE;
	}

	CommandPool & CommandPool::operator=(CommandPool && other) noexcept{
		handle = std::move(other.handle);
		cmdBuffers = std::move(other.cmdBuffers);
		parent = std::move(other.parent);
		allocators = std::move(other.allocators);
		createInfo = std::move(other.createInfo);
		other.handle = VK_NULL_HANDLE;
		return *this;
	}

	CommandPool::~CommandPool(){
		Destroy();
	}

	void CommandPool::Destroy(){
		if (!cmdBuffers.empty()) {
			FreeCommandBuffers();
		}
		if (handle != VK_NULL_HANDLE) {
			vkDestroyCommandPool(parent->vkHandle(), handle, allocators);
			LOG_IF(Instance::VulpesInstanceConfig.VerboseLogging, INFO) << "Command Pool " << handle << " destroyed.";
			handle = VK_NULL_HANDLE;
		}
	}

	void CommandPool::AllocateCmdBuffers(const uint32_t & num_buffers, const VkCommandBufferAllocateInfo& _alloc_info){

		if (!cmdBuffers.empty()) {
			return;
		}

		cmdBuffers.resize(num_buffers);
		auto alloc_info = _alloc_info;
		alloc_info.commandPool = handle;
		alloc_info.commandBufferCount = num_buffers;
		VkResult result = vkAllocateCommandBuffers(parent->vkHandle(), &alloc_info, cmdBuffers.data());
		LOG_IF(Instance::VulpesInstanceConfig.VerboseLogging, INFO) << std::to_string(num_buffers) << " command buffers allocated for command pool " << handle;
		VkAssert(result);
	}

	void CommandPool::FreeCommandBuffers(){
		vkFreeCommandBuffers(parent->vkHandle(), handle, static_cast<uint32_t>(cmdBuffers.size()), cmdBuffers.data());
		LOG_IF(Instance::VulpesInstanceConfig.VerboseLogging, INFO) << std::to_string(cmdBuffers.size()) << " command buffers freed.";
		cmdBuffers.clear();
		cmdBuffers.shrink_to_fit();
	}

	void CommandPool::ResetCmdBuffer(const size_t & idx) {
		assert(createInfo.flags & VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
		vkResetCommandBuffer(cmdBuffers[idx], VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);
	}
	
	const VkCommandPool & CommandPool::vkHandle() const noexcept{
		return handle;
	}

	VkCommandBuffer & vulpes::CommandPool::operator[](const size_t & idx) {
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

}
