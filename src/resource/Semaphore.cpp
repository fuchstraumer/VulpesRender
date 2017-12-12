#include "resource/Semaphore.hpp"
#include "core/LogicalDevice.hpp"
namespace vpr {

    Semaphore::Semaphore(const Device* dvc) : device(dvc) {
        vkCreateSemaphore(device->vkHandle(), &vk_semaphore_create_info_base, nullptr, &handle);
    }

    Semaphore::~Semaphore() {
        vkDestroySemaphore(device->vkHandle(), handle, nullptr);
    }

    const VkSemaphore& Semaphore::vkHandle() const noexcept {
        return handle;
    }

}