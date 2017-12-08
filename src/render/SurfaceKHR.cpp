#include "render/SurfaceKHR.hpp"
#include "core/Instance.hpp"

namespace vpr {

    SurfaceKHR::SurfaceKHR(const Instance* _parent, GLFWwindow* window) : parent(_parent) {
        VkResult err = glfwCreateWindowSurface(parent->vkHandle(), window, nullptr, &handle);
        VkAssert(err);
    }

    SurfaceKHR::~SurfaceKHR() {
        vkDestroySurfaceKHR(parent->vkHandle(), handle, nullptr);
    }

    const VkSurfaceKHR& SurfaceKHR::vkHandle() const noexcept {
        return handle;
    }


}