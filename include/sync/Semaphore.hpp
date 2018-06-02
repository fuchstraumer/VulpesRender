#pragma once
#ifndef VPR_SEMAPHORE_HPP
#define VPR_SEMAPHORE_HPP
#include "vpr_stdafx.h"
#include "ForwardDecl.hpp"

namespace vpr {

    /**Vulkan has three primary synchronization primitives, each with distinct use cases and capabilities. The 
     * docs for each class representing these primitive do their best to communicate this, but consult the Vulkan
     * programming guide for further guidance and consider checking examples of these primitives in use for further
     * clarification.
     * \defgroup Synchronization
     */

    /**The semaphore is Vulkan's device-exclusive synchronization primitive. They are used to synchronize command submissions
     * across different queues and queue families, and cannot have their state queried or modified by the device. Semaphores
     * are automatically reset after being used in a batch of command submissions.
     * 
     * VkQueueSubmit() is the only command capable of modifying the internal state of a semaphore object. While usage of these
     * objects is internally atomic for the API, external semaphore references (e.g, handles in the `pWaitSemaphores` and `pSignalSemaphores`
     * array of VkSubmitInfo) must have their usage synchronized (in case one reference ends up in a signal for one submission, and a wait 
     * for another submission, but lack of synchronization submits the wait submission before the signal submission).
     * 
     * \ingroup Synchronization
     */
    class VPR_API Semaphore {
        Semaphore(const Semaphore&) = delete;
        Semaphore& operator=(const Semaphore&) = delete;
    public:
    
        Semaphore(const Device* dvc);
        ~Semaphore();
        Semaphore(Semaphore&& other) noexcept;
        Semaphore& operator=(Semaphore&& other) noexcept;

        const VkSemaphore& vkHandle() const noexcept;

    private:
        const Device* device{ nullptr };
        VkSemaphore handle{ VK_NULL_HANDLE };
    };

}

#endif //!VPR_SEMAPHORE_HPP
