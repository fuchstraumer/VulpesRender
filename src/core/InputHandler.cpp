#include "vpr_stdafx.h"
#include "core/InputHandler.hpp"
#include "core/Window.hpp"
#include <imgui.h>

namespace vulpes {

    std::array<bool, 1024> InputHandler::Keys = std::array<bool, 1024>();
    std::array<bool, 3> InputHandler::MouseButtons = std::array<bool, 3>();
    float InputHandler::LastX = 1.0f;
    float InputHandler::LastY = 1.0f;
    float InputHandler::MouseDx = 0.0f;
    float InputHandler::MouseDy = 0.0f;
    bool InputHandler::CameraLock = false;

    InputHandler::InputHandler(const Window* _parent) : parent(_parent) {
        setCallbacks();
        setImguiMapping();
    }

    void InputHandler::MouseDrag(const int& button, const float & rot_x, const float & rot_y) {
		if (Instance::VulpesInstanceConfig.CameraType == cfg::cameraType::ARCBALL) {
			arcball.MouseDrag(button, rot_x, rot_y);
		}
	}

	void InputHandler::MouseScroll(const int& button, const float & zoom_delta) {
		if (Instance::VulpesInstanceConfig.CameraType == cfg::cameraType::ARCBALL) {
			arcball.MouseScroll(button, zoom_delta);
		}
	}

    void InputHandler::MouseDown(const int& button, const float& x, const float& y) {
		if (Instance::VulpesInstanceConfig.CameraType == cfg::cameraType::ARCBALL) {
			arcball.MouseDown(button, x, y);
		}
	}

	void InputHandler::MouseUp(const int& button, const float & x, const float & y) {
		if (Instance::VulpesInstanceConfig.CameraType == cfg::cameraType::ARCBALL) {
			arcball.MouseUp(button, x, y);
		}
	}

    void InputHandler::UpdateMovement(const float& dt) {
        ImGuiIO& io = ImGui::GetIO();
		io.DeltaTime = dt;

		if (io.WantCaptureKeyboard) {
			return;
		}

		if (keys[GLFW_KEY_W]) {
			cam.ProcessKeyboard(Direction::FORWARD, dt);
			arcball.RotateUp(dt);
		}
		if (keys[GLFW_KEY_S]) {
			cam.ProcessKeyboard(Direction::BACKWARD, dt);
			arcball.RotateDown(dt);
		}
		if (keys[GLFW_KEY_D]) {
			cam.ProcessKeyboard(Direction::RIGHT, dt);
			arcball.RotateRight(dt);
		}
		if (keys[GLFW_KEY_A]) {
			cam.ProcessKeyboard(Direction::LEFT, dt);
			arcball.RotateLeft(dt);
		}
		if (keys[GLFW_KEY_X]) {
			cam.ProcessKeyboard(Direction::DOWN, dt);
			arcball.TranslateDown(dt);
		}
		if (keys[GLFW_KEY_C]) {
			cam.ProcessKeyboard(Direction::UP, dt);
			arcball.TranslateUp(dt);
		}
    }

