#pragma once
#ifndef VULPES_VK_INSTANCE_H
#define VULPES_VK_INSTANCE_H
#include "vpr_stdafx.h"
#include "../ForwardDecl.h"
#include "../util/Camera.h"
#include "../util/Arcball.h"
#include "../gui/imguiWrapper.h"

namespace vulpes {

	struct PhysicalDeviceFactory;

	enum class cameraType {
		FPS,
		FREE,
		ARCBALL
	};

	struct vulpesInstanceInfo {
		std::string ApplicationName = std::string("DefaultAppName"), EngineName = std::string("DefaultEngineName");
		uint32_t ApplicationVersion = VK_MAKE_VERSION(0,1,0), EngineVersion = VK_MAKE_VERSION(0,1,0), apiVersion = VK_API_VERSION_1_0;
		bool EnableValidation;
		bool EnableMarkers = true;
		bool EnableFullscreen = true;
		bool DefaultFullscreen = false;
		VkRect2D DefaultWindowSize = VkRect2D{ VkOffset2D{}, VkExtent2D{ 1440, 900 } };
		cameraType CameraType = cameraType::FPS;
		bool EnableMSAA = true;
		bool EnableMouseLocking = false;
		float MouseSensitivity = 0.2f;
		VkSampleCountFlagBits SampleCount = VK_SAMPLE_COUNT_4_BIT;
		bool EnableHDR = false;
		bool EnableBloom = false;
		bool TextureAnisotropy = false;
		VkSampleCountFlagBits AnisotropySamples = VK_SAMPLE_COUNT_1_BIT;
	};


	class Instance {
		Instance(const Instance&) = delete;
		Instance(Instance&&) = delete;
		Instance& operator=(const Instance&) = delete;
		Instance& operator=(Instance&&) = delete;
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
		static float mouseScroll;
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
		void UpdateCameraRotation(const float& rot_x, const float& rot_y);
		void UpdateCameraZoom(const float& zoom_delta);

		static vulpesInstanceInfo VulpesInstanceConfig;

	protected:
		static Camera cam;
		static Arcball arcball;
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
		static void MouseButtonCallback(GLFWwindow* window, int button, int action, int code);
		static void MouseScrollCallback(GLFWwindow* window, double x_offset, double y_offset);
		static void KeyboardCallback(GLFWwindow* window, int key, int scan_code, int action, int mods);
		static void CharCallback(GLFWwindow *, unsigned int c);
		static void ResizeCallback(GLFWwindow* window, int width, int height);
	};

}

#endif // !INSTANCE_H
