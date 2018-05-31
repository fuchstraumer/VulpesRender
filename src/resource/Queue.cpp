#include "vpr_stdafx.h"
#include "resource/Queue.hpp"

namespace vpr {

    Queue::Queue(const VkQueue& _handle) : handle(_handle) {
        queueMutex.lock();
    }

    Queue::~Queue() {
        queueMutex.unlock();
    }

    const VkQueue& Queue::vkHandle() const noexcept {
        return handle;
    }

}
