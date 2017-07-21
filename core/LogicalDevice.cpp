#include "stdafx.h"
#include "LogicalDevice.h"
#include "PhysicalDevice.h"
#include "Instance.h"

namespace vulpes {

	Device::Device(const Instance* instance, const PhysicalDevice * device) : parent(device), parentInstance(instance) {

		SetupGraphicsQueues();
		SetupComputeQueues();
		SetupTransferQueues();
		SetupSparseBindingQueues();

		// Local vector copy of QueueCreateInfos: need raw data ptr for createInfo,
		// and easier to insert/include the presentation queues this way.
		std::vector<VkDeviceQueueCreateInfo> queue_infos;
		for (const auto& queue_info_entry : queueInfos) {
			queue_infos.push_back(queue_info_entry.second);
		}

		VerifyPresentationSupport();
		
		createInfo = vk_device_create_info_base;
		createInfo.queueCreateInfoCount = static_cast<uint32_t>(queue_infos.size());
		createInfo.pQueueCreateInfos = queue_infos.data();
		createInfo.pEnabledFeatures = &device->Features;

		if (parentInstance->validationEnabled) {
			EnableValidation();
		}

		if (MarkersEnabled) {
			createInfo.enabledLayerCount = 1;
			createInfo.ppEnabledLayerNames = &standard_validation_layer;
			createInfo.enabledExtensionCount = 2;
			createInfo.ppEnabledExtensionNames = device_extensions_debug.data();
		}
		else {
			createInfo.enabledExtensionCount = 1;
			createInfo.ppEnabledExtensionNames = device_extensions.data();
			createInfo.enabledLayerCount = 1;
			createInfo.ppEnabledLayerNames = &standard_validation_layer;
		}

		VkResult result = vkCreateDevice(parent->vkHandle(), &createInfo, AllocCallbacks, &handle);
		VkAssert(result);

		if (MarkersEnabled) {
			GetMarkerFuncPtrs();
		}

		vkAllocator = std::make_unique<Allocator>(this);

	}

	void Device::SetupGraphicsQueues() {
		QueueFamilyIndices.Graphics = parent->GetQueueFamilyIndex(VK_QUEUE_GRAPHICS_BIT);
		auto create_info = SetupQueueFamily(parent->GetQueueFamilyProperties(VK_QUEUE_GRAPHICS_BIT));
		create_info.queueFamilyIndex = QueueFamilyIndices.Graphics;
		NumGraphicsQueues = create_info.queueCount;
		queueInfos.insert(std::make_pair(VK_QUEUE_GRAPHICS_BIT, create_info));
	}

	void Device::SetupComputeQueues() {
		QueueFamilyIndices.Compute = parent->GetQueueFamilyIndex(VK_QUEUE_COMPUTE_BIT);
		if (QueueFamilyIndices.Compute != QueueFamilyIndices.Graphics) {
			auto compute_info = SetupQueueFamily(parent->GetQueueFamilyProperties(VK_QUEUE_COMPUTE_BIT));
			compute_info.queueFamilyIndex = QueueFamilyIndices.Compute;
			NumComputeQueues = compute_info.queueCount;
			queueInfos.insert(std::make_pair(VK_QUEUE_COMPUTE_BIT, compute_info));
		}
		else {
			auto queue_properties = parent->GetQueueFamilyProperties(VK_QUEUE_COMPUTE_BIT);
			if (queue_properties.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
				QueueFamilyIndices.Compute = QueueFamilyIndices.Graphics;
				NumComputeQueues = NumGraphicsQueues;
			}
		}
	}

	void Device::SetupTransferQueues() {
		QueueFamilyIndices.Transfer = parent->GetQueueFamilyIndex(VK_QUEUE_TRANSFER_BIT);
		if (QueueFamilyIndices.Transfer != QueueFamilyIndices.Graphics) {
			auto transfer_info = SetupQueueFamily(parent->GetQueueFamilyProperties(VK_QUEUE_TRANSFER_BIT));
			transfer_info.queueFamilyIndex = QueueFamilyIndices.Transfer;
			NumTransferQueues = transfer_info.queueCount;
			queueInfos.insert(std::make_pair(VK_QUEUE_TRANSFER_BIT, transfer_info));
		}
		else {
			auto queue_properties = parent->GetQueueFamilyProperties(VK_QUEUE_TRANSFER_BIT);
			if (queue_properties.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
				QueueFamilyIndices.Transfer = QueueFamilyIndices.Graphics;
				NumTransferQueues = NumGraphicsQueues;
			}
		}
	}

