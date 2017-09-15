#pragma once
#ifndef VULPES_VK_WINDOW_H
#define VULPES_VK_WINDOW_H
#include "vpr_stdafx.h"
#include "InputHandler.hpp"
#include "ForwardDecl.hpp"
namespace vulpes {

    class Window {
        Window(const Window& other) = delete;
        Window& operator=(const Window& other) = delete;
    public:

        Window(const Instance* parent_instance, const uint32_t& width, const uint32_t& height);
        ~Window();

        void SetWindowUserPointer(void* user_ptr);
        void CreateSurface();
        GLFWwindow* glfwWindow() noexcept;
        const std::vector<const char*>& Extensions() const noexcept;
        const VkSurfaceKHR& vkSurface() const noexcept;
        glm::ivec2 GetWindowSize() const noexcept;
        static void ResizeCallback(GLFWwindow* window, int width, int height);
        
        std::unique_ptr<input_handler> InputHandler;
    private:

        void createWindow();
        void createInputHandler();
        
        void setExtensions();

        
        GLFWwindow* window;
        uint32_t width, height;
        const Instance* parent;
        VkSurfaceKHR surface;
        std::vector<const char*> extensions;

    };
}

#endif //!VULPES_VK_WINDOW_H