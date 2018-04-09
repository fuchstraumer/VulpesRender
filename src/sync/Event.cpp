#include "sync/Event.hpp"
#include "core/LogicalDevice.hpp"

namespace vpr {

    Event::Event(const Device* dvc) : device(dvc), handle(VK_NULL_HANDLE) {
        VkResult result = vkCreateEvent(device->vkHandle(), &vk_event_create_info_base, nullptr, &handle);
        VkAssert(result);
    }

    Event::~Event() {
        if (handle != VK_NULL_HANDLE) {
            vkDestroyEvent(device->vkHandle(), handle, nullptr);
        }
    }

    Event::Event(Event&& other) noexcept : device(std::move(other.device)), handle(std::move(other.handle)) {
        other.handle = VK_NULL_HANDLE;
    }

    Event& Event::operator=(Event&& other) noexcept {
        device = std::move(other.device);
        handle = std::move(other.handle);
        other.handle = VK_NULL_HANDLE;
        return *this;
    }

    void Event::Set() noexcept {
        VkResult result = vkSetEvent(device->vkHandle(), handle);
        VkAssert(result);
    }

    void Event::Reset() noexcept {
        VkResult result = vkResetEvent(device->vkHandle(), handle);
        VkAssert(result);
    }

    VkResult Event::GetStatus() const noexcept {
        return vkGetEventStatus(device->vkHandle(), handle);
    }

    bool Event::IsSet() const noexcept {
        VkResult result = GetStatus();
        return result == VK_EVENT_SET;
    }

    bool Event::IsReset() const noexcept {
        VkResult result = GetStatus();
        return result == VK_EVENT_RESET;
    }

    void Event::Set(const VkCommandBuffer& cmd, const VkPipelineStageFlags stages_to_set_at) {
        vkCmdSetEvent(cmd, handle, stages_to_set_at);
    }

    void Event::Reset(const VkCommandBuffer& cmd, const VkPipelineStageFlags stages_to_reset_at) {
        vkCmdResetEvent(cmd, handle, stages_to_reset_at);
    }

    void Event::Wait(const VkCommandBuffer& cmd, const VkPipelineStageFlags potential_signal_stages, const VkPipelineStageFlags stages_to_wait_at) {
        vkCmdWaitEvents(cmd, 1, &handle, potential_signal_stages, stages_to_wait_at, 0, nullptr, 0, nullptr, 0, nullptr);
    }
    
}