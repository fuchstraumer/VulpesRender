#ifndef VULPES_VK_WINDOW_H
#define VULPES_VK_WINDOW_H
#include "vpr_stdafx.h"

#if defined(__linux__)
#include <wayland-client.h>
#elif defined(_WIN32) 
#define GLFW_INCLUDE_VULKAN
#include "glfw/glfw3.h"
#undef APIENTRY
#define GLFW_EXPOSE_NATIVE_WIN32
#define GLFW_EXPOSE_NATIVE_WGL
#include "GLFW/glfw3native.h"
#endif

namespace vulpes {

    using glfw_window_t = std::integral_constant<int, 0>;
    using wayland_window_t = std::integral_constant<int, 1>;

    #ifdef(__linux__)
    struct _wayland_window {
        wl_display* display = nullptr;
        wl_registry* registry = nullptr;
        wl_compositor* compositor = nullptr;
        wl_shell* shell = nullptr;
        wl_seat* seat = nullptr;
        wl_pointer* pointer = nullptr;
        wl_keyboard* keyboard = nullptr;
        wl_surface* surface = nullptr;
        wl_shell_surface* shell_surface = nullptr;
    };
    #endif

    #ifdef(_WIN32) 
    struct _glfw_window {
        glfwWindow* window = nullptr;
    };
    #endif
}

#endif //!VULPES_VK_WINDOW_H