	void Device::SetupSparseBindingQueues() {
		QueueFamilyIndices.SparseBinding = parent->GetQueueFamilyIndex(VK_QUEUE_SPARSE_BINDING_BIT);
		if (QueueFamilyIndices.SparseBinding != QueueFamilyIndices.Graphics) {
			auto sparse_info = SetupQueueFamily(parent->GetQueueFamilyProperties(VK_QUEUE_SPARSE_BINDING_BIT));
			sparse_info.queueFamilyIndex = QueueFamilyIndices.SparseBinding;
			NumSparseBindingQueues = sparse_info.queueCount;
		}
		else {
			auto queue_properties = parent->GetQueueFamilyProperties(VK_QUEUE_SPARSE_BINDING_BIT);
			if (queue_properties.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
				QueueFamilyIndices.SparseBinding = QueueFamilyIndices.Graphics;
				NumSparseBindingQueues = NumGraphicsQueues;
			}
		}
	}

	void Device::VerifyPresentationSupport() {
	
		// Check presentation support
		VkBool32 present_support = false;
		for (uint32_t i = 0; i < 3; ++i) {
			vkGetPhysicalDeviceSurfaceSupportKHR(parent->vkHandle(), i, parentInstance->GetSurface(), &present_support);
			if (present_support) {
				QueueFamilyIndices.Present = i;
				break;
			}
		}

		if (!present_support) {
			LOG(ERROR) << "No queues found that support presentation to a surface.";
			throw std::runtime_error("No queues found that support presentation to a surface!");
		}

	}

	VkDeviceQueueCreateInfo Device::SetupQueueFamily(const VkQueueFamilyProperties & family_properties) {
		VkDeviceQueueCreateInfo result = vk_device_queue_create_info_base;
		result.queueCount = family_properties.queueCount;
		std::vector<float> queue_priorities;
		queue_priorities.assign(result.queueCount, 1.0f);
		result.pQueuePriorities = std::move(queue_priorities.data());
		return result;
	}

	Device::~Device(){
		vkDestroyDevice(handle, AllocCallbacks);
	}

	const VkDevice & Device::vkHandle() const{
		return handle;
	}

	void Device::CheckSurfaceSupport(const VkSurfaceKHR& surf) {
		VkBool32 supported;
		vkGetPhysicalDeviceSurfaceSupportKHR(parent->vkHandle(), QueueFamilyIndices.Present, surf, &supported);
		assert(supported);
	}

	VkQueue Device::GraphicsQueue(const uint32_t & idx) const{
		assert(idx < NumGraphicsQueues);
		VkQueue result;
		vkGetDeviceQueue(vkHandle(), QueueFamilyIndices.Graphics, idx, &result);
		return result;
	}

	VkQueue Device::TransferQueue(const uint32_t & idx) const{

		LOG_IF(QueueFamilyIndices.Compute == QueueFamilyIndices.Graphics, INFO) << "Retrieving queue that supports transfer ops, but isn't dedicated transfer queue.";

		assert(idx < NumTransferQueues);
		VkQueue result;
		vkGetDeviceQueue(vkHandle(), QueueFamilyIndices.Transfer, idx, &result);
		assert(result != VK_NULL_HANDLE);
		return result;

	}

	VkQueue Device::ComputeQueue(const uint32_t & idx) const{

		LOG_IF(QueueFamilyIndices.Compute == QueueFamilyIndices.Graphics, INFO) << "Retrieving queue that supports compute ops, but isn't dedicated compute queue.";
		assert(idx < NumComputeQueues);
		VkQueue result;
		vkGetDeviceQueue(vkHandle(), QueueFamilyIndices.Compute, idx, &result);
		return result;

	}

