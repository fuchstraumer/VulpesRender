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
        Instance(bool enable_validation, const VkApplicationInfo* info, GLFWwindow* window, const char** extensions, const uint32_t extension_count, const char** layers = nullptr, const uint32_t layer_count = 0);

        ~Instance();

        const VkInstance& vkHandle() const noexcept;
        const VkSurfaceKHR& vkSurface() const noexcept;
        const PhysicalDevice* GetPhysicalDevice() const noexcept;
        GLFWwindow* GetGLFWwindow() const noexcept;

        void CreateSurfaceKHR();
        void DestroySurfaceKHR();

        const bool& ValidationEnabled() const noexcept;

    private:

        void checkExtensions(std::vector<const char*>& requested_extensions);
        void setupPhysicalDevice();
        
        mutable GLFWwindow* window;
        std::unique_ptr<PhysicalDevice> physicalDevice;
        std::unique_ptr<SurfaceKHR> surface;     
        VkInstance handle;
        VkInstanceCreateInfo createInfo;
        bool validationEnabled; 

        bool checkValidationSupport(const char** layer_names, const uint32_t layer_count);
        void prepareValidation();

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
