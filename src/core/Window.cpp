#include "vpr_stdafx.h"
#include "core/Window.hpp"
#include <imgui.h>
#include "core/Instance.hpp"
#include "core/BaseScene.hpp"

namespace vulpes {

    Window::Window(const Instance* instance, const uint32_t& _width, const uint32_t& _height) : Parent(instance), width(_width), height(_height) {

        glfwInit();
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        createWindow();
    }

    Window::~Window() {
        vkDestroySurfaceKHR(Parent->vkHandle(), surface, nullptr);
    }

    void Window::createWindow() {

        std::string window_title = Instance::VulpesInstanceConfig.ApplicationName;
        window = glfwCreateWindow(static_cast<int>(width), static_cast<int>(height), window_title.c_str(), nullptr, nullptr);

        ImGuiIO& io = ImGui::GetIO();
#ifdef _WIN32
		io.ImeWindowHandle = glfwGetWin32Window(Window);
#endif
        io.DisplayFramebufferScale = ImVec2(1.0f, 1.0f);
        io.DisplaySize = ImVec2(static_cast<float>(width), static_cast<float>(height));

        glfwSetWindowSizeCallback(window, ResizeCallback);
        createInputHandler(); // TODO: Input handler class.
        getExtensions();
        createSurface();

    }

    const GLFWwindow* Window() const noexcept {
        return window;
    }

    void Window::setExtensions() {

        uint32_t extension_count = 0;
        const char** extension_names;
        names = glfwGetRequiredInstanceExtensions(&extension_count);
        for(uint32_t i = 0; i < extension_count; ++i) {
            extensions.emplace_back(extension_names[i]);
        }

    }

    const std::vector<std::string>& Window::Extensions() const noexcept {
        return extensions;
    }

    void Window::createSurface() {

        VkResult err = glfwCreateWindowSurface(Parent->vkHandle(), window, nullptr, &surface);
        VkAssert(err);
        LOG(INFO) << "Created window surface.";

    }

    void Window::createInputHandler() {
        InputHandler = std::make_unique<InputHandler>(this);
    }

    void Window::ResizeCallback(GLFWwindow* window, int width, int height) {

        if(width == 0 || height == 0) {
            LOG(WARNING) << "Resize callback called with zero width or height for window.";
            return;
        }

        ImGuiIO& io = ImGui::GetIO();
        io.DisplaySize = ImVec2(static_cast<float>(width), static_cast<float>(height));

        BaseScene* scene = reinterpret_cast<BaseScene*>(glfwGetWindowUserPointer(window));
        scene->RecreateSwapchain();

    }

}