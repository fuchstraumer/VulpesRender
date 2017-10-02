#pragma once
#ifndef VULPES_VK_WINDOW_H
#define VULPES_VK_WINDOW_H
#include "vpr_stdafx.h"
#include "InputHandler.hpp"
#include "ForwardDecl.hpp"
namespace vulpes {

#if defined(_WIN32) || defined(__linux__) 
typedef GLFWWindow vulpes_window_t;
#elif defined(__APPLE__) 
typedef void* vulpes_window_t;
#else 
#pragma message("No valid platform detected!")
#endif

    /*! Window is a wrapper around the GLFW windowing system, and handles creating the underlying rendering
    *    window along with creating a suitable VkSurfaceKHR object. It is also responsible for signaling a
    *    window resizing event.
    * \ingroup Core
    */
    class Window {
        Window(const Window& other) = delete;
        Window& operator=(const Window& other) = delete;
    public:

        Window(const Instance* parent_instance, const uint32_t& width, const uint32_t& height);
        ~Window();

        /** !This method attaches any object - usually a scene of some sort - to this window, allowing this class to signal the attached object 
        *   that a window resize or window mode change has occured. This is done to the BaseScene class in the examples for this project.
        */
        void SetWindowUserPointer(void* user_ptr);
        void CreateSurface();
        GLFWwindow* glfwWindow() noexcept;
        const std::vector<const char*>& Extensions() const noexcept;
        const VkSurfaceKHR& vkSurface() const noexcept;
        glm::ivec2 GetWindowSize() const noexcept;

        /**! This method is called when GLFW detects a window resize or window mode change. It is within this method that the object that is pointed
        *    to by the SetWindowUserPointer method is accessed. In this case, the BaseScene class has it's RecreateSwapchain method called.
        *    \todo This method really needs to be generalized, or somehow overridable so that it doesn't call the BaseScene class in case this class is not being used.
        */
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