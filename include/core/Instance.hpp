#pragma once
#ifndef VULPES_VK_INSTANCE_H
#define VULPES_VK_INSTANCE_H

#include "vpr_stdafx.h"
#include "ForwardDecl.hpp"

namespace vpr {

    /** The Core group handles the base Vulkan resources and objects: LogicalDevice, PhysicalDevice, Instance, and Window. It also 
    *   includes the InputHandler class, which is responsible for handling input events and updats from the Window class.
    * \defgroup Core
    */

    /**The VprExtensionPack structure makes requesting extensions in the device and instance constructors easier and more robust,
     * by allowing the specification of required and optional extensions. The inability or failure to enable a required extension
     * will cause an exception ot be thrown, while issues with optional extensions will log the extension unable (at a WARNING level)
     * to be enabled or used - then it will be removed from the list of extensions submitted to the constructor.
     * \ingroup Core
     */
    struct VprExtensionPack {
        const char* const* RequiredExtensionNames;
        uint32_t RequiredExtensionCount;
        const char* const* OptionalExtensionNames;
        uint32_t OptionalExtensionCount;
    };

    /** Instance is a wrapper around the base Vulkan object that must be initialized first. The VkInstanceCreateInfo struct passed to the constructor
    *    contains information about the current layers enabled, and which Vulkan Instance extensions to enable. By default, this should/will contain
    *    extensions required to support the creation of a window surface (VkSurfaceKHR).
    *
    *    This class also contains a Window object, which it is responsible for creating and destroying as necessary, along with a PhysicalDevice object.
    *    These can both be retrieved through the relevant methods.
    *    \ingroup Core
    */
    class VPR_API Instance  {
        Instance(const Instance &) = delete;
        Instance& operator=(const Instance &) = delete;
    public:
        
        Instance(bool enable_validation, const VkApplicationInfo* info, GLFWwindow* window);
        Instance(bool enable_validation, const VkApplicationInfo* info, GLFWwindow* window, const VprExtensionPack* extensions, const char* const* layers = nullptr, const uint32_t layer_count = 0);
        ~Instance();

        const VkInstance& vkHandle() const noexcept;
        const VkSurfaceKHR& vkSurface() const noexcept;
        const PhysicalDevice* GetPhysicalDevice() const noexcept;
        GLFWwindow* GetGLFWwindow() const noexcept;

        void CreateSurfaceKHR();
        void DestroySurfaceKHR();

        const bool& ValidationEnabled() const noexcept;

    private:

        void prepareValidation(const char* const* layers, const uint32_t layer_count);
        bool checkValidationSupport(const char* const* layer_names, const uint32_t layer_count) const;
        void prepareValidationCallbacks();
        void extensionSetup(const VprExtensionPack* extensions);
        void prepareRequiredExtensions(const VprExtensionPack* extensions, std::vector<const char*>& output) const;
        void prepareOptionalExtensions(const VprExtensionPack* extensions, std::vector<const char*>& output) const;
        void extensionCheck(std::vector<const char*>& extensions, bool throw_on_error) const;
        void checkOptionalExtensions(std::vector<const char*>& optional_extensions) const;
        void checkRequiredExtensions(std::vector<const char*>& required_extensions) const;
        void setupPhysicalDevice();
        
        mutable GLFWwindow* window;
        std::unique_ptr<PhysicalDevice> physicalDevice;
        std::unique_ptr<SurfaceKHR> surface;     
        VkInstance handle;
        VkInstanceCreateInfo createInfo;
        bool validationEnabled; 
        std::vector<const char*> enabledExtensions;
        

        VkDebugReportCallbackEXT debugCallback;

    };

    /** Pass a swapchain and instance pointer to this to have the swapchain and surface destroyed and recreated
    *   in the proper order. If done incorrectly, the validation layers will give you errors about a surface being 
    *   destroyed before it's swapchain is (in the best case), or crash in the worst case
    */
    void RecreateSwapchainAndSurface(Instance* instance, Swapchain* swap);

    VKAPI_ATTR VkBool32 VKAPI_CALL VkDebugCallbackFn(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT type, uint64_t object_handle, size_t location, int32_t message_code, const char* layer_prefix,
        const char* message, void* user_data);
}

#endif // !INSTANCE_H
