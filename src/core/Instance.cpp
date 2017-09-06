#include "vpr_stdafx.h"
#include "core/Instance.hpp"
#include <imgui.h>
#include "core/PhysicalDevice.hpp"

namespace vulpes {

	cfg::vulpesInstanceInfo Instance::VulpesInstanceConfig = cfg::vulpesInstanceInfo();

	void Instance::setupPhysicalDevices(){
		physicalDeviceFactory = std::make_unique<PhysicalDeviceFactory>();
		physicalDevice = std::make_unique<PhysicalDevice>(physicalDeviceFactory->GetBestDevice(handle));
	}

	Instance::~Instance(){
		vkDestroySurfaceKHR(handle, surface, AllocationCallbacks);
		if (validationEnabled) {
			DestroyDebugCallback(handle, errorCallback, AllocationCallbacks);
			DestroyDebugCallback(handle, warningCallback, AllocationCallbacks);
			DestroyDebugCallback(handle, infoCallback, AllocationCallbacks);
			DestroyDebugCallback(handle, perfCallback, AllocationCallbacks);
		}
		vkDestroyInstance(handle, AllocationCallbacks);
	}

	const VkInstance& Instance::vkHandle() const{
		return handle;
	}

	const VkSurfaceKHR Instance::GetSurface() const {
		return window->vkSurface();
	}

	const VkPhysicalDevice & Instance::GetPhysicalDevice() const noexcept{
		return physicalDevice->vkHandle();
	}

	InstanceGLFW::Instance(VkInstanceCreateInfo create_info, const bool & enable_validation, const uint32_t& _width, const uint32_t& _height) {

		createInfo = create_info;
		validationEnabled = enable_validation;

		createWindow(width, height);

		createInfo.ppEnabledExtensionNames = window->Extensions.data();
		createInfo.enabledExtensionCount = static_cast<uint32_t>(window->Extensions.size());
		VkResult err = vkCreateInstance(&createInfo, AllocationCallbacks, &handle);
		VkAssert(err);

		if (validationEnabled) {
			CreateDebugCallback(*this, VK_DEBUG_REPORT_WARNING_BIT_EXT, &warningCallback, AllocationCallbacks);
			CreateDebugCallback(*this, VK_DEBUG_REPORT_ERROR_BIT_EXT, &errorCallback, AllocationCallbacks);
			CreateDebugCallback(*this, VK_DEBUG_REPORT_INFORMATION_BIT_EXT, &infoCallback, AllocationCallbacks);
			CreateDebugCallback(*this, VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT, &perfCallback, AllocationCallbacks);
		}
		
	}

	void Instance::createWindow(const uint32_t& width, const uint32_t& height) {
        
        Window = std::make_unique<Window>(this, width, height);
        LOG(INFO) << "Created a GLFW window object.";

	}






}
