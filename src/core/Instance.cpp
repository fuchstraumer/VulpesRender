#include "vpr_stdafx.h"
#include "core/Instance.hpp"
#include "core/PhysicalDevice.hpp"
#include "render/SurfaceKHR.hpp"
#include "util/easylogging++.h"
#include "GLFW/glfw3.h"
#if defined(_WIN32) 
#undef APIENTRY
#define GLFW_EXPOSE_NATIVE_WIN32
#define GLFW_EXPOSE_NATIVE_WGL
#include "GLFW/glfw3native.h"
#endif
namespace vpr {
    
    Instance::Instance(const VkApplicationInfo*info, GLFWwindow* _window, const uint32_t& _width, const uint32_t& _height) : window(_window) {

        createInfo = VkInstanceCreateInfo{ VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO, nullptr, 0,
            info, 0, nullptr
        };
        uint32_t ext_cnt = 0;
        const char** ext_names;
        ext_names = glfwGetRequiredInstanceExtensions(&ext_cnt);

        createInfo.ppEnabledExtensionNames = ext_names;
        createInfo.enabledExtensionCount = ext_cnt;

        // Note: this will be fixed in a future version.
        {
            createInfo.ppEnabledLayerNames = nullptr;
            createInfo.enabledLayerCount = 0;
        }

        VkResult err = vkCreateInstance(&createInfo, nullptr, &handle);
        VkAssert(err);

        setupPhysicalDevice();
        createSurfaceKHR();

    }

    Instance::Instance(const VkApplicationInfo * info, const char ** extensions, const size_t extension_count, GLFWwindow * window, const uint32_t width, const uint32_t height) {
    }

    void Instance::setupPhysicalDevice(){
        physicalDevice = std::make_unique<PhysicalDevice>(vkHandle());
    }

    void Instance::RecreateSurface() {
        surface.reset();
        createSurfaceKHR();
    }

    void Instance::createSurfaceKHR() {
        surface = std::make_unique<SurfaceKHR>(this, window);
    }

    Instance::~Instance(){
        surface.reset();
        vkDestroyInstance(handle, nullptr);
    }

    const VkInstance& Instance::vkHandle() const noexcept {
        return handle;
    }

    const VkSurfaceKHR& Instance::vkSurface() const noexcept {
        return surface->vkHandle();
    }

    const PhysicalDevice* Instance::GetPhysicalDevice() const noexcept{
        return physicalDevice.get();
    }

    GLFWwindow * Instance::GetGLFWwindow() const noexcept {
        return window;
    }

}
