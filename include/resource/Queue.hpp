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

    private:

        std::mutex queueMutex;
        const VkQueue& handle;
        const Device* parent;

    };

}

#endif // !VULPESRENDER_QUEUE_HPP
