#include "vpr_stdafx.h"
#include "core/Instance.hpp"
#include <imgui.h>
#include "core/PhysicalDevice.hpp"
#include "common/VkDebug.hpp"
namespace vulpes {

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

        if (validationEnabled) {
            std::vector<const char*> layers(1);
            layers[0] = "VK_LAYER_LUNARG_standard_validation";
            createInfo.ppEnabledLayerNames = layers.data();
            createInfo.enabledLayerCount = 1;
        }
        else {
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



    Window * Instance::GetWindow() noexcept {
        return window.get();
    }

    void Instance::createDebugCallbacks() noexcept {

        LOG(INFO) << "Validation layers enabled, creating debug callbacks.";
        CreateDebugCallback(vkHandle(), VK_DEBUG_REPORT_WARNING_BIT_EXT, &warningCallback, nullptr);
        CreateDebugCallback(vkHandle(), VK_DEBUG_REPORT_ERROR_BIT_EXT, &errorCallback, nullptr);
        CreateDebugCallback(vkHandle(), VK_DEBUG_REPORT_INFORMATION_BIT_EXT, &infoCallback, nullptr);
        CreateDebugCallback(vkHandle(), VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT, &perfCallback, nullptr);

    }

    void Instance::destroyDebugCallbacks() noexcept {

        DestroyDebugCallback(handle, errorCallback, nullptr);
        DestroyDebugCallback(handle, warningCallback, nullptr);
        DestroyDebugCallback(handle, infoCallback, nullptr);
        DestroyDebugCallback(handle, perfCallback, nullptr);

    }

	void Instance::createWindow(const uint32_t& _width, const uint32_t& _height) {
        
        window = std::make_unique<Window>(this, _width, _height);
        LOG(INFO) << "Created a GLFW window object.";

	}






}
