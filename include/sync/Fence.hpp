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
    class Fence {
        Fence(const Fence&) = delete;
        Fence& operator=(const Fence&) = delete;
    public:

        Fence(const Device* dvc, VkFenceCreateFlags flags);
        Fence(Fence&& other) noexcept;
        Fence& operator=(Fence&& other) noexcept;
        ~Fence();

        const VkFence& vkHandle() const noexcept;

    private:
        const Device* device{ nullptr };
        VkFence handle{ VK_NULL_HANDLE };
    };

}


#endif //!VPR_FENCE_HPP