    void InputHandler::setCallbacks() const {

        glfwSetCursorPosCallback(Window, MousePosCallback);
        glfwSetKeyCallback(Window, KeyboardCallback);
        glfwSetMouseButtonCallback(Window, MouseButtonCallback);
        glfwSetScrollCallback(Window, MouseScrollCallback);
        glfwSetCharCallback(Window, CharCallback);
        if (Instance::VulpesInstanceConfig.EnableMouseLocking) {
            glfwSetInputMode(Window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        }
        else {
            glfwSetInputMode(Window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        }

    }

    void InputHandler::setImguiMapping() const {

        ImGuiIO& io = ImGui::GetIO();
		io.KeyMap[ImGuiKey_Tab] = GLFW_KEY_TAB;                        
		io.KeyMap[ImGuiKey_LeftArrow] = GLFW_KEY_LEFT;
		io.KeyMap[ImGuiKey_RightArrow] = GLFW_KEY_RIGHT;
		io.KeyMap[ImGuiKey_UpArrow] = GLFW_KEY_UP;
		io.KeyMap[ImGuiKey_DownArrow] = GLFW_KEY_DOWN;
		io.KeyMap[ImGuiKey_PageUp] = GLFW_KEY_PAGE_UP;
		io.KeyMap[ImGuiKey_PageDown] = GLFW_KEY_PAGE_DOWN;
		io.KeyMap[ImGuiKey_Home] = GLFW_KEY_HOME;
		io.KeyMap[ImGuiKey_End] = GLFW_KEY_END;
		io.KeyMap[ImGuiKey_Delete] = GLFW_KEY_DELETE;
		io.KeyMap[ImGuiKey_Backspace] = GLFW_KEY_BACKSPACE;
		io.KeyMap[ImGuiKey_Enter] = GLFW_KEY_ENTER;
		io.KeyMap[ImGuiKey_Escape] = GLFW_KEY_ESCAPE;
		io.KeyMap[ImGuiKey_A] = GLFW_KEY_A;
		io.KeyMap[ImGuiKey_C] = GLFW_KEY_C;
		io.KeyMap[ImGuiKey_V] = GLFW_KEY_V;
		io.KeyMap[ImGuiKey_X] = GLFW_KEY_X;
		io.KeyMap[ImGuiKey_Y] = GLFW_KEY_Y;
		io.KeyMap[ImGuiKey_Z] = GLFW_KEY_Z;
		io.SetClipboardTextFn = SetClipboardCallback;
		io.GetClipboardTextFn = GetClipboardCallback;

    }

    void InputHandler::MousePosCallback(GLFWwindow * window, double mouse_x, double mouse_y) {
		MouseDx = static_cast<float>(mouse_x) - LastX;
		MouseDy = static_cast<float>(mouse_y) - LastY;

		LastX = static_cast<float>(mouse_x);
		LastY = static_cast<float>(mouse_y);

		ImGuiIO& io = ImGui::GetIO();
		io.MousePos.x = LastX;
		io.MousePos.y = LastY;

		if (!io.WantCaptureMouse && !cameraLock) {
            if(Instance::VulpesInstanceConfig.CameraType == cfg::CameraType::FPS) {
                cam.UpdateMousePos(mouseDx, mouseDy);
            }
            else if(Instance::VulpesInstanceConfig.CameraType == cfg::CameraType::ARCBALL) {
                arcball.UpdateMousePos(mouseDx, mouseDy);
            }
        }
        
	}

	void InputHandler::MouseButtonCallback(GLFWwindow * window, int button, int action, int code) {

        ImGuiIO& io = ImGui::GetIO();
        
		if (button >= 0 && button < 3) {
			if (action == GLFW_PRESS) {
				MouseButtons[button] = true;
				io.MouseDown[button] = true;
			}
			else if (action == GLFW_RELEASE) {
				MouseButtons[button] = false;
				io.MouseDown[button] = false;
			}
		}
	}

    void InputHandler::MouseScrollCallback(GLFWwindow * window, double x_offset, double y_offset) {
        
        mouseScroll += static_cast<float>(y_offset);
        ImGuiIO& io = ImGui::GetIO();
        io.MouseWheel = y_offset;

	}

	void InputHandler::KeyboardCallback(GLFWwindow * window, int key, int scan_code, int action, int mods){
		
		auto io = ImGui::GetIO();

		if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
			glfwSetWindowShouldClose(window, true);
		}

		if (key == GLFW_KEY_LEFT_BRACKET && action == GLFW_PRESS) {
			VulpesInstanceConfig.MovementSpeed += 25.0f;
		}
		if (key == GLFW_KEY_RIGHT_BRACKET && action == GLFW_PRESS) {
			VulpesInstanceConfig.MovementSpeed -= 25.0f;
		}

		if (key >= 0 && key < 1024) {
			if (action == GLFW_PRESS) {
				Keys[key] = true;
				io.KeysDown[key] = true;
			}
			if (action == GLFW_RELEASE) {
				Keys[key] = false;
				io.KeysDown[key] = false;
			}
		}

	}

	void InputHandler::CharCallback(GLFWwindow*, unsigned int c) {
		ImGuiIO& io = ImGui::GetIO();
		if (c > 0 && c < 0x10000) {
			io.AddInputCharacter(static_cast<unsigned short>(c));
		}
	}

	void InputHandler::SetClipboardCallback(void * window, const char * text) {
		glfwSetClipboardString(reinterpret_cast<GLFWwindow*>(window), text);
	}

	const char* InputHandler::GetClipboardCallback(void* window) {
		return glfwGetClipboardString(reinterpret_cast<GLFWwindow*>(window));
	}

}