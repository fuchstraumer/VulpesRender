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

        /**Initializes a new surface KHR for the given instance given the parameters. Hosted by the specified physical_device.
         * \param window Is either a GLFWwindow pointer, or an ANativeWindow pointer. 
         */
        SurfaceKHR(const Instance* _parent, VkPhysicalDevice physical_device, void* window);
        SurfaceKHR(SurfaceKHR&& other) noexcept;
        SurfaceKHR& operator=(SurfaceKHR&& other) noexcept;
        ~SurfaceKHR();
        /**Destroys then properly re-creates this surface. Required as part of a swapchain recreation event.*/
        void Recreate();

        const VkSurfaceKHR& vkHandle() const noexcept;

        /**This function must be called once before presenting to a surface using a physical device, or the validation layers will 
         * throw warnings. */
        static VkBool32 VerifyPresentationSupport(VkPhysicalDevice physical_device, VkSurfaceKHR surface);

    private:

        void create();
        void destroy();
#ifndef __ANDROID__
        GLFWwindow* window{ nullptr };
#else
        struct ANativeWindow* window{ nullptr };
#endif 
        const Instance* parent{ nullptr };
        VkPhysicalDevice device{ VK_NULL_HANDLE };
        VkSurfaceKHR handle{ VK_NULL_HANDLE };
    };


}

#endif //!VULPES_VK_SURFACE_KHR_HPP
