#pragma once
#ifndef VPR_SEMAPHORE_HPP
#define VPR_SEMAPHORE_HPP
#include "vpr_stdafx.h"
#include "ForwardDecl.hpp"

namespace vpr {

    class Semaphore {
        Semaphore(const Semaphore&) = delete;
        Semaphore& operator=(const Semaphore&) = delete;
    public:
        Semaphore(const Device* dvc);
        ~Semaphore();
        Semaphore(Semaphore&& other) noexcept;
        Semaphore& operator=(Semaphore&& other) noexcept;

        const VkSemaphore& vkHandle() const noexcept;

    private:
        const Device* device;
        VkSemaphore handle;

    };

}

#endif //!VPR_SEMAPHORE_HPP