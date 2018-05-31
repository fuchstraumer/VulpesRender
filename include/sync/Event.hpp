#pragma once
#ifndef VPR_EVENT_HPP
#define VPR_EVENT_HPP
#include "ForwardDecl.hpp"
#include "vpr_stdafx.h"

namespace vpr {

    /**Events are a unique synchronization primitive on Vulkan: unlike fences or semaphores they are not strictly 
     * intended for only use on the device or only use on the host. Events can have their status retrieved and set
     * on the host, but cannot be waited on by the host. However, they can be waited on by the device - but the device
     * cannot retrieve their status (it can however, set or reset the event).
     * 
     * vkCmdWaitEvent also takes six more parameters than what is presented here - representing various memory barriers
     * to execute once the event clears. These would've added clutter to the function signature though, so that is left
     * unimplemented. Instead, the pure wait-only interpretation of that function is presented through this class interface.
     * 
     * Events are best used to synchronize and regulate events occuring at different stages of the same pipeline, unlike
     * semaphores which work well across queues and command submissions (though they could be used like this, as well, I believe).
     * 
     * \ingroup Synchronization
     */
    class Event {
        Event(const Event&) = delete;
        Event& operator=(const Event&) = delete;
    public:

        Event(const Device* dvc);
        ~Event();
        Event(Event&& other) noexcept;
        Event& operator=(Event&& other) noexcept;

        const VkEvent& vkHandle() const noexcept;

        /**Sets the event to signaled/"set" on the *host*.*/
        void Set() noexcept;
        /**Resets the event unsignaled/"reset" on the *host* */
        void Reset() noexcept;
        /**Retrieves current status of the event - either VK_EVENT_SET or VK_EVENT_RESET*/
        VkResult GetStatus() const noexcept;
        
        bool IsSet() const noexcept;
        bool IsReset() const noexcept;

        /**Sets this event to signaled at the given stage of the pipeline, upon execution of the passed command buffer.*/
        void Set(const VkCommandBuffer& cmd, const VkPipelineStageFlags stage_to_signal_at);
        /**Resets this event at the given pipeline stage upon execution of the passed command buffer.*/
        void Reset(const VkCommandBuffer& cmd, const VkPipelineStageFlags stage_to_reset_at);
        /**Waits for event when executing given command buffer.
         * \param potential_signal_stages: stages the event can potentially be set to "signalled" in.
         * \param stages_to_wait_at: stages that will wait for this event before executing
        */
        void Wait(const VkCommandBuffer& cmd, const VkPipelineStageFlags potential_signal_stages, 
            const VkPipelineStageFlags stages_to_wait_at);

    private:

        const Device* device;
        VkEvent handle;
    };

}

#endif //!VPR_EVENT_HPP
