#pragma once
#ifndef VPR_SEMAPHORE_HPP
#define VPR_SEMAPHORE_HPP
#include "vpr_stdafx.h"
#include "ForwardDecl.hpp"

namespace vpr {

    class Semaphore {
    public:
        Semaphore(const Device* dvc);
        ~Semaphore();

        const VkSemaphore& vkHandle() const noexcept;

    private:
        const Device* device;
        VkSemaphore handle;

    };

}

#endif //!VPR_SEMAPHORE_HPP