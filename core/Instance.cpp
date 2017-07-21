#include "stdafx.h"
#include "Instance.h"
#include "common/VkDebug.h"
#include "PhysicalDevice.h"
#include "scenes\BaseScene.h"
#ifndef VK_CUSTOM_ALLOCATION_CALLBACKS
const VkAllocationCallbacks* vulpes::Instance::AllocationCallbacks = nullptr;
#endif // !VK_CUSTOM_ALLOCATION_CALLBACKS

vulpes::Camera vulpes::Instance::cam = vulpes::Camera();

namespace vulpes {

	std::array<bool, 1024> Instance::keys = std::array<bool, 1024>();
	float Instance::LastX = DEFAULT_WIDTH / 2.0f;
	float Instance::LastY = DEFAULT_HEIGHT / 2.0f;
	float Instance::mouseDx = 0.0f;
	float Instance::mouseDy = 0.0f;
	bool Instance::cameraLock = false;

	void Instance::SetupPhysicalDevices(){
		physicalDeviceFactory = new PhysicalDeviceFactory();
		physicalDevice = new PhysicalDevice(physicalDeviceFactory->GetBestDevice(handle));
	}

	void Instance::UpdateMovement(const float & dt) {
		if (keys[GLFW_KEY_W]) {
			cam.ProcessKeyboard(Direction::FORWARD, dt);
		}
		if (keys[GLFW_KEY_S]) {
			cam.ProcessKeyboard(Direction::BACKWARD, dt);
		}
		if (keys[GLFW_KEY_D]) {
			cam.ProcessKeyboard(Direction::RIGHT, dt);
		}
		if (keys[GLFW_KEY_A]) {
			cam.ProcessKeyboard(Direction::LEFT, dt);
		}
		if (keys[GLFW_KEY_X]) {
			cam.ProcessKeyboard(Direction::DOWN, dt);
		}
		if (keys[GLFW_KEY_C]) {
			cam.ProcessKeyboard(Direction::UP, dt);
		}
	}

	Instance::~Instance(){
		vkDestroySurfaceKHR(handle, surface, AllocationCallbacks);
		if (validationEnabled) {
			DestroyDebugCallback(handle, errorCallback, AllocationCallbacks);
			DestroyDebugCallback(handle, warningCallback, AllocationCallbacks);
			DestroyDebugCallback(handle, infoCallback, AllocationCallbacks);
			DestroyDebugCallback(handle, perfCallback, AllocationCallbacks);
		}
		vkDestroyInstance(handle, AllocationCallbacks);
	}

	const VkInstance& Instance::vkHandle() const{
		return handle;
	}

	Instance::operator VkInstance() const{
		return handle;
	}

	const VkSurfaceKHR Instance::GetSurface() const {
		return surface;
	}

	const VkPhysicalDevice & Instance::GetPhysicalDevice() const noexcept{
		return physicalDevice->vkHandle();
	}

	glm::mat4 Instance::GetViewMatrix() const noexcept{
		return cam.GetViewMatrix();
	}

	glm::mat4 Instance::GetProjectionMatrix() const noexcept{
		return projection;
	}

	glm::vec3 Instance::GetCamPos() const noexcept{
		return cam.Position;
	}

	void Instance::SetCamPos(const glm::vec3 & pos){
		cam.Position = pos;
	}

	InstanceGLFW::InstanceGLFW(VkInstanceCreateInfo create_info, const bool & enable_validation, const uint32_t& _width, const uint32_t& _height) {
		
		width = _width;
		height = _height;
		createInfo = create_info;
		validationEnabled = enable_validation;

		glfwInit();
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

		CreateWindowGLFW();
		
		std::vector<const char*> ext;

		{
			uint32_t cnt = 0;
			const char** names;
			names = glfwGetRequiredInstanceExtensions(&cnt);
			for (uint32_t i = 0; i < cnt; ++i) {
				ext.push_back(names[i]);
			}
		}

		if (enable_validation) {
			create_info.enabledLayerCount = 1;
			create_info.ppEnabledLayerNames = validation_layers.data();
			ext.push_back(debug_callback_extension);
			
		}
		else {
			createInfo.ppEnabledLayerNames = nullptr;
			createInfo.enabledLayerCount = 0;
		}
		createInfo.ppEnabledExtensionNames = ext.data();
		createInfo.enabledExtensionCount = static_cast<uint32_t>(ext.size());
		VkResult err = vkCreateInstance(&createInfo, AllocationCallbacks, &this->handle);
		VkAssert(err);

		if (validationEnabled) {
			CreateDebugCallback(*this, VK_DEBUG_REPORT_WARNING_BIT_EXT, &warningCallback, AllocationCallbacks);
			CreateDebugCallback(*this, VK_DEBUG_REPORT_ERROR_BIT_EXT, &errorCallback, AllocationCallbacks);
			CreateDebugCallback(*this, VK_DEBUG_REPORT_INFORMATION_BIT_EXT, &infoCallback, AllocationCallbacks);
			CreateDebugCallback(*this, VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT, &perfCallback, AllocationCallbacks);
		}
		
		projection = glm::perspective(glm::radians(75.0f), static_cast<float>(width) / static_cast<float>(height), 0.01f, 40000.0f);
		projection[1][1] *= -1.0f;
	}

