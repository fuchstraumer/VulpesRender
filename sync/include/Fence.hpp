#pragma once
#ifndef VPR_FENCE_HPP
#define VPR_FENCE_HPP
#include "vpr_stdafx.h"
#include "ForwardDecl.hpp"

namespace vpr {

    /**A fence is the most heavyweight of Vulkan synchronization primitives, and is 
     * for explicitly synchronizating the device and host. Avoid using these unless
     * absolutely necessary, as while it's better than waiting on a queue or device 
     * (requiring full event/pipeline flush), it's still not a cheap operation.
     * \ingroup Synchronization
     */
    class VPR_API Fence {
        Fence(const Fence&) = delete;
        Fence& operator=(const Fence&) = delete;
    public:
        /**Creates a new fence object, using the given flags to change initial state (as that is the only important param in the createInfo)
         * \param flags Set to VK_FENCE_CREATE_SIGNALED_BIT to create the fence in a signaled state - otherwise pass a 0 and it will remain unsignaled
        */
        Fence(const VkDevice& dvc, VkFenceCreateFlags flags);
        Fence(Fence&& other) noexcept;
        Fence& operator=(Fence&& other) noexcept;
        ~Fence();

        const VkFence& vkHandle() const noexcept;

    private:
        VkDevice device{ VK_NULL_HANDLE };
        VkFence handle{ VK_NULL_HANDLE };
    };

}


#endif //!VPR_FENCE_HPP
