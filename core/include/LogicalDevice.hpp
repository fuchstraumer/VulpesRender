#pragma once
#ifndef VULPES_VK_LOGICAL_DEVICE_H
#define VULPES_VK_LOGICAL_DEVICE_H
#include "vpr_stdafx.h"
#include "ForwardDecl.hpp"

namespace vpr
{

    struct DeviceDataMembers;
    struct VkDebugUtilsFunctions;
    
    /**Simple wrapper struct for storing queue family indices. Retrieved from Device instance.
     * \ingroup Core
     */
    struct VPR_API vkQueueFamilyIndices
    {
        vkQueueFamilyIndices();
        uint32_t Graphics;
        uint32_t Compute;
        uint32_t Transfer;
        uint32_t SparseBinding;
        /**On some hardware, there may be a unique presentation queue. In that case, this will be the index of that queue.*/
        uint32_t Present;
    };

    /**!The Device class is a wrapper around the vkLogicalDevice object. This object is what most Vulkan resources and objects are spawned from,
    *   and is then responsible for managing these resources. Most Vulkan functions relating to resource creation, binding, or updating will take
    *   a VkDevice reference as a parameter. This is considered to be a "Logical device" as it represents a non-physical device and each physical
    *   device can actually store and handle multiple logical devices.
    *   
    *   The vast majority of classes in this codebase contain a private const Device pointer, for use in their internal functions requiring this object's
    *   handle. 
    *   \ingroup Core
    */
    class VPR_API Device
    {
        Device(const Device&) = delete;
        Device(Device&&) = delete;
        Device& operator=(const Device&) = delete;
        Device& operator=(Device&&) = delete;
    public:

        /**Constructs a new VkDevice instance. 
         * \param instance Parent instance this device belongs to
         * \param p_device Physical device that this device operators over and on
         * \param VkSurfaceKHR Surface this device will be presenting to, if applicable. Used to verify presentation support for the current configuration.
         * \param extensions Used to decide what device-level extensions should be loaded. Throws if unable to load a required extension. If left to null, will only load
         * what is required for presentation and try to load allocation extensions to improve that system
         * \param layer_names Names of layers to load - If left to null and the parent instance has validation enabled, uses the VK_LAYER_LUNARG_standard_validation set
        */
        Device(const Instance* instance, const PhysicalDevice* p_device, VkSurfaceKHR surface = VK_NULL_HANDLE, const VprExtensionPack* extensions = nullptr, const char* const* layer_names = nullptr, const uint32_t layer_count = 0);
        ~Device();

        const VkDevice& vkHandle() const;
        /**Returns whether or not the currently active physical device, along with the logical device, supports/has queues dedicated compute operations.*/
        bool HasDedicatedComputeQueues() const;
        /**Returns true when the VK_KHR_dedicated_allocation extension and it's cohort has been loaded. Used by memory allocation systems to improve fit and potential performance of certain memory allocations.*/
        bool DedicatedAllocationExtensionsEnabled() const noexcept;
        bool HasExtension(const char* name) const noexcept;
        /**Important note - uses strdup, so the data must be free'd by the user once they are done reading the extensions array!*/
        void GetEnabledExtensions(size_t* num_extensions, char** extensions) const;
        void UpdateSurface(VkSurfaceKHR new_surface);

        /**! Returns queue that has support for most operation types. First checks for graphics, compute, and transfer. 
        *    Then proceeds to graphics and compute. Lastly, it will just return a graphics-only queue and log a warning.
        *   \param idx - in the case of a device with several "generalized" queues, selects which queue to return.
        */
        VkQueue GeneralQueue(const uint32_t idx = 0) const;

        /* Note: While most hardware presents support fpr multiple graphics queues, this is almost certainly not the actual case. 
         * Instead, it is likely the driver is doing some kind of multiplexing of it's singular graphics queue. By default only one
         * graphics queue will be created, as it is not recommended to use more than one anyways.*/
        VkQueue GraphicsQueue(const uint32_t idx = 0) const;
        VkQueue TransferQueue(const uint32_t idx = 0) const;
        VkQueue ComputeQueue(const uint32_t idx = 0) const;
        VkQueue SparseBindingQueue(const uint32_t idx = 0) const;

