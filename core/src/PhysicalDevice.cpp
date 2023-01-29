#include "vpr_stdafx.h"
#include "PhysicalDevice.hpp"
#include "Instance.hpp"
#include <set>
#include <unordered_map>
#include <algorithm>
#include <iterator>
#include <iostream>

namespace vpr
{
	namespace detail
	{
		std::vector<size_t> getSTypesFromDeviceFeaturesStruct(const void* pNext);
		bool featuresAvailableMatchFeaturesRequested(const VkPhysicalDeviceFeatures& requestedFeatures, const VkPhysicalDeviceFeatures& supportedFeatures);
		constexpr uint32_t convertApiVersion(const VprExtensionPack::ApiVersion version);
		VkPhysicalDevice chooseBestDevice(const size_t numDevices, const VkPhysicalDevice* devicesArray, const VprExtensionPack::ApiVersion _desiredApiVersion);
	}

	struct PhysicalDeviceImpl
	{
		PhysicalDeviceImpl() = default;
		~PhysicalDeviceImpl() = default;
		PhysicalDeviceImpl(const PhysicalDeviceImpl& other) = delete;
		PhysicalDeviceImpl& operator=(const PhysicalDeviceImpl& other) = delete;

		void InitializeDevice(const VkInstance& instance, const VprExtensionPack* extension_pack);

		// just returns all available devices on system
		std::vector<VkPhysicalDevice> getAvailableDevices(const VkInstance& instance);
		// returns device most compatible with requested extensions
		VkPhysicalDevice findMostCompatibleDevice(const VprExtensionPack* extension_pack, const std::vector<VkPhysicalDevice>& avail_devices);
		// returns "best" device by looking for one with least restrictive limits and biggest texture sizes 
		VkPhysicalDevice chooseIdealDevice(const std::vector<VkPhysicalDevice>& avail_devices, const VprExtensionPack::ApiVersion desiredApiVersion);

		void getQueueFamilyProperties();
		uint32_t getQueueFamilyIndex(const VkQueueFlagBits bitfield) const noexcept;

		VkPhysicalDevice handle{ VK_NULL_HANDLE };
		VkPhysicalDeviceMemoryProperties memoryProperties;
		std::vector<VkQueueFamilyProperties> queueFamilyProperties;

	};

	void PhysicalDeviceImpl::InitializeDevice(const VkInstance& instance, const VprExtensionPack* extension_pack)
	{
		VkPhysicalDevice chosenDevice = VK_NULL_HANDLE;
		std::vector<VkPhysicalDevice> availDevices = getAvailableDevices(instance);

		if (extension_pack->featuresToEnable2)
		{
			chosenDevice = findMostCompatibleDevice(extension_pack, availDevices);
		}
		else
		{
			chosenDevice = chooseIdealDevice(availDevices, extension_pack->PreferredApiVersion);
		}

		handle = chosenDevice;

		vkGetPhysicalDeviceMemoryProperties(handle, &memoryProperties);
		getQueueFamilyProperties();
	}

	std::vector<VkPhysicalDevice> PhysicalDeviceImpl::getAvailableDevices(const VkInstance& instance)
	{
		uint32_t deviceCount = 0u;
		vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
		std::vector<VkPhysicalDevice> results(deviceCount, VK_NULL_HANDLE);
		vkEnumeratePhysicalDevices(instance, &deviceCount, results.data());
		return results;
	}

	VkPhysicalDevice PhysicalDeviceImpl::findMostCompatibleDevice(const VprExtensionPack* extension_pack, const std::vector<VkPhysicalDevice>& avail_devices)
	{
		// Gather requested device features first
		std::vector<size_t> requestedFeatureStypes = detail::getSTypesFromDeviceFeaturesStruct(extension_pack->featuresToEnable2->pNext);
		// we need to store compatible devices as we go - on some systems, we may get multiple results (like CPU graphics) being supported
		// with smaller extension sets, alongside the dedicated GPU. So we pass just the list of compatible devices to chooseIdealDevice,
		// which will choose the best option from the ones that are compatible with the requested features/props :)
		std::vector<VkPhysicalDevice> compatibleDevices;

		for (size_t i = 0; i < avail_devices.size(); ++i)
		{
			VkPhysicalDevice currDevice = avail_devices[i];

			VkPhysicalDeviceFeatures2 supportedFeatures
			{
				VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
				nullptr,
				VkPhysicalDeviceFeatures{}
			};
			memset(&supportedFeatures.features, 0, sizeof(VkPhysicalDeviceFeatures));

			vkGetPhysicalDeviceFeatures2(currDevice, &supportedFeatures);

			// compare boolean supported features first
			const bool baseFeaturesMatch =
				detail::featuresAvailableMatchFeaturesRequested(extension_pack->featuresToEnable2->features, supportedFeatures.features);

			// find number of common supported advanced/extension-based features
			std::vector<size_t> availFeatureStypes = detail::getSTypesFromDeviceFeaturesStruct(supportedFeatures.pNext);
			// get the union of the two above sets, to find the list of features requested that are also supported
			std::vector<size_t> availAndRequestedFeatures;
			availAndRequestedFeatures.reserve(requestedFeatureStypes.size());
			std::set_intersection(
				requestedFeatureStypes.begin(), requestedFeatureStypes.end(),
				availFeatureStypes.begin(), availFeatureStypes.end(),
				std::back_inserter(availAndRequestedFeatures));
			// now, compare the intersection set with the requested set: if they're equal, we're good to go!
			const bool allFeaturesSupported = std::equal(
				requestedFeatureStypes.begin(), requestedFeatureStypes.end(),
				availAndRequestedFeatures.begin(), availAndRequestedFeatures.end());

			if (baseFeaturesMatch && allFeaturesSupported)
			{
				compatibleDevices.emplace_back(currDevice);
			}
		}

		return chooseIdealDevice(compatibleDevices, extension_pack->PreferredApiVersion);
	}

