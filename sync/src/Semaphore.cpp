#include "Semaphore.hpp"
#include "vkAssert.hpp"
#include "CreateInfoBase.hpp"

namespace vpr
{

    Semaphore::Semaphore(const VkDevice& dvc) : device(dvc)
    {
        vkCreateSemaphore(device, &vk_semaphore_create_info_base, nullptr, &handle);
    }

    Semaphore::~Semaphore()
    {
        if (handle != VK_NULL_HANDLE)
        {
            vkDestroySemaphore(device, handle, nullptr);
        }
    }

    Semaphore::Semaphore(Semaphore&& other) noexcept : handle(std::move(other.handle)), device(std::move(other.device))
    {
        other.handle = VK_NULL_HANDLE;
    }

    Semaphore& Semaphore::operator=(Semaphore&& other) noexcept
    {
        handle = std::move(other.handle);
        other.handle = VK_NULL_HANDLE;
        device = std::move(other.device);
        return *this;
    }

    const VkSemaphore& Semaphore::vkHandle() const noexcept {
        return handle;
    }

}
