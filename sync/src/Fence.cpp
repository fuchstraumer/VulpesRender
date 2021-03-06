#include "Fence.hpp"
#include "vkAssert.hpp"
#include "CreateInfoBase.hpp"

namespace vpr
{

    Fence::Fence(const VkDevice& dvc, VkFenceCreateFlags flags) : device(dvc)
    {
        VkFenceCreateInfo info = vk_fence_create_info_base;
        info.flags = flags;
        VkResult result = vkCreateFence(device, &info, nullptr, &handle);
        VkAssert(result);
    }

    Fence::Fence(Fence&& other) noexcept : device(std::move(other.device)), handle(std::move(other.handle)) {
        other.handle = VK_NULL_HANDLE;
    }

    Fence& Fence::operator=(Fence&& other) noexcept {
        device = std::move(other.device);
        handle = std::move(other.handle);
        other.handle = VK_NULL_HANDLE;
        return *this;
    }

    Fence::~Fence() {
        if (handle != VK_NULL_HANDLE) {
            vkDestroyFence(device, handle, nullptr);
        }
    }

    const VkFence& Fence::vkHandle() const noexcept {
        return handle;
    }

}
