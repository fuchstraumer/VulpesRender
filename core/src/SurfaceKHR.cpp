#include "SurfaceKHR.hpp"
#include "Instance.hpp"
#ifndef __ANDROID__
#ifdef VPR_USE_SDL
#include <SDL2/SDL_vulkan.h>
#else
#define GLFW_INCLUDE_VULKAN
#include "GLFW/glfw3.h"
#endif
#else 
#include <vulkan/vulkan_android.h>
#endif
#include "vkAssert.hpp"
#include <utility>

namespace vpr {

#ifndef __ANDROID__
#ifdef VPR_USE_SDL
    using platform_window_type = SDL_Window;
#else
    using platform_window_type = GLFWwindow;
#endif
#else
    using platform_window_type = ANativeWindow;
#endif

    SurfaceKHR::SurfaceKHR(const Instance* _parent, VkPhysicalDevice _device, void* _window) : parent(_parent), window(reinterpret_cast<platform_window_type*>(_window)), device(_device), handle(VK_NULL_HANDLE) {
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

#ifndef __ANDROID__
    void SurfaceKHR::create() {
#ifdef VPR_USE_SDL
        assert(SDL_Vulkan_CreateSurface(window, parent->vkHandle(), &handle));
#else
        VkResult err = glfwCreateWindowSurface(parent->vkHandle(), window, nullptr, &handle);
        VkAssert(err);
#endif
        if (!VerifyPresentationSupport(device, handle)) {
            throw std::runtime_error("Surfaces are not supported on current physical device!");
        }
    }
#else
    void SurfaceKHR::create() {
        VkAndroidSurfaceCreateInfoKHR surface_create_info{
            VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR,
            nullptr,
            0,
            window
        };
        VkResult err = vkCreateAndroidSurfaceKHR(parent->vkHandle(), &surface_create_info, nullptr, &handle);
        VkAssert(err);
        if (!VerifyPresentationSupport(device, handle)) {
            throw std::runtime_error("Surfaces are not support on the current physical device!");
        }
    }
#endif

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
