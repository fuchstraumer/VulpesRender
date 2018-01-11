#pragma once
#ifndef VULPES_VK_LOGICAL_DEVICE_H
#define VULPES_VK_LOGICAL_DEVICE_H
#include "vpr_stdafx.h"
#include "ForwardDecl.hpp"
#include <map>

namespace vpr {

    struct VPR_API vkQueueFamilyIndices {
        // indices into queue families.
        uint32_t Graphics = std::numeric_limits<uint32_t>::max(), 
                 Compute = std::numeric_limits<uint32_t>::max(), 
                 Transfer = std::numeric_limits<uint32_t>::max(), 
                 SparseBinding = std::numeric_limits<uint32_t>::max(), 
                 Present = std::numeric_limits<uint32_t>::max();
    };

    /**!The Device class is a wrapper around the vkLogicalDevice object. This object is what most Vulkan resources and objects are spawned from,
    *   and is then responsible for managing these resources. Most Vulkan functions relating to resource creation, binding, or updating will take
    *   a VkDevice reference as a parameter. This is considered to be a "Logical Device" as it represents a non-physical device and each physical
    *   device can actually store and handle multiple logical devices.
    *   
    *   The vast majority of classes in this codebase contain a private const Device pointer, for use in their internal functions requiring this object's
    *   handle. 
    *   \ingroup Core
    */
    class VPR_API Device {
        Device(const Device&) = delete;
        Device(Device&&) = delete;
        Device& operator=(const Device&) = delete;
        Device& operator=(Device&&) = delete;
    public:
        
        Device(const Instance* instance, const PhysicalDevice* device);
        Device(const Instance* instance, const PhysicalDevice* p_device, const uint32_t extension_count, const char* const* extension_names,
            const uint32_t layer_count = std::numeric_limits<uint32_t>::max(), const char* const* layer_names = nullptr);

        void VerifyPresentationSupport();

        VkDeviceQueueCreateInfo SetupQueueFamily(const VkQueueFamilyProperties& family_properties);

        ~Device();

        const VkDevice& vkHandle() const;

        void CheckSurfaceSupport(const VkSurfaceKHR& surf);

        /**! Returns whether or not the currently active physical device, along with the logical device, supports/has queues dedicated compute operations */
        bool HasDedicatedComputeQueues() const;

        /**! Returns queue that has support for most operation types. First checks for graphics, compute, and transfer. 
        *    Then proceeds to graphics and compute. Lastly, it will just return a graphics-only queue and log a warning.
        *   \param idx - in the case of a device with several "generalized" queues, selects which queue to return.
        */
        VkQueue GeneralQueue(const uint32_t& idx = 0) const;

        // Attempts to find queue that only does requested operation first, then returns omni-purpose queues.
        VkQueue GraphicsQueue(const uint32_t & idx = 0) const;
        VkQueue TransferQueue(const uint32_t & idx = 0) const;
        VkQueue ComputeQueue(const uint32_t & idx = 0) const;
        VkQueue SparseBindingQueue(const uint32_t& idx = 0) const;

        /**! Checks whether or not the given format along with the specified flags supports optimal or linear tiling.
        *   \param format - Vulkan format enum specifying the type of image data
        *   \param flags - flags specifying features of format: commonly what it is being used for, e.g cube map, sampled image, storage image, etc
        */
        VkImageTiling GetFormatTiling(const VkFormat& format, const VkFormatFeatureFlags & flags) const;
        
        /**! Checks a collection of possible formats, returning the one that supports the given tiling and feature flags.
        *   \param options - vector of formats that are usable for the desired task.
        *   \param tiling - required image tiling setting.
        *   \param flags - features required, commonly related to intended use for the image.
        *   \return Returns found format if successful, otherwise returns VK_FORMAT_UNDEFINED and logs a detailed error.
        */
        VkFormat FindSupportedFormat(const std::vector<VkFormat>& options, const VkImageTiling& tiling, const VkFormatFeatureFlags& flags) const;
        
        /**! Finds a Vulkan image format suitable for use in the depth buffer

        */
        VkFormat FindDepthFormat() const;
        VkFormat GetSwapchainColorFormat() const;

        /*
            Methods related to physical device
        */
        uint32_t GetMemoryTypeIdx(const uint32_t& type_bitfield, const VkMemoryPropertyFlags& property_flags, VkBool32* memory_type_found = nullptr) const;
        uint32_t GetPhysicalDeviceID() const noexcept;
        const PhysicalDevice& GetPhysicalDevice() const noexcept;
        VkPhysicalDeviceProperties GetPhysicalDeviceProperties() const noexcept;
        VkPhysicalDeviceMemoryProperties GetPhysicalDeviceMemoryProperties() const noexcept;
        
        uint32_t NumGraphicsQueues = 0, NumComputeQueues = 0, NumTransferQueues = 0, NumSparseBindingQueues = 0;
        vkQueueFamilyIndices QueueFamilyIndices;
        std::unique_ptr<Allocator> vkAllocator;

    private:

        void setupGraphicsQueues();
        void setupComputeQueues();
        void setupTransferQueues();
        void setupSparseBindingQueues();
        
        void checkRequestedExtensions(std::vector<const char*>& requested_extensions);
        VkDevice handle;
        VkDeviceCreateInfo createInfo;

        const PhysicalDevice* parent;
        const Instance* parentInstance;

        std::map<VkQueueFlags, VkDeviceQueueCreateInfo> queueInfos;

    };



}

#endif // !VULPES_VK_LOGICAL_DEVICE_H
