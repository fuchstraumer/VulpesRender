#include "Sampler.hpp"
#include "vkAssert.hpp"
#include "CreateInfoBase.hpp"

namespace vpr {

    Sampler::Sampler(const VkDevice& dvc, const VkSamplerCreateInfo& info) : device(dvc), handle(VK_NULL_HANDLE) {
        VkResult result = vkCreateSampler(device, &info, nullptr, &handle);
        VkAssert(result);
    }

    Sampler::~Sampler() {
        if (handle != VK_NULL_HANDLE) {
            vkDestroySampler(device, handle, nullptr);
        }
    }

    Sampler::Sampler(Sampler&& other) noexcept : device(std::move(other.device)),
        handle(std::move(other.handle)) { other.handle = VK_NULL_HANDLE; }

    Sampler& Sampler::operator=(Sampler&& other) noexcept {
        device = std::move(other.device);
        handle = std::move(other.handle);
        other.handle = VK_NULL_HANDLE;
        return *this;
    }

    const VkSampler& Sampler::vkHandle() const noexcept {
        return handle;
    }
}