	VkQueue Device::SparseBindingQueue(const uint32_t & idx) const {
		
		if (!queueInfos.count(VK_QUEUE_SPARSE_BINDING_BIT)) {
			LOG(ERROR) << "Current device does not support sparse binding queues!";
			throw std::runtime_error("Requested unsuported queue family (Sparse Binding)");
		}

		LOG_IF(QueueFamilyIndices.Compute == QueueFamilyIndices.Graphics, INFO) << "Retrieving queue that supports sparse binding, but isn't dedicated sparse binding queue.";

		assert(idx < NumSparseBindingQueues);
		VkQueue result;
		vkGetDeviceQueue(handle, QueueFamilyIndices.SparseBinding, idx, &result);
		return result;

	}

	VkImageTiling Device::GetFormatTiling(const VkFormat & format, const VkFormatFeatureFlags & flags) const {
		VkFormatProperties properties;
		vkGetPhysicalDeviceFormatProperties(parent->vkHandle(), format, &properties);
		if (properties.optimalTilingFeatures & flags) {
			return VK_IMAGE_TILING_OPTIMAL;
		}
		else {
			// Check that the device at least supports the desired features for linear tiling
			assert(properties.linearTilingFeatures & flags);
			return VK_IMAGE_TILING_LINEAR;
		}
	}

	VkFormat Device::FindSupportedFormat(const std::vector<VkFormat>& options, const VkImageTiling & tiling, const VkFormatFeatureFlags & flags) const {
		for (const auto& fmt : options) {
			VkFormatProperties properties;
			vkGetPhysicalDeviceFormatProperties(parent->vkHandle(), fmt, &properties);
			if (tiling == VK_IMAGE_TILING_LINEAR && (properties.linearTilingFeatures & flags) == flags) {
				return fmt;
			}
			else if (tiling == VK_IMAGE_TILING_OPTIMAL && (properties.optimalTilingFeatures & flags) == flags) {
				return fmt;
			}
		}
		throw(std::runtime_error("Failed to find a supported format!"));
	}

