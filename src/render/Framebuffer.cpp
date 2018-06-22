#include "vpr_stdafx.h"
#include "render/Framebuffer.hpp"
#include "core/LogicalDevice.hpp"
#include "common/vkAssert.hpp"

namespace vpr {
    
    Framebuffer::Framebuffer(const Device * _parent, const VkFramebufferCreateInfo & create_info) : parent(_parent),
        handle(VK_NULL_HANDLE) {
        VkResult result = vkCreateFramebuffer(parent->vkHandle(), &create_info, nullptr, &handle);
        VkAssert(result);
    }

    Framebuffer::~Framebuffer() {
        Destroy();
    }

    Framebuffer::Framebuffer(Framebuffer&& other) noexcept : handle(std::move(other.handle)), parent(std::move(other.parent)) {
        other.handle = VK_NULL_HANDLE;
    }

    Framebuffer& Framebuffer::operator=(Framebuffer&& other) noexcept {
        handle = std::move(other.handle);
        parent = std::move(other.parent);
        other.handle = VK_NULL_HANDLE;
        return *this;
    }
    
    const VkFramebuffer & Framebuffer::vkHandle() const noexcept{
        return handle;
    }

    void Framebuffer::Destroy() {
        if (handle != VK_NULL_HANDLE) {
            vkDestroyFramebuffer(parent->vkHandle(), handle, nullptr);
        }
    }

}
