#pragma once
#ifndef VULPES_VK_INSTANCE_H
#define VULPES_VK_INSTANCE_H
#include "vpr_stdafx.h"
#include "ForwardDecl.hpp"
#include <memory>

namespace vpr {

    struct InstanceExtensionHandler;

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
    struct VPR_API VprExtensionPack {
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
    
        enum class instance_layers : uint32_t {
            Disabled,
            Minimal,
            Full,
        };
        
        Instance(instance_layers layers, const VkApplicationInfo* info, GLFWwindow* window);
        Instance(instance_layers layers_flags, const VkApplicationInfo* info, GLFWwindow* window, const VprExtensionPack* extensions, 
            const char* const* layers = nullptr, const uint32_t layer_count = 0);
        ~Instance();

        const VkInstance& vkHandle() const noexcept;
        const VkSurfaceKHR& vkSurface() const noexcept;
        const PhysicalDevice* GetPhysicalDevice() const noexcept;
        GLFWwindow* GetGLFWwindow() const noexcept;
        void DebugMessage(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT obj_type, uint64_t obj, size_t location, int32_t msg_code, const char* layer, const char* message);

        void CreateSurfaceKHR();
        void DestroySurfaceKHR();

        bool ValidationEnabled() const noexcept;

    private:

        void prepareValidation(const char* const* layers, const uint32_t layer_count);
        bool checkValidationSupport(const char* const* layer_names, const uint32_t layer_count) const;
        void checkApiVersionSupport(VkApplicationInfo* info);
        void prepareValidationCallbacks();
        void setupPhysicalDevice();
        
        mutable GLFWwindow* window;
        std::unique_ptr<PhysicalDevice> physicalDevice;
        std::unique_ptr<SurfaceKHR> surface;
        std::unique_ptr<InstanceExtensionHandler> extensionHandler;
        VkInstance handle;
        VkInstanceCreateInfo createInfo;
        VkDebugReportCallbackEXT debugCallback;
        PFN_vkDebugReportMessageEXT debugMessageFn{ nullptr };
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