	VkFormat Device::FindDepthFormat() const{
		return FindSupportedFormat({ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT }, VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
	}

	uint32_t Device::GetMemoryTypeIdx(const uint32_t & type_bitfield, const VkMemoryPropertyFlags & property_flags, VkBool32 * memory_type_found) const{
		return parent->GetMemoryTypeIdx(type_bitfield, property_flags, memory_type_found);
	}

	uint32_t Device::GetPhysicalDeviceID() const noexcept{
		return parent->Properties.deviceID;
	}

	const PhysicalDevice & Device::GetPhysicalDevice() const noexcept{
		return *parent;
	}

	bool Device::HasDedicatedComputeQueues() const {
		if (QueueFamilyIndices.Compute != QueueFamilyIndices.Graphics) {
			return true;
		}
		return false;
	}

	void Device::EnableValidation() {

		std::vector<VkExtensionProperties> extensions;
		uint32_t cnt = 0;
		vkEnumerateDeviceExtensionProperties(parent->vkHandle(), nullptr, &cnt, nullptr);
		extensions.resize(cnt);
		vkEnumerateDeviceExtensionProperties(parent->vkHandle(), nullptr, &cnt, extensions.data());
		for (auto& ext : extensions) {
			if (!strcmp(ext.extensionName, VK_EXT_DEBUG_MARKER_EXTENSION_NAME)) {
				MarkersEnabled = true;
				std::cerr << "Markers enabled\n";
			}
			else {
				MarkersEnabled = false;
				std::cerr << "Markers disabled\n";
			}
		}

	}

	void Device::GetMarkerFuncPtrs() {

		pfnDebugMarkerSetObjectTag = reinterpret_cast<PFN_vkDebugMarkerSetObjectTagEXT>(vkGetDeviceProcAddr(handle, "vkDebugMarkerSetObjectTagEXT"));
		pfnDebugMarkerSetObjectName = reinterpret_cast<PFN_vkDebugMarkerSetObjectNameEXT>(vkGetDeviceProcAddr(handle, "vkDebugMarkerSetObjectNameEXT"));
		pfnCmdDebugMarkerBegin = reinterpret_cast<PFN_vkCmdDebugMarkerBeginEXT>(vkGetDeviceProcAddr(handle, "vkCmdDebugMarkerBeginEXT"));
		pfnCmdDebugMarkerEnd = reinterpret_cast<PFN_vkCmdDebugMarkerEndEXT>(vkGetDeviceProcAddr(handle, "vkCmdDebugMarkerEndEXT"));
		pfnCmdDebugMarkerInsert = reinterpret_cast<PFN_vkCmdDebugMarkerInsertEXT>(vkGetDeviceProcAddr(handle, "vkCmdDebugMarkerInsertEXT"));
	
	}

	VkQueue Device::GeneralQueue(const uint32_t & desired_idx) const {
		
		uint32_t idx = 0;

		for (uint32_t i = 0; i < static_cast<uint32_t>(parent->QueueFamilyProperties.size()); ++i) {
			if ((parent->QueueFamilyProperties[i].queueFlags | VK_QUEUE_TRANSFER_BIT) && (parent->QueueFamilyProperties[i].queueFlags | VK_QUEUE_GRAPHICS_BIT)) {
				idx = i;
				break;
			}
		}

		VkQueue result;
		vkGetDeviceQueue(vkHandle(), idx, desired_idx, &result);
		return result;

	}

	void Device::vkSetObjectDebugMarkerName(const uint64_t & object_handle, const VkDebugReportObjectTypeEXT & object_type, const char * name) const {
		
		if (pfnDebugMarkerSetObjectName) {
			VkDebugMarkerObjectNameInfoEXT name_info = vk_debug_marker_object_name_info_base;
			name_info.objectType = object_type;
			name_info.object = object_handle;
			name_info.pObjectName = name;
			VkResult result = pfnDebugMarkerSetObjectName(handle, &name_info);
			VkAssert(result);
		}

	}

	void Device::vkSetObjectDebugMarkerTag(const uint64_t & object_handle, const VkDebugReportObjectTypeEXT & object_type, uint64_t name, size_t tag_size, const void * tag) const {

		if (pfnDebugMarkerSetObjectTag) {
			VkDebugMarkerObjectTagInfoEXT tag_info{ VK_STRUCTURE_TYPE_DEBUG_MARKER_OBJECT_TAG_INFO_EXT, nullptr };
			tag_info.objectType = object_type;
			tag_info.object = object_handle;
			tag_info.tagName = name;
			tag_info.tagSize = tag_size;
			tag_info.pTag = tag;
			pfnDebugMarkerSetObjectTag(handle, &tag_info);
		}

	}

	void Device::vkCmdBeginDebugMarkerRegion(VkCommandBuffer & cmd, const char * region_name, const glm::vec4 & region_color) const {
		
		if (pfnCmdDebugMarkerBegin) {
			VkDebugMarkerMarkerInfoEXT marker_info{ VK_STRUCTURE_TYPE_DEBUG_MARKER_MARKER_INFO_EXT };
			memcpy(marker_info.color, glm::value_ptr(region_color), sizeof(float) * 4);
			marker_info.pMarkerName = region_name;
			pfnCmdDebugMarkerBegin(cmd, &marker_info);
		}

	}

	void Device::vkCmdInsertDebugMarker(VkCommandBuffer & cmd, const char * marker_name, const glm::vec4 & marker_color) const {
		
		if (pfnCmdDebugMarkerInsert) {
			VkDebugMarkerMarkerInfoEXT marker_info{ VK_STRUCTURE_TYPE_DEBUG_MARKER_MARKER_INFO_EXT };
			memcpy(marker_info.color, glm::value_ptr(marker_color), sizeof(float) * 4);
			marker_info.pMarkerName = marker_name;
			pfnCmdDebugMarkerInsert(cmd, &marker_info);
		}

	}

	void Device::vkCmdEndDebugMarkerRegion(VkCommandBuffer & cmd) const {
		
		if (pfnCmdDebugMarkerEnd) {
			pfnCmdDebugMarkerEnd(cmd);
		}

	}

}
