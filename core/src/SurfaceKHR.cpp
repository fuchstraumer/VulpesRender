#include "SurfaceKHR.hpp"
#include "Instance.hpp"
#define GLFW_INCLUDE_VULKAN
#include "GLFW/glfw3.h"
#include "vkAssert.hpp"
#include <utility>

namespace vpr {

    SurfaceKHR::SurfaceKHR(const Instance* _parent, GLFWwindow* window) : parent(_parent), handle(VK_NULL_HANDLE) {
        VkResult err = glfwCreateWindowSurface(parent->vkHandle(), window, nullptr, &handle);
        VkAssert(err);
    }

    SurfaceKHR::SurfaceKHR(SurfaceKHR&& other) noexcept : parent(std::move(other.parent)), handle(std::move(other.handle)) {
        other.handle = VK_NULL_HANDLE;
    }

    SurfaceKHR& SurfaceKHR::operator=(SurfaceKHR&& other) noexcept {
        parent = std::move(other.parent);
        handle = std::move(other.handle);
        other.handle = VK_NULL_HANDLE;
        return *this;
    }

    SurfaceKHR::~SurfaceKHR() {
        if (handle != VK_NULL_HANDLE) {
            vkDestroySurfaceKHR(parent->vkHandle(), handle, nullptr);
            handle = VK_NULL_HANDLE;
        }
    }

    const VkSurfaceKHR& SurfaceKHR::vkHandle() const noexcept {
        return handle;
    }

}
