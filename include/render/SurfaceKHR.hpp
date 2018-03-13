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
     * Note that this class only works for non-fullscreen windows.
     * \ingroup Rendering
     */
    class VPR_API SurfaceKHR {
    public:

        SurfaceKHR(const Instance* _parent, GLFWwindow* window);
        ~SurfaceKHR();
        const VkSurfaceKHR& vkHandle() const noexcept;

    private:
        const Instance* parent;
        VkSurfaceKHR handle;
    };

}

#endif //!VULPES_VK_SURFACE_KHR_HPP