#pragma once
#ifndef VULPESRENDER_QUEUE_HPP
#define VULPESRENDER_QUEUE_HPP

#include "vpr_stdafx.h"
#include "ForwardDecl.hpp"

namespace vulpes {
    
    class Queue {
    public:

        Queue(const VkQueue& handle, const Device* dvc);
        ~Queue();

        const VkQueue& vkHandle() const noexcept;
        void AddSubmission(const VkSubmitInfo& submit_info);
        void Submit(const uint64_t& fence_timeout = vk_default_fence_timeout);

    private:

        VkFence submitFence;
        std::mutex queueMutex;
        VkQueue& handle;
        const Device* parent;
        std::vector<VkSubmitInfo> submissions;

    };

}

#endif // !VULPESRENDER_QUEUE_HPP
