#pragma once
#ifndef VULPES_VK_SWAPCHAIN_H
#define VULPES_VK_SWAPCHAIN_H
#include "stdafx.h"
#include "engine/renderer/ForwardDecl.h"
#include "engine/renderer\NonCopyable.h"

namespace vulpes {

	struct SwapchainInfo {
		SwapchainInfo(const VkPhysicalDevice& dvc, const VkSurfaceKHR& sfc);
		VkSurfaceCapabilitiesKHR Capabilities;
		std::vector<VkSurfaceFormatKHR> Formats;
		std::vector<VkPresentModeKHR> PresentModes;
		VkSurfaceFormatKHR GetBestFormat() const;
		VkPresentModeKHR GetBestPresentMode() const;
		VkExtent2D ChooseSwapchainExtent(const Instance* _instance) const;
	};

	class Swapchain : public NonCopyable {
	public:

		~Swapchain();
		
		void Init(const Instance* _instance, const PhysicalDevice* _phys_device, const Device* _device);

		void Recreate();

		uint32_t GetNextImage(const VkSemaphore& present_semaphore = VK_NULL_HANDLE) const;

		void Present(const VkQueue& queue, const uint32_t& image_index, const VkSemaphore& present_semaphore = VK_NULL_HANDLE);

		void Destroy();

		SwapchainInfo* Info;

		const VkSwapchainKHR& vkHandle() const;
		operator VkSwapchainKHR() const;

		const VkAllocationCallbacks* AllocCallbacks = nullptr;
		VkFormat ColorFormat;
		std::vector<VkImage> Images; // one per swap image.
		std::vector<VkImageView> ImageViews;
		uint32_t ImageCount;
		VkColorSpaceKHR ColorSpace;
		VkExtent2D Extent;
	private:

		void setParameters();
		void setupCreateInfo();
		void setupSwapImages();
		void setupImageViews();

		VkPresentModeKHR presentMode;
		VkSurfaceFormatKHR surfaceFormat;
		VkSwapchainCreateInfoKHR createInfo;
		VkSwapchainKHR handle = VK_NULL_HANDLE;
		const Instance* instance;
		const PhysicalDevice* phys_device;
		const Device* device;
	};
}
#endif // !VULPES_VK_SWAPCHAIN_H
