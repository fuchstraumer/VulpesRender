#ifndef VULPES_VK_WINDOW_H
#define VULPES_VK_WINDOW_H
#include "vpr_stdafx.h"
#define GLFW_INCLUDE_VULKAN
#include "glfw/glfw3.h"

#if defined(__linux__)
#include <wayland-client.h>
#elif defined(_WIN32) 
#undef APIENTRY
#define GLFW_EXPOSE_NATIVE_WIN32
#define GLFW_EXPOSE_NATIVE_WGL
#include "GLFW/glfw3native.h"
#endif

namespace vulpes {

    class Window {
        Window(const Window& other) = delete;
        Window& operator=(const Window& other) = delete;
    public:

        Window(const Instance* parent_instance, const uint32_t& width, const uint32_t& height);
        Window(const VkInstance& parent_instance, const bool& borderless);

        void Recreate();
        void Resize(const uint32_t& width, const uint32_t& height);

        const GLFWwindow* Window() const noexcept;
        const std::vector<std::string>& Extensions() const noexcept;
        const VkSurfaceKHR& vkSurface() const noexcept;

        virtual static void ResizeCallback(GLFWwindow* window, int width, int height);
        
    private:

        void createWindow();
        void createInputHandler();
        void createSurface();
        void setExtensions();

        GLFWwindow* window;
        uint32_t width, height;
        const Instance* parent;
        VkSurfaceKHR surface;
        std::vector<std::string> extensions;

    };
}

#endif //!VULPES_VK_WINDOW_H