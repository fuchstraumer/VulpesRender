#pragma once
#ifndef VULPES_VK_INSTANCE_H
#define VULPES_VK_INSTANCE_H
#include "stdafx.h"
#include "engine/renderer/ForwardDecl.h"
#include "engine/renderer/NonCopyable.h"
#include "Camera.h"
#include "../gui/imguiWrapper.h"

namespace vulpes {

	struct PhysicalDeviceFactory;

	class Instance : NonMovable {
	public:
		
		Instance() = default;

		virtual void SetupSurface() = 0;
		virtual void SetupPhysicalDevices();
		void UpdateMovement(const float& dt);
		virtual ~Instance();

		// Allocators 
		static const VkAllocationCallbacks* AllocationCallbacks;

		const VkInstance& vkHandle() const;
		operator VkInstance() const;
		const VkSurfaceKHR GetSurface() const;
		const VkPhysicalDevice& GetPhysicalDevice() const noexcept;
		// Debug callbacks
		VkDebugReportCallbackEXT errorCallback, warningCallback, perfCallback, infoCallback, vkCallback;
		bool validationEnabled;
		
		std::vector<const char*> extensions;
		std::vector<const char*> layers;

		static std::array<bool, 1024> keys;
		static std::array<bool, 3> mouse_buttons;
		static float LastX, LastY;
		static float mouseDx, mouseDy;
		float frameTime;
		static bool cameraLock;

		VkSurfaceKHR surface = VK_NULL_HANDLE;
		
		PhysicalDeviceFactory* physicalDeviceFactory;
		PhysicalDevice* physicalDevice;

		glm::mat4 GetViewMatrix() const noexcept;
		glm::mat4 GetProjectionMatrix() const noexcept;
		glm::vec3 GetCamPos() const noexcept;

		glm::mat4 projection;

		void SetCamPos(const glm::vec3& pos);
		void SetCamRot(const glm::vec3& rot);

	protected:
		static Camera cam;
		VkInstance handle;
		uint32_t width, height;
		VkInstanceCreateInfo createInfo;

	};


	class InstanceGLFW : public Instance {
	public:

		InstanceGLFW(VkInstanceCreateInfo create_info, const bool& enable_validation, const uint32_t& width = DEFAULT_WIDTH, const uint32_t& height = DEFAULT_HEIGHT);

		void CreateWindowGLFW(const bool& fullscreen_enabled = false);

		virtual void SetupSurface() override;

		GLFWwindow* Window;

		static void MousePosCallback(GLFWwindow* window, double mouse_x, double mouse_y);
		//static void MouseButtonCallback(GLFWwindow* window, int button, int action, int code);
		//static void MouseScrollCallback(GLFWwindow* window, double x_offset, double y_offset);
		static void KeyboardCallback(GLFWwindow* window, int key, int scan_code, int action, int mods);
		static void ResizeCallback(GLFWwindow* window, int width, int height);
	};

	class InstanceAndroid : public Instance {
	public:

		InstanceAndroid(VkInstanceCreateInfo* create_info, const bool& enable_validation, const uint32_t& width = DEFAULT_WIDTH, const uint32_t& height = DEFAULT_HEIGHT);

		virtual void SetupSurface() override;

	};

}

#endif // !INSTANCE_H
