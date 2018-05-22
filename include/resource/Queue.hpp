#pragma once
#ifndef VULPESRENDER_QUEUE_HPP
#define VULPESRENDER_QUEUE_HPP
#include "vpr_stdafx.h"
#include "ForwardDecl.hpp"
#include <mutex>
namespace vpr {
    
    /**This is NOT an RAII wrapper around a VkQueue handle. it is intended for use like lock_guard, where the resource
     * is protected from use across multiple threads so long as this object exists. The contained mutex is locked
     * upon construction, and unlocked on destruction. Also note that there is no need for a parent device pointer,
     * and the Queue handle is passed and stored as a const reference
     * \ingroup Resources
     */
    class VPR_API Queue {
    public:

        Queue(const VkQueue& handle);
        ~Queue();
        const VkQueue& vkHandle() const noexcept;

    private:
        std::mutex queueMutex;
        const VkQueue& handle;
    };

}

#endif // !VULPESRENDER_QUEUE_HPP