	VkPhysicalDevice PhysicalDeviceImpl::chooseIdealDevice(const std::vector<VkPhysicalDevice>& avail_devices, const VprExtensionPack::ApiVersion _desiredApiVersion)
	{
		return detail::chooseBestDevice(avail_devices.size(), avail_devices.data(), _desiredApiVersion);
	}

	void PhysicalDeviceImpl::getQueueFamilyProperties()
	{
		uint32_t queue_family_cnt = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(handle, &queue_family_cnt, nullptr);
		queueFamilyProperties.resize(queue_family_cnt, VkQueueFamilyProperties{ VkQueueFlags(0), 0, 0, VkExtent3D{0, 0, 0} });
		vkGetPhysicalDeviceQueueFamilyProperties(handle, &queue_family_cnt, queueFamilyProperties.data());
	}

	uint32_t PhysicalDeviceImpl::getQueueFamilyIndex(const VkQueueFlagBits bitfield) const noexcept
	{
		if (bitfield & VK_QUEUE_COMPUTE_BIT)
		{
			for (uint32_t i = 0; i < static_cast<uint32_t>(queueFamilyProperties.size()); ++i)
			{
				if ((queueFamilyProperties[i].queueFlags & bitfield) && ((queueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0))
				{
					return i;
				}
			}
		}

		if (bitfield & VK_QUEUE_TRANSFER_BIT)
		{
			for (uint32_t i = 0; i < static_cast<uint32_t>(queueFamilyProperties.size()); ++i)
			{
				if ((queueFamilyProperties[i].queueFlags & bitfield) && ((queueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0) && ((queueFamilyProperties[i].queueFlags & VK_QUEUE_COMPUTE_BIT) == 0))
				{
					return i;
				}
			}
		}

		for (uint32_t i = 0; i < static_cast<uint32_t>(queueFamilyProperties.size()); ++i)
		{
			if (queueFamilyProperties[i].queueFlags & bitfield)
			{
				return i;
			}
		}

		std::cerr << "Failed to find desired queue family with given flags.\n";
		return std::numeric_limits<uint32_t>::max();
	}

	PhysicalDevice::PhysicalDevice(const VkInstance& instance_handle, const VprExtensionPack* extension_pack) : impl(std::make_unique<PhysicalDeviceImpl>())
	{
		impl->InitializeDevice(instance_handle, extension_pack);
	}

	PhysicalDevice::PhysicalDevice(PhysicalDevice&& other) noexcept : impl(std::move(other.impl))
	{
		other.impl = nullptr;
	}

	PhysicalDevice& PhysicalDevice::operator=(PhysicalDevice&& other) noexcept
	{
		impl = std::move(other.impl);
		other.impl = nullptr;
		return *this;
	}

	PhysicalDevice::~PhysicalDevice()
	{
		impl.reset();
	}

	const VkPhysicalDevice& PhysicalDevice::vkHandle() const noexcept
	{
		return impl->handle;
	}

	const VkPhysicalDeviceMemoryProperties& PhysicalDevice::MemoryProperties() const noexcept
	{
		return impl->memoryProperties;
	}

	uint32_t PhysicalDevice::GetQueueFamilyIndex(const VkQueueFlagBits bitfield) const noexcept
	{
		return impl->getQueueFamilyIndex(bitfield);
	}

	VkQueueFamilyProperties PhysicalDevice::GetQueueFamilyProperties(const VkQueueFlagBits bitfield) const
	{
		const uint32_t idx = impl->getQueueFamilyIndex(bitfield);
		if (idx != std::numeric_limits<uint32_t>::max())
		{
			return impl->queueFamilyProperties[idx];
		}
		else
		{
			std::cerr << "Failed to retrieve queue family properties: couldn't find queue family with given bitfield.\n";
			return VkQueueFamilyProperties();
		}

	}

	VPR_API VkPhysicalDevice ChooseBestScoringPhysicalDevice(const size_t numDevices, const VkPhysicalDevice* devices)
	{
		return detail::chooseBestDevice(numDevices, devices, vpr::VprExtensionPack::ApiVersion::BestSupported);
	}


	namespace detail
	{

		std::vector<size_t> getSTypesFromDeviceFeaturesStruct(const void* pNext)
		{
			std::vector<size_t> featureStypes;

			while (pNext != nullptr)
			{
				// structures are arranged so that sType is in first 8 bytes, pNext is in second 8 bytes
				const size_t sType = *reinterpret_cast<const size_t*>(pNext);
				featureStypes.emplace_back(sType);
				const std::byte* newAddr = reinterpret_cast<const std::byte*>(pNext);
				newAddr += sizeof(size_t);
				pNext = reinterpret_cast<const void*>(newAddr);
			}

			std::sort(featureStypes.begin(), featureStypes.end());

			return featureStypes;
		}

		bool featuresAvailableMatchFeaturesRequested(const VkPhysicalDeviceFeatures& requestedFeatures, const VkPhysicalDeviceFeatures& supportedFeatures)
		{
			int score = std::memcmp(&requestedFeatures, &supportedFeatures, sizeof(VkPhysicalDeviceFeatures));
			return score == 0;
		}

		constexpr uint32_t convertApiVersion(const VprExtensionPack::ApiVersion version)
		{
			switch (version)
			{
			case VprExtensionPack::ApiVersion::BestSupported:
				// we check if supported < desired, so when desired is 0 the check always passes
				// and we just run with whatever the current device supports
				return 0;
			case VprExtensionPack::ApiVersion::Vulkan10:
				return VK_API_VERSION_1_0;
			case VprExtensionPack::ApiVersion::Vulkan11:
				return VK_API_VERSION_1_1;
			case VprExtensionPack::ApiVersion::Vulkan12:
				return VK_API_VERSION_1_2;
			case VprExtensionPack::ApiVersion::Vulkan13:
				return VK_API_VERSION_1_3;
			default:
				return VK_API_VERSION_1_3;
			}
		}

		VkPhysicalDevice chooseBestDevice(const size_t numDevices, const VkPhysicalDevice* devicesArray, const VprExtensionPack::ApiVersion _desiredApiVersion)
		{
			uint32_t desiredApiVersion = detail::convertApiVersion(_desiredApiVersion);

			size_t bestDeviceIdx = 0;
			size_t bestScore = 0;

			for (size_t i = 0; i < numDevices; ++i)
			{
				size_t deviceScore = 0;
				// use the simple device properties function. the stuff returned by deviceProperties2 is mostly 
				// relevant later, but here we're gonna use simple metrics to land on whichever device has the most Oomph
				VkPhysicalDeviceProperties properties;
				vkGetPhysicalDeviceProperties(devicesArray[i], &properties);

				if (properties.apiVersion < desiredApiVersion)
				{
					// try the next one, I guess
					continue;
				}

				if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
				{
					deviceScore += 10000;
				}
				else if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU)
				{
					deviceScore += 1000;
				}

				VkPhysicalDeviceFeatures features;
				vkGetPhysicalDeviceFeatures(devicesArray[i], &features);

				if (features.geometryShader)
				{
					deviceScore += 1000;
				}

				if (features.tessellationShader)
				{
					deviceScore += 1000;
				}

				if (features.samplerAnisotropy)
				{
					deviceScore += 250;
				}

				if (features.imageCubeArray)
				{
					deviceScore += 250;
				}

				if (features.fullDrawIndexUint32)
				{
					deviceScore += 500;
				}

				if (features.multiDrawIndirect)
				{
					deviceScore += 500;
				}

				if (features.textureCompressionETC2)
				{
					deviceScore += 250;
				}

				if (features.textureCompressionASTC_LDR)
				{
					deviceScore += 250;
				}

				// what happens in case of a tie? multi-GPU mode
				if (deviceScore > bestScore)
				{
					bestDeviceIdx = i;
					bestScore = deviceScore;
				}
			}

			if (bestScore != 0u)
			{
				return devicesArray[bestDeviceIdx];
			}
			else
			{
				// likely no devices that supported requested version of the API
				return VK_NULL_HANDLE;
			}
		}
	}

}
