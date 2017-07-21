#include "stdafx.h"
#include "PhysicalDevice.h"

namespace vulpes {

	VkPhysicalDevice PhysicalDeviceFactory::GetBestDevice(const VkInstance & parent_instance){
		// Enumerate devices.
		uint32_t dvc_cnt = 0;
		vkEnumeratePhysicalDevices(parent_instance, &dvc_cnt, nullptr);
		std::vector<VkPhysicalDevice> devices(dvc_cnt);
		vkEnumeratePhysicalDevices(parent_instance, &dvc_cnt, devices.data());

		// device scoring lambda
		auto score_device = [](const VkPhysicalDevice& dvc) {
			VkPhysicalDeviceFeatures features;
			VkPhysicalDeviceProperties properties;
			vkGetPhysicalDeviceFeatures(dvc, &features);
			vkGetPhysicalDeviceProperties(dvc, &properties);
			if (!features.geometryShader || !features.tessellationShader) {
				return 0;
			}

			int score = 0;
			if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
				score += 1000;
			}

			auto tex_sizes = {
				properties.limits.maxImageDimension1D,
				properties.limits.maxImageDimension2D,
				properties.limits.maxImageDimension3D,
			};

			for (auto& sz : tex_sizes) {
				score += sz / 100;
			}
			return score;
		};

		for (const auto& dvc : devices) {
			UnusedDevices.insert(std::make_pair(score_device(dvc), dvc));
		}

		// Get best device handle.
		uint32_t score = 0;
		VkPhysicalDevice best_device = VK_NULL_HANDLE;
		for (const auto& dvc_iter : UnusedDevices) {
			if (dvc_iter.first > score) {
				score = dvc_iter.first;
				best_device = dvc_iter.second;
			}
		}
		assert(score > 0 && best_device != VK_NULL_HANDLE);

		return best_device;
	}

	PhysicalDevice::PhysicalDevice(const VkPhysicalDevice& _handle) : handle(_handle), AllocationCallbacks(nullptr) {
		vkGetPhysicalDeviceProperties(handle, &Properties);
		vkGetPhysicalDeviceFeatures(handle, &Features);
		vkGetPhysicalDeviceMemoryProperties(handle, &MemoryProperties);
		uint32_t queue_family_cnt = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(handle, &queue_family_cnt, nullptr);
		QueueFamilyProperties.resize(queue_family_cnt);
		vkGetPhysicalDeviceQueueFamilyProperties(handle, &queue_family_cnt, QueueFamilyProperties.data());
		uint32_t ext_cnt = 0;
		vkEnumerateDeviceExtensionProperties(handle, nullptr, &ext_cnt, nullptr);
		if (ext_cnt > 0) {
			std::vector<VkExtensionProperties> extensions(ext_cnt);
			if(vkEnumerateDeviceExtensionProperties(handle, nullptr, &ext_cnt, extensions.data()) == VK_SUCCESS){
				ExtensionProperties.assign(extensions.begin(), extensions.end());
			}
		}
	}

	uint32_t PhysicalDevice::GetMemoryTypeIdx(const uint32_t & type_bitfield, const VkMemoryPropertyFlags & property_flags, VkBool32 * memory_type_found) const{
		auto bitfield = type_bitfield;
		for (uint32_t i = 0; i < MemoryProperties.memoryTypeCount; ++i) {
			if (bitfield & 1) {
				// check if property flags match
				if ((MemoryProperties.memoryTypes[i].propertyFlags & property_flags) == property_flags) {
					if (memory_type_found) {
						*memory_type_found = true;
					}
					return i;
				}
			}
			bitfield >>= 1;
		}
		return std::numeric_limits<uint32_t>::max();
	}

	uint32_t PhysicalDevice::GetQueueFamilyIndex(const VkQueueFlagBits & bitfield) const{
		if (bitfield & VK_QUEUE_COMPUTE_BIT) {
			for (uint32_t i = 0; i < static_cast<uint32_t>(QueueFamilyProperties.size()); ++i) {
				if ((QueueFamilyProperties[i].queueFlags & bitfield) && ((QueueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0)) {
					return i;
				}
			}
		}

		if (bitfield & VK_QUEUE_TRANSFER_BIT) {
			for (uint32_t i = 0; i < static_cast<uint32_t>(QueueFamilyProperties.size()); ++i) {
				if ((QueueFamilyProperties[i].queueFlags & bitfield) && ((QueueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0) && ((QueueFamilyProperties[i].queueFlags & VK_QUEUE_COMPUTE_BIT) == 0)) {
					return i;
				}
			}
		}

		for (uint32_t i = 0; i < static_cast<uint32_t>(QueueFamilyProperties.size()); ++i) {
			if (QueueFamilyProperties[i].queueFlags & bitfield) {
				return i;
			}
		}

		return std::numeric_limits<uint32_t>::max();
	}

	VkQueueFamilyProperties PhysicalDevice::GetQueueFamilyProperties(const VkQueueFlagBits & bitfield) const{
		// Same process as previous: try to find dedicated queues for compute/transfer alone first, then generalize the search.
		if (bitfield & VK_QUEUE_COMPUTE_BIT) {
			for (uint32_t i = 0; i < static_cast<uint32_t>(QueueFamilyProperties.size()); ++i) {
				if ((QueueFamilyProperties[i].queueFlags & bitfield) && ((QueueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0)) {
					return QueueFamilyProperties[i];
				}
			}
		}
		if (bitfield & VK_QUEUE_TRANSFER_BIT) {
			for (uint32_t i = 0; i < static_cast<uint32_t>(QueueFamilyProperties.size()); ++i) {
				if ((QueueFamilyProperties[i].queueFlags & bitfield) && ((QueueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0) && ((QueueFamilyProperties[i].queueFlags & VK_QUEUE_COMPUTE_BIT) == 0)) {
					return QueueFamilyProperties[i];
				}
			}
		}

		for (uint32_t i = 0; i < static_cast<uint32_t>(QueueFamilyProperties.size()); ++i) {
			if (QueueFamilyProperties[i].queueFlags & bitfield) {
				return QueueFamilyProperties[i];
			}
		}
		return VkQueueFamilyProperties();
	}

	const VkPhysicalDevice & PhysicalDevice::vkHandle() const{
		return handle;
	}

	PhysicalDevice::operator VkPhysicalDevice() const{
		return handle;
	}

}