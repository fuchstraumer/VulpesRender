#pragma once
#ifndef VULPES_VK_INSTANCE_H
#define VULPES_VK_INSTANCE_H
#include "vpr_stdafx.h"
#include "../ForwardDecl.hpp"
#include "InstanceConfig.hpp"
#include "Window.hpp"

namespace vulpes {

	class Instance {
		Instance(const Instance&) = delete;
		Instance& operator=(const Instance&) = delete;
	public:
		
		Instance(VkInstanceCreateInfo create_info, const bool& enable_validation, const uint32_t& width = DEFAULT_WIDTH, const uint32_t& height = DEFAULT_HEIGHT);
        ~Instance();

		const VkInstance& vkHandle() const noexcept;
        const VkSurfaceKHR vkSurface() const noexcept;
        const PhysicalDevice* GetPhysicalDevice() const noexcept;
        const Window* GetWindow() const noexcept;
        Window* GetWindow() noexcept;
        static cfg::vulpesInstanceInfo VulpesInstanceConfig;

    private:

        void setupPhysicalDevice();
        void createWindow(const uint32_t& width, const uint32_t& height);
        void createDebugCallbacks() noexcept;
        void destroyDebugCallbacks() noexcept;

		std::unique_ptr<PhysicalDevice> physicalDevice;        
		VkDebugReportCallbackEXT errorCallback, warningCallback, perfCallback, infoCallback, vkCallback;
        std::vector<std::string> layers;
        std::unique_ptr<Window> window;
		VkInstance handle;
		uint32_t width, height;
        VkInstanceCreateInfo createInfo;
        bool validationEnabled{ false };
	};

}

#endif // !INSTANCE_H
