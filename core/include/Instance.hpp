#pragma once
#ifndef VULPES_VK_INSTANCE_H
#define VULPES_VK_INSTANCE_H
#include "vpr_stdafx.h"
#include "ForwardDecl.hpp"

namespace vpr
{

    struct InstanceExtensionHandler;


    /** The Core group handles the base Vulkan resources and objects: LogicalDevice, PhysicalDevice, Instance, and Window. It also 
    *   includes the InputHandler class, which is responsible for handling input events and updats from the Window class.
    * \defgroup Core
    */

    /**The VprExtensionPack structure makes requesting extensions in the device and instance constructors easier and more robust,
     * by allowing the specification of required and optional extensions. The inability or failure to enable a required extension
     * will cause an exception to be thrown, while issues with optional extensions will log the extension unable (at a WARNING level)
     * to be enabled or used - then it will be removed from the list of extensions submitted to the constructor.
     * \ingroup Core
     */
    struct VPR_API VprExtensionPack
    {
        /**These extension names specify what must be loaded - failure to do so results in an exception*/
        const char* const* RequiredExtensionNames;
        uint32_t RequiredExtensionCount;
        /**These are "nice to have" extensions that will not compromise required functionality, so failing to load them is a non-issue.*/
        const char* const* OptionalExtensionNames;
        uint32_t OptionalExtensionCount;
        // Added to device pNext, assuming user has already set pNext after first pointer as needed
        const void* pNextChainStart{ nullptr };
        // If not nullptr, will be casted to the DeviceFeatures/PhysicalDeviceFeatures struct and used to overwrite it upon
        // creation, so that users can pass in their own features to enable
        const VkPhysicalDeviceFeatures* featuresToEnable{ nullptr };
    };

    /**Instance is a wrapper around the base Vulkan object that must be initialized first. The VkInstanceCreateInfo struct passed to the constructor
    * contains information about the current layers enabled, and which Vulkan Instance extensions to enable. By default, this should/will contain
    * extensions required to support the creation of a window surface (done by calling a glfw/SDL function to retrieve required extensions on 
    * glfw/SDL-enabled platforms)
    *  \ingroup Core
    */
    class VPR_API Instance 
    {
        Instance(const Instance&) = delete;
        Instance& operator=(const Instance&) = delete;
    public:

        /**Currently both minimal and full layers are the same: need some time to better decide 
         * what the "minimal" layer setup could look like.
         */
        enum class instance_layers : uint32_t
        {
            Disabled,
            Minimal,
            Full,
        };
        
        /**Sets the layer status as specified, and changes the minor state/informational info attached to the instance as specified by the info parameter */
        Instance(instance_layers layers, const VkApplicationInfo* info);
        /**Uses an extensions list, compared to the other constructor that simply uses what glfw/SDL says is required.*/
        Instance(instance_layers layers_flags, const VkApplicationInfo* info, const VprExtensionPack* extensions, 
            const char* const* layers = nullptr, const uint32_t layer_count = 0);
        ~Instance();

        const VkInstance& vkHandle() const noexcept;
        bool ValidationEnabled() const noexcept;
        /**For the best chance of matching extension names properly, use the Vulkan macros for extension names.*/
        bool HasExtension(const char* extension_name) const noexcept;
        /**The extensions parameter is set using strdup, so the user MUST be sure to free this memory when finished reading it.*/
        void GetEnabledExtensions(size_t* num_extensions, char** extensions) const;

        const VkApplicationInfo& ApplicationInfo() const noexcept;

    private:

        void prepareValidation(const char* const* layers, const uint32_t layer_count);
        bool checkValidationSupport(const char* const* layer_names, const uint32_t layer_count) const;
        void checkApiVersionSupport(VkApplicationInfo* info);
        
        InstanceExtensionHandler* extensionHandler;
        VkInstance handle;
        VkInstanceCreateInfo createInfo;
        VkApplicationInfo applicationInfo;

    };

    /**Pass an easyloggingpp logging repository pointer into this function, and it will be set as
     * the repository for this module to use. That way, all log messages from all modules (even 
     * when using this as a shared library) will go to the same sinks
     * \ingroup Core
     */
    VPR_API void SetLoggingRepository_VprCore(void* logging_repo);
    VPR_API void* GetLoggingRepository_VprCore();

}

#endif // !INSTANCE_H
