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

#include "InputHandler.hpp"
#include "ForwardDecl.hpp"
namespace vulpes {

    class Window {
        Window(const Window& other) = delete;
        Window& operator=(const Window& other) = delete;
    public:

        Window(const Instance* parent_instance, const uint32_t& width, const uint32_t& height);
        ~Window();

        void SetWindowUserPointer(std::any user_ptr);

        GLFWwindow* glfwWindow() noexcept;
        const std::vector<const char*>& Extensions() const noexcept;
        const VkSurfaceKHR& vkSurface() const noexcept;
        glm::ivec2 GetWindowSize() const noexcept;
        static void ResizeCallback(GLFWwindow* window, int width, int height);

        std::unique_ptr<input_handler> InputHandler;
    private:

        void createWindow();
        void createInputHandler();
        void createSurface();
        void setExtensions();

        
        GLFWwindow* window;
        uint32_t width, height;
        const Instance* parent;
        VkSurfaceKHR surface;
        std::vector<const char*> extensions;

    };
}

#endif //!VULPES_VK_WINDOW_H