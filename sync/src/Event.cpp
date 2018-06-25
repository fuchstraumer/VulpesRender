#include "Event.hpp"
#include "vkAssert.hpp"
#include "CreateInfoBase.hpp"

namespace vpr {

    Event::Event(const VkDevice& dvc) : device(dvc), handle(VK_NULL_HANDLE) {
        VkResult result = vkCreateEvent(device, &vk_event_create_info_base, nullptr, &handle);
        VkAssert(result);
    }

    Event::~Event() {
        if (handle != VK_NULL_HANDLE) {
            vkDestroyEvent(device, handle, nullptr);
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
        VkResult result = vkSetEvent(device, handle);
        VkAssert(result);
    }

    void Event::Reset() noexcept {
        VkResult result = vkResetEvent(device, handle);
        VkAssert(result);
    }

    VkResult Event::GetStatus() const noexcept {
        return vkGetEventStatus(device, handle);
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
