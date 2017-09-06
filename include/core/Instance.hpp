#pragma once
#ifndef VULPES_VK_INSTANCE_H
#define VULPES_VK_INSTANCE_H
#include "vpr_stdafx.h"
#include "../ForwardDecl.hpp"
#include "InstanceConfig.hpp"
#include "Window.hpp"

namespace vulpes {

	struct PhysicalDeviceFactory;

	class Instance {
		Instance(const Instance&) = delete;
		Instance& operator=(const Instance&) = delete;
	public:
		
		Instance(VkInstanceCreateInfo create_info, const bool& enable_validation, const uint32_t& width = DEFAULT_WIDTH, const uint32_t& height = DEFAULT_HEIGHT);
        ~Instance();

		const VkInstance& vkHandle() const;
        const VkSurfaceKHR vkSurface() const;
        
        const VkPhysicalDevice& PhysicalDevice() const noexcept;

        static cfg::vulpesInstanceInfo VulpesInstanceConfig;

    private:

        void setupPhysicalDevices();
        void createWindow(const uint32_t& width, const uint32_t& height);
    
		std::unique_ptr<PhysicalDevice> physicalDeviceFactory;
		std::unique_ptr<PhysicalDevice> physicalDevice;        
		VkDebugReportCallbackEXT errorCallback, warningCallback, perfCallback, infoCallback, vkCallback;
        std::vector<std::string> layers;
        std::unique_ptr<Window> window;
		VkInstance handle;
		uint32_t width, height;
        VkInstanceCreateInfo createInfo;
        
	};

}

#endif // !INSTANCE_H
