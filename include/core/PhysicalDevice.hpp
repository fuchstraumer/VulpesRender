#pragma once
#ifndef VULPES_VK_PHYSICAL_DEVICE_H
#define VULPES_VK_PHYSICAL_DEVICE_H
#include "vpr_stdafx.h"
namespace vulpes {

	class PhysicalDevice {
        PhysicalDevice(const PhysicalDevice& other) = delete;
        PhysicalDevice& operator=(const PhysicalDevice& other) = delete;
	public:

        PhysicalDevice(const VkInstance& instance_handle);
        const VkPhysicalDevice& vkHandle() const noexcept;

		uint32_t GetMemoryTypeIdx(const uint32_t& type_bitfield, const VkMemoryPropertyFlags& property_flags, VkBool32* memory_type_found = nullptr) const noexcept;
		uint32_t GetQueueFamilyIndex(const VkQueueFlagBits& bitfield) const noexcept ;
        VkQueueFamilyProperties GetQueueFamilyProperties(const VkQueueFlagBits& bitfield) const;

		VkPhysicalDeviceProperties Properties;
		VkPhysicalDeviceFeatures Features;
		VkPhysicalDeviceMemoryProperties MemoryProperties;
        std::vector<VkExtensionProperties> ExtensionProperties;

	private:

        void getAttributes() noexcept;
        void retrieveQueueFamilyProperties() noexcept;
        void retrieveExtensionProperties() noexcept;
        VkPhysicalDevice getBestDevice(const VkInstance & parent_instance);

        std::vector<VkQueueFamilyProperties> queueFamilyProperties;
		VkPhysicalDevice handle;
		
	};

}
#endif // !VULPES_VK_PHYSICAL_DEVICE_H
