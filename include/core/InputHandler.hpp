#ifndef VULPES_VK_INPUT_HANDLER_HPP
#define VULPES_VK_INPUT_HANDLER_HPP

#include "vpr_stdafx.h"

namespace vulpes {

    class Window;

    struct InputHandler {

        InputHandler(const Window* _parent);

        void MouseDrag(const int& button, const float& dx, const float& dy);
        void MouseScroll(const int& button, const float& delta);
        void MouseDown(const int& button, const float& x, const float& y);
        void MouseUp(const int& button, const float& x, const float& y);

        void UpdateMovement(const float& dt);

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
        static bool CameraLock;

    private:

        void setImguiMapping() const;
        void setCallbacks() const;
        const Window* parent;
    };

}

#endif //!VULPES_VK_INPUT_HANDLER_HPP