        /**!Checks whether or not the given format along with the specified flags supports optimal or linear tiling.
        *   \param format - Vulkan format enum specifying the type of image data
        *   \param flags - flags specifying features of format: commonly what it is being used for, e.g cube map, sampled image, storage image, etc
        */
        VkImageTiling GetFormatTiling(const VkFormat format, const VkFormatFeatureFlags flags) const;
        
        /**! Checks a collection of possible formats, returning the one that supports the given tiling and feature flags.
        *   \param options - vector of formats that are usable for the desired task.
        *   \param tiling - required image tiling setting.
        *   \param flags - features required, commonly related to intended use for the image.
        *   \return Returns found format if successful, otherwise returns VK_FORMAT_UNDEFINED and logs a detailed error.
        */
        VkFormat FindSupportedFormat(const VkFormat* formats, const size_t num_formats, const VkImageTiling tiling, const VkFormatFeatureFlags flags) const;
        
        /**Finds a Vulkan image format suitable for use in the depth buffer. Currently could benefit from better quantification of what defines the "best" depth format, as this will depend on hardware and whether or not we are even using the stencil.*/
        VkFormat FindDepthFormat() const;

        /**Returns the index of a memory type satisfying the requirements specified by the given parameters. Returns std::numeric_limits<uint32_t>::max() if the value
         * cannot be found, and writes to memory_type_found
         * \param type_bitfield bitfield retrieved from VkMemoryRequirements
         * \param property_flags required properties that the memory type must support
         * \param memory_type_found written to, if non-null, based on search results
         */
        uint32_t GetMemoryTypeIdx(const uint32_t type_bitfield, const VkMemoryPropertyFlags property_flags, VkBool32* memory_type_found = nullptr) const;
        const PhysicalDevice& GetPhysicalDevice() const noexcept;
        const VkPhysicalDeviceProperties& GetPhysicalDeviceProperties() const noexcept;
        const VkPhysicalDeviceMemoryProperties& GetPhysicalDeviceMemoryProperties() const noexcept;
        /**Used to retrieve structure of debug utils function pointers. */
        const VkDebugUtilsFunctions& DebugUtilsHandler() const;
        
        /*This will be 1, the vast majority of the time. Currently, having more than one graphics queue is effectively unsupported: no hardware actually implements this capability so I didn't bother allowing it to be supported. */
        const uint32_t& NumGraphicsQueues() const noexcept;
        const uint32_t& NumComputeQueues() const noexcept;
        const uint32_t& NumTransferQueues() const noexcept;
        const uint32_t& NumSparseBindingQueues() const noexcept;
        const vkQueueFamilyIndices& QueueFamilyIndices() const noexcept;

        const Instance* ParentInstance() const noexcept;

    private:
        void verifyPresentationSupport();        
        void checkSurfaceSupport(const VkSurfaceKHR& surf);
        VkDeviceQueueCreateInfo setupQueueFamily(const VkQueueFamilyProperties& family_properties);
        void create(const VprExtensionPack* extensions, const char* const* layers, const uint32_t layer_count);
        void setupQueues();
        void setupValidation(const char* const* layers, const uint32_t layer_count);
        void setupExtensions(const VprExtensionPack* extensions);
        void setupGraphicsQueues();
        void setupComputeQueues();
        void setupTransferQueues();
        void setupSparseBindingQueues();
        void setupDebugUtilsHandler();
    
        uint32_t numGraphicsQueues{ 0 };
        uint32_t numComputeQueues{ 0 };
        uint32_t numTransferQueues{ 0 };
        uint32_t numSparseBindingQueues{ 0 };
        vkQueueFamilyIndices queueFamilyIndices;
        VkDevice handle{ VK_NULL_HANDLE };
        VkDeviceCreateInfo createInfo{ };
        VkDebugUtilsFunctions* debugUtilsHandler{ nullptr };
        const PhysicalDevice* parent{ nullptr };
        const Instance* parentInstance{ nullptr };
        VkSurfaceKHR surface;
        mutable DeviceDataMembers* dataMembers;
    };

}

#endif // !VULPES_VK_LOGICAL_DEVICE_H
