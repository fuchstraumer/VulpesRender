#include "SurfaceKHR.hpp"
#include "Instance.hpp"
#define GLFW_INCLUDE_VULKAN
#include "GLFW/glfw3.h"
#include "vkAssert.hpp"
#include <utility>

namespace vpr {

    SurfaceKHR::SurfaceKHR(const Instance* _parent, VkPhysicalDevice _device, GLFWwindow* _window) : parent(_parent), window(_window), device(_device), handle(VK_NULL_HANDLE) {
        create();
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
        destroy();
    }

    void SurfaceKHR::Recreate() {
        destroy();
        create();
    }

    const VkSurfaceKHR& SurfaceKHR::vkHandle() const noexcept {
        return handle;
    }

    void SurfaceKHR::create() {
        VkResult err = glfwCreateWindowSurface(parent->vkHandle(), window, nullptr, &handle);
        VkAssert(err);
        if (!VerifyPresentationSupport(device, handle)) {
            throw std::runtime_error("Surfaces are not supported on current physical device!");
        }
    }

    void SurfaceKHR::destroy() {
        if (handle != VK_NULL_HANDLE) {
            vkDestroySurfaceKHR(parent->vkHandle(), handle, nullptr);
            handle = VK_NULL_HANDLE;
        }
    }

    VkBool32 SurfaceKHR::VerifyPresentationSupport(VkPhysicalDevice physical_device, VkSurfaceKHR surface) {
        // Check presentation support
        VkBool32 present_support = VK_FALSE;
        for (uint32_t i = 0; i < 3; ++i) {
            vkGetPhysicalDeviceSurfaceSupportKHR(physical_device, i, surface, &present_support);
            if (present_support) {
                break;
            }
        }
        return present_support;
    }

}
