#include "vpr_stdafx.h"
#include "core/Instance.hpp"
#include "core/PhysicalDevice.hpp"
#include "util/easylogging++.h"
#include <imgui.h>

namespace vpr {

    vulpes_graphics_options_t Instance::GraphicsSettings = vulpes_graphics_options_t();
    vulpes_state_t Instance::VulpesState = vulpes_state_t();
    
    Instance::Instance(VkInstanceCreateInfo create_info, const bool & enable_validation, const uint32_t& _width, const uint32_t& _height) {

        createInfo = create_info;
        validationEnabled = enable_validation;

        createWindow(_width, _height);

        createInfo.ppEnabledExtensionNames = window->Extensions().data();
        if (!window->Extensions().empty()) {
            LOG(INFO) << "Enabling following window-based extensions:";
            for (const auto& ext : window->Extensions()) {
                LOG(INFO) << ext;
            }
        }
        createInfo.enabledExtensionCount = static_cast<uint32_t>(window->Extensions().size());

        // Note: this will be fixed in a future version.
        {
            createInfo.ppEnabledLayerNames = nullptr;
            createInfo.enabledLayerCount = 0;
        }

        VkResult err = vkCreateInstance(&createInfo, nullptr, &handle);
        VkAssert(err);

        if (validationEnabled) {
            createDebugCallbacks();
        }

        setupPhysicalDevice();
        window->CreateSurface();

    }

    void Instance::setupPhysicalDevice(){
        physicalDevice = std::make_unique<PhysicalDevice>(vkHandle());
    }

    Instance::~Instance(){
        window.reset();
        if (validationEnabled) {
            destroyDebugCallbacks();
        }
        vkDestroyInstance(handle, nullptr);
    }

    const VkInstance& Instance::vkHandle() const noexcept {
        return handle;
    }

    const VkSurfaceKHR Instance::vkSurface() const noexcept {
        return window->vkSurface();
    }

    const PhysicalDevice* Instance::GetPhysicalDevice() const noexcept{
        return physicalDevice.get();
    }

    const Window* Instance::GetWindow() const noexcept {
        return window.get();
    }

    Window* Instance::GetWindow() noexcept {
        return window.get();
    }

    void Instance::createDebugCallbacks() noexcept {
        LOG(WARNING) << "Validation layers enabled, but aren't currently supported. Use the relevant environment variables if you still wish to use them.";
    }

    void Instance::destroyDebugCallbacks() noexcept {

    }

    void Instance::createWindow(const uint32_t& _width, const uint32_t& _height) {
        
        window = std::make_unique<Window>(this, _width, _height);
        LOG(INFO) << "Created a GLFW window object.";

    }

}
