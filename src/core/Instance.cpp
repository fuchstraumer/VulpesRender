#include "vpr_stdafx.h"
#include "core/Instance.hpp"
#include "core/PhysicalDevice.hpp"
#include "render/SurfaceKHR.hpp"
#include "util/easylogging++.h"
#include <imgui.h>

namespace vpr {

    vulpes_graphics_options_t Instance::GraphicsSettings = vulpes_graphics_options_t();
    vulpes_state_t Instance::VulpesState = vulpes_state_t();
    
    Instance::Instance(VkInstanceCreateInfo create_info, GLFWwindow* _window, const uint32_t& _width, const uint32_t& _height) : window(_window) {

        createInfo = create_info;
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

    void Instance::setupPhysicalDevice(){
        physicalDevice = std::make_unique<PhysicalDevice>(vkHandle());
    }

    void Instance::createSurfaceKHR() {
        surface = std::make_unique<SurfaceKHR>(this, window);
    }

    Instance::~Instance(){
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
