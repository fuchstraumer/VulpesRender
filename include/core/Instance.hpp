#pragma once
#ifndef VULPES_VK_INSTANCE_H
#define VULPES_VK_INSTANCE_H

#include "vpr_stdafx.h"
#include "ForwardDecl.hpp"
#include "Window.hpp"
#include "common/GraphicsSettings.hpp"

namespace vpr {

    /** The Core group handles the base Vulkan resources and objects: LogicalDevice, PhysicalDevice, Instance, and Window. It also 
    *   includes the InputHandler class, which is responsible for handling input events and updats from the Window class.
    * \defgroup Core
    */

    /** Instance is a wrapper around the base Vulkan object that must be initialized first. The VkInstanceCreateInfo struct passed to the constructor
    *    contains information about the current layers enabled, and which Vulkan Instance extensions to enable. By default, this should/will contain
    *    extensions required to support the creation of a window surface (VkSurfaceKHR).
    *
    *    This class also contains a Window object, which it is responsible for creating and destroying as necessary, along with a PhysicalDevice object.
    *    These can both be retrieved through the relevant methods.
    *    \ingroup Core
    */
    class Instance {
        Instance(const Instance&) = delete;
        Instance& operator=(const Instance&) = delete;
    public:
        
        Instance(VkInstanceCreateInfo create_info, const bool& enable_validation, const uint32_t& width, const uint32_t& height);
        ~Instance();

        const VkInstance& vkHandle() const noexcept;
        const VkSurfaceKHR vkSurface() const noexcept;
        const PhysicalDevice* GetPhysicalDevice() const noexcept;
        const Window* GetWindow() const noexcept;
        Window* GetWindow() noexcept;

        static vulpes_graphics_options_t GraphicsSettings;
        static vulpes_state_t VulpesState;
    private:

        void setupPhysicalDevice();
        void createWindow(const uint32_t& width, const uint32_t& height);
        void createDebugCallbacks() noexcept;
        void destroyDebugCallbacks() noexcept;

        std::unique_ptr<PhysicalDevice> physicalDevice;        
        VkDebugReportCallbackEXT errorCallback;
        VkDebugReportCallbackEXT warningCallback;
        VkDebugReportCallbackEXT perfCallback;
        VkDebugReportCallbackEXT infoCallback; 
        VkDebugReportCallbackEXT vkCallback;
        std::unique_ptr<Window> window;
        VkInstance handle;
        VkInstanceCreateInfo createInfo;
        bool validationEnabled{ false };
    };

}

#endif // !INSTANCE_H
