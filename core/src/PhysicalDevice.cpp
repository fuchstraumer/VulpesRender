#include "vpr_stdafx.h"
#include "PhysicalDevice.hpp"
#include "Instance.hpp"
#include <set>
#include <unordered_map>
#include <algorithm>
#include <iterator>
#include "nonstd/variant.hpp"


namespace vpr
{
	namespace detail
	{

		constexpr static size_t validDevicePropertyStructTypes[]
		{
			static_cast<size_t>(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_PROPERTIES_KHR), // paired features
			static_cast<size_t>(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BLEND_OPERATION_ADVANCED_PROPERTIES_EXT), // paired features
			static_cast<size_t>(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_CONSERVATIVE_RASTERIZATION_PROPERTIES_EXT),
			static_cast<size_t>(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_COOPERATIVE_MATRIX_PROPERTIES_NV), // paired features
			static_cast<size_t>(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_CUSTOM_BORDER_COLOR_PROPERTIES_EXT), // paired features
			static_cast<size_t>(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DEPTH_STENCIL_RESOLVE_PROPERTIES),
			static_cast<size_t>(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_PROPERTIES), // paired features
			static_cast<size_t>(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DEVICE_GENERATED_COMMANDS_PROPERTIES_NV),
			static_cast<size_t>(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DISCARD_RECTANGLE_PROPERTIES_EXT),
			static_cast<size_t>(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTERNAL_MEMORY_HOST_PROPERTIES_EXT),
			static_cast<size_t>(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_DENSITY_MAP_PROPERTIES_EXT),
			static_cast<size_t>(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_DENSITY_MAP_2_PROPERTIES_EXT),
			static_cast<size_t>(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_DENSITY_MAP_OFFSET_PROPERTIES_QCOM),
			static_cast<size_t>(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADER_BARYCENTRIC_PROPERTIES_KHR),
			static_cast<size_t>(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADING_RATE_PROPERTIES_KHR),
			static_cast<size_t>(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADING_RATE_ENUMS_PROPERTIES_NV),
			static_cast<size_t>(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_GRAPHICS_PIPELINE_LIBRARY_PROPERTIES_EXT),
			static_cast<size_t>(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_INLINE_UNIFORM_BLOCK_PROPERTIES),
			static_cast<size_t>(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_INLINE_UNIFORM_BLOCK_PROPERTIES_EXT),
			static_cast<size_t>(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ID_PROPERTIES),
			static_cast<size_t>(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ID_PROPERTIES_KHR),
			static_cast<size_t>(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_LINE_RASTERIZATION_PROPERTIES_EXT),
			static_cast<size_t>(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MAINTENANCE_3_PROPERTIES),
			static_cast<size_t>(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MAINTENANCE_3_PROPERTIES_KHR),
			static_cast<size_t>(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MAINTENANCE_4_PROPERTIES),
			static_cast<size_t>(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MAINTENANCE_4_PROPERTIES_KHR),
			static_cast<size_t>(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_PROPERTIES_NV),
			static_cast<size_t>(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MULTI_DRAW_PROPERTIES_EXT),
			static_cast<size_t>(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MULTIVIEW_PER_VIEW_ATTRIBUTES_PROPERTIES_NVX),
			static_cast<size_t>(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PERFORMANCE_QUERY_PROPERTIES_KHR)

		};

		using PropertyStructVariant = nonstd::variant<
			VkPhysicalDeviceAccelerationStructurePropertiesKHR,
			VkPhysicalDeviceBlendOperationAdvancedPropertiesEXT,
			VkPhysicalDeviceConservativeRasterizationPropertiesEXT,
			VkPhysicalDeviceCooperativeMatrixPropertiesNV,
			VkPhysicalDeviceCustomBorderColorPropertiesEXT,
			VkPhysicalDeviceDepthStencilResolveProperties,
			VkPhysicalDeviceDepthStencilResolvePropertiesKHR,
			VkPhysicalDeviceDescriptorIndexingProperties,
			VkPhysicalDeviceDescriptorIndexingPropertiesEXT,
			VkPhysicalDeviceDeviceGeneratedCommandsPropertiesNV,
			VkPhysicalDeviceDiscardRectanglePropertiesEXT,
			VkPhysicalDeviceDriverProperties,
			VkPhysicalDeviceDriverPropertiesKHR,
			VkPhysicalDeviceFloatControlsProperties,
			VkPhysicalDeviceFloatControlsPropertiesKHR,
			VkPhysicalDeviceExternalMemoryHostPropertiesEXT,
			VkPhysicalDeviceFragmentDensityMapPropertiesEXT,
			VkPhysicalDeviceFragmentDensityMap2PropertiesEXT,
			VkPhysicalDeviceFragmentDensityMapOffsetPropertiesQCOM,
			VkPhysicalDeviceFragmentShaderBarycentricPropertiesKHR,
			VkPhysicalDeviceGraphicsPipelineLibraryPropertiesEXT,
			VkPhysicalDeviceInlineUniformBlockProperties,
			VkPhysicalDeviceLineRasterizationPropertiesEXT,
			VkPhysicalDeviceMaintenance3Properties,
			VkPhysicalDeviceMaintenance3PropertiesKHR,
			VkPhysicalDeviceMaintenance4Properties,
			VkPhysicalDeviceMaintenance4PropertiesKHR,
			VkPhysicalDeviceMeshShaderPropertiesNV,
			VkPhysicalDeviceMultiDrawPropertiesEXT,
			VkPhysicalDeviceMultiviewProperties>;