	void InstanceGLFW::CreateWindowGLFW(const bool & fullscreen_enabled) {
		if (!fullscreen_enabled) {
			Window = glfwCreateWindow(width, height, createInfo.pApplicationInfo->pApplicationName, nullptr, nullptr);
			glfwSetCursorPosCallback(Window, MousePosCallback);
			glfwSetKeyCallback(Window, KeyboardCallback);
			glfwSetWindowSizeCallback(Window, ResizeCallback);

			glfwSetInputMode(Window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
		}
		else {
			GLFWmonitor* monitor = glfwGetPrimaryMonitor();
			// Get data needed from primary monitor to go fullscreen.
			const GLFWvidmode* mode = glfwGetVideoMode(monitor);
			glfwWindowHint(GLFW_RED_BITS, mode->redBits);
			glfwWindowHint(GLFW_GREEN_BITS, mode->greenBits);
			glfwWindowHint(GLFW_BLUE_BITS, mode->blueBits);
			glfwWindowHint(GLFW_REFRESH_RATE, mode->refreshRate);
			Window = glfwCreateWindow(mode->width, mode->height, createInfo.pApplicationInfo->pApplicationName, monitor, NULL);
			vkDestroySurfaceKHR(handle, surface, AllocationCallbacks);
			SetupSurface();
		}


		ImGuiIO& io = ImGui::GetIO();
		io.KeyMap[ImGuiKey_Tab] = GLFW_KEY_TAB;                         // Keyboard mapping. ImGui will use those indices to peek into the io.KeyDown[] array.
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

		io.ImeWindowHandle = glfwGetWin32Window(Window);
		io.DisplayFramebufferScale = ImVec2(1.0f, 1.0f);
		io.DisplaySize = ImVec2(static_cast<float>(DEFAULT_WIDTH), static_cast<float>(DEFAULT_HEIGHT));
	}

	void InstanceGLFW::SetupSurface(){
		VkResult err = glfwCreateWindowSurface(handle, Window, nullptr, &surface);
		VkAssert(err);
	}

	void InstanceGLFW::MousePosCallback(GLFWwindow * window, double mouse_x, double mouse_y) {
		mouseDx = static_cast<float>(mouse_x) - LastX;
		mouseDy = static_cast<float>(mouse_y) - LastY;

		LastX = static_cast<float>(mouse_x);
		LastY = static_cast<float>(mouse_y);

		if (!cameraLock) {
			cam.ProcessMouseMovement(mouseDx, mouseDy);
		}
	}

	void InstanceGLFW::KeyboardCallback(GLFWwindow * window, int key, int scan_code, int action, int mods){
		
		auto io = ImGui::GetIO();
		// Escape key isn't fed into movement update call, just sets a flag
		if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
			glfwSetWindowShouldClose(window, true);
		}

		if (key == GLFW_KEY_LEFT_BRACKET && action == GLFW_PRESS) {
			cam.MovementSpeed += 25.0f;
		}
		if (key == GLFW_KEY_RIGHT_BRACKET && action == GLFW_PRESS) {
			cam.MovementSpeed -= 25.0f;
		}
		// Rest feed into movement
		if (key >= 0 && key < 1024) {
			if (action == GLFW_PRESS) {
				keys[key] = true;
				io.KeysDown[key] = true;
			}
			else if (action == GLFW_RELEASE) {
				keys[key] = false;
				io.KeysDown[key] = false;
			}
		}
	}

	void InstanceGLFW::ResizeCallback(GLFWwindow * window, int width, int height) {
		if (width == 0 || height == 0) {
			return;
		}

		BaseScene* scene = reinterpret_cast<BaseScene*>(glfwGetWindowUserPointer(window));
		scene->RecreateSwapchain();
	}

	InstanceAndroid::InstanceAndroid(VkInstanceCreateInfo * create_info, const bool & enable_validation, const uint32_t & width, const uint32_t & height)
	{
	}

	void InstanceAndroid::SetupSurface()
	{
	}

}
