#pragma once
#ifndef VULPES_VK_LOGICAL_DEVICE_H
#define VULPES_VK_LOGICAL_DEVICE_H

#include "stdafx.h"
#include "engine/renderer\ForwardDecl.h"
#include "engine/renderer\NonCopyable.h"
#include "../resource/Allocator.h"
namespace vulpes {

	struct vkQueueFamilyIndices {
		// indices into queue families.
		uint32_t Graphics = std::numeric_limits<uint32_t>::max(), 
				 Compute = std::numeric_limits<uint32_t>::max(), 
				 Transfer = std::numeric_limits<uint32_t>::max(), 
				 SparseBinding = std::numeric_limits<uint32_t>::max(), 
				 Present = std::numeric_limits<uint32_t>::max();
	};

	class Device : public NonMovable {
	public:
		
		Device(const Instance* instance, const PhysicalDevice* device);

		void SetupGraphicsQueues();
		void SetupComputeQueues();
		void SetupTransferQueues();
		void SetupSparseBindingQueues();
		void VerifyPresentationSupport();

		VkDeviceQueueCreateInfo SetupQueueFamily(const VkQueueFamilyProperties& family_properties);

		~Device();

		const VkDevice& vkHandle() const;

		void CheckSurfaceSupport(const VkSurfaceKHR& surf);

		bool HasDedicatedComputeQueues() const;

		void EnableValidation();

		void GetMarkerFuncPtrs();

		// Returns queue that has support for most operation types.
		VkQueue GeneralQueue(const uint32_t& idx = 0) const;

		// Attempts to find queue that only does requested operation first, then returns omni-purpose queues.
		VkQueue GraphicsQueue(const uint32_t & idx = 0) const;
		VkQueue TransferQueue(const uint32_t & idx = 0) const;
		VkQueue ComputeQueue(const uint32_t & idx = 0) const;
		VkQueue SparseBindingQueue(const uint32_t& idx = 0) const;

		/*
			Methods related to supported features and/or formats
		*/
		VkImageTiling GetFormatTiling(const VkFormat& format, const VkFormatFeatureFlags & flags) const;
		VkFormat FindSupportedFormat(const std::vector<VkFormat>& options, const VkImageTiling& tiling, const VkFormatFeatureFlags& flags) const;
		VkFormat FindDepthFormat() const;

		/*
			Methods related to physical device
		*/
		uint32_t GetMemoryTypeIdx(const uint32_t& type_bitfield, const VkMemoryPropertyFlags& property_flags, VkBool32* memory_type_found = nullptr) const;
		uint32_t GetPhysicalDeviceID() const noexcept;
		const PhysicalDevice& GetPhysicalDevice() const noexcept;

		void vkSetObjectDebugMarkerName(const uint64_t& object_handle, const VkDebugReportObjectTypeEXT& object_type, const char* name) const;
		void vkSetObjectDebugMarkerTag(const uint64_t& object_handle, const VkDebugReportObjectTypeEXT& object_type, uint64_t name, size_t tagSize, const void* tag) const;
		void vkCmdBeginDebugMarkerRegion(VkCommandBuffer& cmd, const char* region_name, const glm::vec4& region_color) const;
		void vkCmdInsertDebugMarker(VkCommandBuffer& cmd, const char* marker_name, const glm::vec4& marker_color) const;
		void vkCmdEndDebugMarkerRegion(VkCommandBuffer& cmd) const;
		bool MarkersEnabled;

		uint32_t NumGraphicsQueues = 0, NumComputeQueues = 0, NumTransferQueues = 0, NumSparseBindingQueues = 0;
		vkQueueFamilyIndices QueueFamilyIndices;
		std::vector<const char*> Extensions;

		std::unique_ptr<Allocator> vkAllocator;

	private:

		const VkAllocationCallbacks* AllocCallbacks = nullptr;

		VkDevice handle;
		VkDeviceCreateInfo createInfo;

		const PhysicalDevice* parent;
		const Instance* parentInstance;

		std::map<VkQueueFlags, VkDeviceQueueCreateInfo> queueInfos;

		PFN_vkDebugMarkerSetObjectTagEXT pfnDebugMarkerSetObjectTag;
		PFN_vkDebugMarkerSetObjectNameEXT pfnDebugMarkerSetObjectName;
		PFN_vkCmdDebugMarkerBeginEXT pfnCmdDebugMarkerBegin;
		PFN_vkCmdDebugMarkerEndEXT pfnCmdDebugMarkerEnd;
		PFN_vkCmdDebugMarkerInsertEXT pfnCmdDebugMarkerInsert;
	};

}

#endif // !VULPES_VK_LOGICAL_DEVICE_H
