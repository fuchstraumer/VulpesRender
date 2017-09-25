#pragma once
#ifndef VULPES_VK_INPUT_HANDLER_HPP
#define VULPES_VK_INPUT_HANDLER_HPP

#include "vpr_stdafx.h"
#include "ForwardDecl.hpp"
namespace vulpes {

    class Window;
    /**  The input_handler class "contains" the static callback functions that glfw updates during each frame. It also (truly) contains
    *    two arrays that are updated with input values from the keyboard and mouse, along with parameters relating to the mouse's previous
    *    location and the mouse position's deltas.
    *    \ingroup Core
    */

    struct input_handler {

        input_handler(Window* _parent);

        static void MousePosCallback(GLFWwindow* window, double mouse_x, double mouse_y);
		static void MouseButtonCallback(GLFWwindow* window, int button, int action, int code);
		static void MouseScrollCallback(GLFWwindow* window, double x_offset, double y_offset);
		static void KeyboardCallback(GLFWwindow* window, int key, int scan_code, int action, int mods);
		static void CharCallback(GLFWwindow *, unsigned int c);
		static void SetClipboardCallback(void* window, const char* text);
		static const char* GetClipboardCallback(void* window);

        static std::array<bool, 1024> Keys;
        static std::array<bool, 3> MouseButtons;
        static float LastX, LastY, MouseDx, MouseDy, MouseScroll;

    private:

        void setImguiMapping() const;
        void setCallbacks();
        Window* parent;
    };

}

#endif //!VULPES_VK_INPUT_HANDLER_HPP