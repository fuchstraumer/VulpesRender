#include "render/SurfaceKHR.hpp"
#include "core/Instance.hpp"
#include "GLFW/glfw3.h"
#if defined(_WIN32) 
#undef APIENTRY
#define GLFW_EXPOSE_NATIVE_WIN32
#define GLFW_EXPOSE_NATIVE_WGL
#include "GLFW/glfw3native.h"
#endif
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