#pragma once
#ifndef VULPES_VK_SURFACE_KHR_HPP
#define VULPES_VK_SURFACE_KHR_HPP
#include "vpr_stdafx.h"
#include "ForwardDecl.hpp"

namespace vpr {

    /**
     * The bare-minimum required to wrap a VkSurfaceKHR object. Uses the glfwCreateWindowSurface function
     * to handle the various platform-specific details that would change otherwise.
     * 
     * \ingroup Core
     */
    class VPR_API SurfaceKHR {
        SurfaceKHR(const SurfaceKHR&) = delete;
        SurfaceKHR& operator=(const SurfaceKHR&) = delete;
    public:

        SurfaceKHR(const Instance* _parent, VkPhysicalDevice physical_device, GLFWwindow* window);
        SurfaceKHR(SurfaceKHR&& other) noexcept;
        SurfaceKHR& operator=(SurfaceKHR&& other) noexcept;
        ~SurfaceKHR();

        void Recreate();
        const VkSurfaceKHR& vkHandle() const noexcept;

        static VkBool32 VerifyPresentationSupport(VkPhysicalDevice physical_device, VkSurfaceKHR surface);

    private:

        void create();
        void destroy();

        GLFWwindow* window;
        const Instance* parent;
        VkPhysicalDevice device;
        VkSurfaceKHR handle;
    };


}

#endif //!VULPES_VK_SURFACE_KHR_HPP