		constexpr static size_t validDeviceFeatureStructTypes[]
		{
			static_cast<size_t>(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR),
			static_cast<size_t>(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BLEND_OPERATION_ADVANCED_FEATURES_EXT),
			static_cast<size_t>(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_COOPERATIVE_MATRIX_FEATURES_NV),
			static_cast<size_t>(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_CUSTOM_BORDER_COLOR_FEATURES_EXT),
			static_cast<size_t>(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES),
			static_cast<size_t>(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DEVICE_GENERATED_COMMANDS_FEATURES_NV),
			static_cast<size_t>(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_FEATURES_EXT),
			static_cast<size_t>(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_2_FEATURES_EXT),
			static_cast<size_t>(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTERNAL_MEMORY_RDMA_FEATURES_NV),
			static_cast<size_t>(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_DENSITY_MAP_FEATURES_EXT),
			static_cast<size_t>(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_DENSITY_MAP_2_FEATURES_EXT),
			static_cast<size_t>(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_DENSITY_MAP_OFFSET_FEATURES_QCOM),
			static_cast<size_t>(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADER_BARYCENTRIC_FEATURES_KHR),
			static_cast<size_t>(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADER_BARYCENTRIC_FEATURES_NV),
			static_cast<size_t>(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADING_RATE_FEATURES_KHR),
			static_cast<size_t>(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADING_RATE_ENUMS_FEATURES_NV),
			static_cast<size_t>(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_GRAPHICS_PIPELINE_LIBRARY_FEATURES_EXT),
			static_cast<size_t>(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_INLINE_UNIFORM_BLOCK_FEATURES),
			static_cast<size_t>(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_INLINE_UNIFORM_BLOCK_FEATURES_EXT),
			static_cast<size_t>(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_LINE_RASTERIZATION_FEATURES_EXT),
			static_cast<size_t>(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MAINTENANCE_4_FEATURES),
			static_cast<size_t>(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MAINTENANCE_4_FEATURES_KHR),
			static_cast<size_t>(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_FEATURES_NV),
			static_cast<size_t>(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MULTI_DRAW_FEATURES_EXT),
			static_cast<size_t>(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PERFORMANCE_QUERY_FEATURES_KHR)
		};

		constexpr static size_t validQueueFamilyPropertyStructTypes[]
		{
			static_cast<size_t>(VK_STRUCTURE_TYPE_QUEUE_FAMILY_CHECKPOINT_PROPERTIES_NV),
			static_cast<size_t>(VK_STRUCTURE_TYPE_QUEUE_FAMILY_CHECKPOINT_PROPERTIES_2_NV),
			static_cast<size_t>(VK_STRUCTURE_TYPE_QUEUE_FAMILY_GLOBAL_PRIORITY_PROPERTIES_EXT),
			static_cast<size_t>(VK_STRUCTURE_TYPE_QUEUE_FAMILY_GLOBAL_PRIORITY_PROPERTIES_KHR)
		};

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

	}

	struct PhysicalDeviceImpl
	{
		PhysicalDeviceImpl() = default;
		~PhysicalDeviceImpl() = default;

		void InitializeDevice(const VkInstance& instance, const uint32_t instance_version, const VprExtensionPack* extension_pack);

		// just returns all available devices on system
		std::vector<VkPhysicalDevice> getAvailableDevices(const VkInstance& instance);
		// returns device most compatible with requested extensions
		VkPhysicalDevice findMostCompatibleDevice(const VprExtensionPack* extension_pack, const std::vector<VkPhysicalDevice>& avail_devices);
		// when no extensions are really requested from the device, just choose whichever one has the most to offer
		VkPhysicalDevice chooseIdealDevice(const std::vector<VkPhysicalDevice>& avail_devices);
	};

	void PhysicalDeviceImpl::InitializeDevice(const VkInstance& instance, const uint32_t instance_version, const VprExtensionPack* extension_pack)
	{
		VkPhysicalDevice chosenDevice = VK_NULL_HANDLE;
		std::vector<VkPhysicalDevice> availDevices = getAvailableDevices(instance);

		if (extension_pack->featuresToEnable2)
		{
			chosenDevice = findMostCompatibleDevice(extension_pack, availDevices);
		}
		else
		{
			chosenDevice = chooseIdealDevice(availDevices);
		}

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

		return chooseIdealDevice(compatibleDevices);
	}

	VkPhysicalDevice PhysicalDeviceImpl::chooseIdealDevice(const std::vector<VkPhysicalDevice>& avail_devices)
	{
		size_t bestDeviceIdx = 0;
		size_t bestScore = 0;

		for (size_t i = 0; i < avail_devices.size(); ++i)
		{

		}

		return avail_devices[bestDeviceIdx];
	}

}
