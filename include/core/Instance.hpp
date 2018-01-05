#pragma once
#ifndef VULPES_VK_INSTANCE_H
#define VULPES_VK_INSTANCE_H

#include "vpr_stdafx.h"
#include "ForwardDecl.hpp"

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
    class Instance  {
        Instance(const Instance &) = delete;
        Instance& operator=(const Instance &) = delete;
    public:
        
        Instance(const VkApplicationInfo* info, GLFWwindow* window, const uint32_t& width, const uint32_t& height);
        Instance(const VkApplicationInfo* info, const char** extensions, const size_t extension_count, GLFWwindow* window, const uint32_t width, const uint32_t height);
        ~Instance();

        const VkInstance& vkHandle() const noexcept;
        const VkSurfaceKHR& vkSurface() const noexcept;
        const PhysicalDevice* GetPhysicalDevice() const noexcept;
        GLFWwindow* GetGLFWwindow() const noexcept;

        void RecreateSurface();

    private:

        void setupPhysicalDevice();
        void createSurfaceKHR();
        mutable GLFWwindow* window;
        std::unique_ptr<PhysicalDevice> physicalDevice;
        std::unique_ptr<SurfaceKHR> surface;     
        VkInstance handle;
        VkInstanceCreateInfo createInfo;
        bool validationEnabled{ false };
    };

}

#endif // !INSTANCE_H
