#pragma once
#ifndef VULPES_VK_PHYSICAL_DEVICE_H
#define VULPES_VK_PHYSICAL_DEVICE_H
#include "stdafx.h"
namespace vulpes {

	class PhysicalDevice {
	public:
		PhysicalDevice() = delete;
		PhysicalDevice(const PhysicalDevice& other) = delete;
		PhysicalDevice& operator=(const PhysicalDevice& other) = delete;

		PhysicalDevice(const VkPhysicalDevice& handle);

		uint32_t GetMemoryTypeIdx(const uint32_t& type_bitfield, const VkMemoryPropertyFlags& property_flags, VkBool32* memory_type_found = nullptr) const;
		uint32_t GetQueueFamilyIndex(const VkQueueFlagBits& bitfield) const;
		VkQueueFamilyProperties GetQueueFamilyProperties(const VkQueueFlagBits& bitfield) const;

		const VkPhysicalDevice& vkHandle() const;
		operator VkPhysicalDevice() const;

		VkPhysicalDeviceProperties Properties;
		VkPhysicalDeviceFeatures Features;
		//VkPhysicalDeviceFeatures EnabledFeatures;
		VkPhysicalDeviceMemoryProperties MemoryProperties;
		std::vector<VkQueueFamilyProperties> QueueFamilyProperties;
		std::vector<VkExtensionProperties> ExtensionProperties;
	private:
		VkPhysicalDevice handle;
		const VkAllocationCallbacks* AllocationCallbacks;
		
	};

	struct PhysicalDeviceFactory {
		VkPhysicalDevice GetBestDevice(const VkInstance& parent_instance);
		std::map<uint32_t, VkPhysicalDevice> UnusedDevices;
	};

}
#endif // !VULPES_VK_PHYSICAL_DEVICE_H
