#include "vpr_stdafx.h"
#include "resource/Queue.hpp"

namespace vulpes {

    Queue::Queue(const VkQueue& _handle, const Device* dvc) : handle(_handle), parent(dvc) {
        queueMutex.lock();
    }

    Queue::~Queue() {
        queueMutex.unlock();
    }

    const VkQueue& Queue::vkHandle() const noexcept {
        return handle;
    }

}