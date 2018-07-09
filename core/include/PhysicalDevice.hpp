#pragma once
#ifndef VULPES_VK_PHYSICAL_DEVICE_H
#define VULPES_VK_PHYSICAL_DEVICE_H
#include "vpr_stdafx.h"
#include <memory>

namespace vpr {

    class PhysicalDeviceImpl;

    /**! PhysicalDevice is a wrapper around a VkPhysicalDevice object, which is itself merely a handle representing a Vulkan-compatible
    *    hardware device in a user's system. This class stores the relevant VkPhysicalDeviceProperties, VkPhysicalDeviceFeatures, and 
    *    VkPhysicalDeviceMemoryProperties that can be freely queried from anywhere in the program. This can/should be used to check for
    *    limits on things like texture and buffer size, supported memory types, supported rendering modes, and supported texture types like cubemaps.
    *
    *    As each computer can have multiple physical devices, the class uses a simplistic scoring system to find the "best" hardware on the current system.
    *    \ingroup Core
    */
    class VPR_API PhysicalDevice {
        PhysicalDevice(const PhysicalDevice& other) = delete;
        PhysicalDevice& operator=(const PhysicalDevice& other) = delete;
    public:

        PhysicalDevice(const VkInstance& instance_handle);
        PhysicalDevice(PhysicalDevice&& other) noexcept;
        PhysicalDevice& operator=(PhysicalDevice&& other) noexcept;
        ~PhysicalDevice();
        
        const VkPhysicalDevice& vkHandle() const noexcept;

        /**! Attempts to find the hardware-appropriate index of a memory type that meets the flags given.
            \param type_bitfield - the memoryTypeBits field of a VkMemoryRequirements struct, retrieved from a 
                    vkGetImageMemoryRequirements/vkGetBufferMemoryRequirements call.
            \param property_flags - the type of memory requested by the user, commonly device-local or host-coherent memory.
            \return Index of the requested memory type on success, std::numeric_limits<uint32_t>::max() on failure.
        */
        uint32_t GetMemoryTypeIdx(const uint32_t& type_bitfield, const VkMemoryPropertyFlags& property_flags, VkBool32* memory_type_found = nullptr) const noexcept;
        
        /**! Attempts to find a Queue family that supports the full bitfield given: this can be multiple types, so graphics + compute options or compute + transfer
        *    bitfields can be passed to the method.
            \return Index of the queue meeting all of the flags specified, or std::numeric_limits<uint32_t>::max() on failure.
        */
        uint32_t GetQueueFamilyIndex(const VkQueueFlagBits& bitfield) const noexcept ;
        VkQueueFamilyProperties GetQueueFamilyProperties(const VkQueueFlagBits& bitfield) const;
        const VkPhysicalDeviceProperties& GetProperties() const noexcept;
        const VkPhysicalDeviceFeatures& GetFeatures() const noexcept;
        const VkPhysicalDeviceMemoryProperties& GetMemoryProperties() const noexcept;
        const VkPhysicalDeviceSubgroupProperties& GetSubgroupProperties() const noexcept;

    private:

        std::unique_ptr<PhysicalDeviceImpl> impl;
    };

}
#endif // !VULPES_VK_PHYSICAL_DEVICE_H
