#pragma once
#ifndef VULPES_VK_PHYSICAL_DEVICE_H
#define VULPES_VK_PHYSICAL_DEVICE_H
#include "vpr_stdafx.h"
#include <memory>

namespace vpr
{

    struct PhysicalDeviceImpl;

    /**! PhysicalDevice is a wrapper around a VkPhysicalDevice object, which is itself merely a handle representing a Vulkan-compatible
    *    hardware device in a user's system. This class stores the relevant VkPhysicalDeviceProperties, VkPhysicalDeviceFeatures, and 
    *    VkPhysicalDeviceMemoryProperties that can be freely queried from anywhere in the program. This can/should be used to check for
    *    limits on things like texture and buffer size, supported memory types, supported rendering modes, and supported texture types like cubemaps.
    *    \ingroup Core
    */
    class VPR_API PhysicalDevice
    {
        PhysicalDevice(const PhysicalDevice& other) = delete;
        PhysicalDevice& operator=(const PhysicalDevice& other) = delete;
    public:

        /**Automated setup - uses the given instance handle to find the "best" available GPU on a system. This will prefer dedicated cards first, and uses 
         * some other parameters to "score" devices from there.
         */
        PhysicalDevice(const VkInstance& instance_handle, const struct VprExtensionPack* extension_pack);
        PhysicalDevice(PhysicalDevice&& other) noexcept;
        PhysicalDevice& operator=(PhysicalDevice&& other) noexcept;
        ~PhysicalDevice();
        
        const VkPhysicalDevice& vkHandle() const noexcept;
        const VkPhysicalDeviceMemoryProperties& MemoryProperties() const noexcept;

        uint32_t GetQueueFamilyIndex(const VkQueueFlagBits bitfield) const noexcept;
        VkQueueFamilyProperties GetQueueFamilyProperties(const VkQueueFlagBits bitfield) const;

    private:
        std::unique_ptr<PhysicalDeviceImpl> impl;
    };

    // Use this if higher level setup functionality needs to scout out devices for supported features etc etc, and then call this 
    // after using VkEnumeratePhysicalDevices there. By doing this, you can find what the best scoring physical device is and thus
    // which one this system will choose - so you can match them up ahead of time, and only query / enable features on the device
    // that we'll end up using :)
    VPR_API VkPhysicalDevice ChooseBestScoringPhysicalDevice(const size_t numDevices, const VkPhysicalDevice* devices);

}
#endif // !VULPES_VK_PHYSICAL_DEVICE_H
