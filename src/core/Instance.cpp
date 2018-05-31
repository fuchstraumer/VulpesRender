#include "vpr_stdafx.h"
#include "core/Instance.hpp"
#include "core/PhysicalDevice.hpp"
#include "render/SurfaceKHR.hpp"
#include "render/Swapchain.hpp"
#include "easylogging++.h"
#include "GLFW/glfw3.h"
#include <vulkan/vulkan.h>

namespace vpr {

    Instance::Instance(instance_layers layers, const VkApplicationInfo*info, GLFWwindow* _window) : Instance(layers, info, _window, nullptr) {}

    Instance::Instance(instance_layers layers_flags, const VkApplicationInfo * info, GLFWwindow * _window, const VprExtensionPack* extensions, const char* const* layers, const uint32_t layer_count) :
        window(_window), validationLayers(layers_flags), createInfo{ VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO, nullptr, 0, nullptr } {
        VkApplicationInfo our_info = *info;

        extensionSetup(extensions);
        checkApiVersionSupport(&our_info);
        prepareValidation(layers, layer_count);

        createInfo.pApplicationInfo = &our_info;
        VkResult err = vkCreateInstance(&createInfo, nullptr, &handle);
        VkAssert(err);

        if (validationLayers != instance_layers::Disabled) {
            prepareValidationCallbacks();
        }

        setupPhysicalDevice();
        CreateSurfaceKHR();
    }

    Instance::~Instance() {
        if (debugCallback && (validationLayers != instance_layers::Disabled)) {
            auto func = (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(handle, "vkDestroyDebugReportCallbackEXT");
            func(handle, debugCallback, nullptr);
        }
        DestroySurfaceKHR();
        vkDestroyInstance(handle, nullptr);
    }

    void Instance::CreateSurfaceKHR() {
        LOG_IF(surface, WARNING) << "Attempting to create a SurfaceKHR when one might already exist: if it does, this will probably cause swapchain errors!";
        surface = std::make_unique<SurfaceKHR>(this, window);
    }

    void Instance::DestroySurfaceKHR() {
        surface.reset();
    }

    const bool& Instance::ValidationEnabled() const noexcept {
        return (validationLayers != instance_layers::Disabled);
    }

    const VkInstance& Instance::vkHandle() const noexcept {
        return handle;
    }

    const VkSurfaceKHR& Instance::vkSurface() const noexcept {
        return surface->vkHandle();
    }

    const PhysicalDevice* Instance::GetPhysicalDevice() const noexcept{
        return physicalDevice.get();
    }

    GLFWwindow * Instance::GetGLFWwindow() const noexcept {
        return window;
    }

    void RecreateSwapchainAndSurface(Instance * instance, Swapchain * swap) {
        swap->Destroy();
        instance->DestroySurfaceKHR();
        instance->CreateSurfaceKHR();
        swap->Recreate();
    }

    void Instance::checkApiVersionSupport(VkApplicationInfo* info) {
        uint32_t api_version = 0;
        vkEnumerateInstanceVersion(&api_version);
        if (VK_MAKE_VERSION(1, 1, 0) <= api_version) {
            info->apiVersion = VK_MAKE_VERSION(1, 1, 0);
        }
        else {
            info->apiVersion = VK_MAKE_VERSION(1, 0, 0);
        }
    }

    void Instance::prepareValidation(const char* const* layers, const uint32_t layer_count) {
        if (validationLayers != instance_layers::Disabled) {
            assert(!layers && (layer_count == 0));
            createInfo.ppEnabledLayerNames = nullptr;
            createInfo.enabledLayerCount = 0;
        }
        else {
            if (layers && (layer_count != 0)) {
                if (!checkValidationSupport(layers, layer_count)) {
                    throw std::runtime_error("Tried to enable Vulkan validation layers, but they're unsupported on the current system!");
                }
                createInfo.ppEnabledLayerNames = layers;
                createInfo.enabledLayerCount = layer_count;
            }
            else {
                constexpr static const char* const default_layer = "VK_LAYER_LUNARG_standard_validation";
                createInfo.ppEnabledLayerNames = &default_layer;
                createInfo.enabledLayerCount = 1;
            }
        }
    }

    bool Instance::checkValidationSupport(const char* const* layer_names, const uint32_t layer_count) const {
        uint32_t queried_count = 0;
        vkEnumerateInstanceLayerProperties(&queried_count, nullptr);
        std::vector<VkLayerProperties> queried_layers(queried_count);
        vkEnumerateInstanceLayerProperties(&queried_count, queried_layers.data());

        std::vector<const char*> layers{ layer_names, layer_names + layer_count };
        for (const auto& layer_name : layers) {
            const auto has_layer = [queried_layers](const char* name) {
                auto found_iter = std::find_if(queried_layers.cbegin(), queried_layers.cend(), [name](const VkLayerProperties& p) {
                    return strcmp(name, p.layerName) == 0;
                });
                return found_iter != queried_layers.cend();
            };

            if (!has_layer(layer_name)) {
                return false;
            }

        }

        return true;
    }

    void Instance::prepareValidationCallbacks() {
        const VkDebugReportCallbackCreateInfoEXT create_info{
            VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT,
            nullptr,
            VK_DEBUG_REPORT_INFORMATION_BIT_EXT | VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT | VK_DEBUG_REPORT_DEBUG_BIT_EXT,
            VkDebugCallbackFn
        };

        auto func = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(handle, "vkCreateDebugReportCallbackEXT");
        assert(func);
        VkResult result = func(handle, &create_info, nullptr, &debugCallback);
        VkAssert(result);
    }

    void Instance::extensionSetup(const VprExtensionPack* extensions) {

        std::vector<const char*> all_extensions;
        prepareRequiredExtensions(extensions, all_extensions);
        if (extensions != nullptr) {
            if (extensions->OptionalExtensionNames != nullptr) {
                prepareOptionalExtensions(extensions, all_extensions);
            }
        }
        enabledExtensions = all_extensions;
        createInfo.ppEnabledExtensionNames = enabledExtensions.data();
        createInfo.enabledExtensionCount = static_cast<uint32_t>(enabledExtensions.size());

    }

    void Instance::prepareRequiredExtensions(const VprExtensionPack* extensions, std::vector<const char*>& output) const {

        uint32_t req_ext_cnt = 0;
        const char** req_ext_names = nullptr;
        req_ext_names = glfwGetRequiredInstanceExtensions(&req_ext_cnt);
        std::vector<const char*> glfw_required_extensions{ req_ext_names, req_ext_names + req_ext_cnt };

        for (auto&& elem : glfw_required_extensions) {
            output.emplace_back(std::move(elem));
        }
        
        if (extensions != nullptr) {
            if (extensions->RequiredExtensionNames != nullptr) {
                std::vector<const char*> input_required_extensions{ extensions->RequiredExtensionNames,
                    extensions->RequiredExtensionNames + extensions->RequiredExtensionCount };

                for (auto&& elem : input_required_extensions) {
                    output.emplace_back(std::move(elem));
                }
            }
        }
    
        if (validationLayers != instance_layers::Disabled) {
            output.emplace_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
        }

        checkRequiredExtensions(output);
    }

    void Instance::prepareOptionalExtensions(const VprExtensionPack* extensions, std::vector<const char*>& output) const {
        std::vector<const char*> optional_extensions{ extensions->OptionalExtensionNames, 
            extensions->OptionalExtensionNames + extensions->OptionalExtensionCount };

        checkOptionalExtensions(optional_extensions);

        for(auto&& elem : optional_extensions) {
            output.emplace_back(std::move(elem));
        }

    }

    void Instance::extensionCheck(std::vector<const char*>& extensions, bool throw_on_error) const {
        uint32_t queried_extension_count = 0;
        vkEnumerateInstanceExtensionProperties(nullptr, &queried_extension_count, nullptr);
        std::vector<VkExtensionProperties> queried_extensions(queried_extension_count);
        vkEnumerateInstanceExtensionProperties(nullptr, &queried_extension_count, queried_extensions.data());

        const auto has_extension = [queried_extensions](const char* name) {
            auto req_found = std::find_if(queried_extensions.cbegin(), queried_extensions.cend(), [name](const VkExtensionProperties& properties) {
                return strcmp(properties.extensionName, name) == 0;
            });
            return req_found != queried_extensions.cend();
        };

        auto iter = extensions.begin();
        while (iter != extensions.end()) {
            if (!has_extension(*iter)) {
                if (throw_on_error) {
                    LOG(ERROR) << "Required extension \"" << *iter << "\" not supported for instance in construction!";
                    throw std::runtime_error("Instance does not support a required extension!");
                }
                else {
                    LOG(WARNING) << "Extension with name \"" << *iter << "\" requested but isn't supported. Removing from list attached to Instance's creation info.";
                    extensions.erase(iter++);
                }
            }
            else {
                ++iter;
            }
            
        }
    }

    void Instance::checkRequiredExtensions(std::vector<const char*>& required_extensions) const {
        extensionCheck(required_extensions, true);
    }

    void Instance::checkOptionalExtensions(std::vector<const char*>& requested_extensions) const {
        extensionCheck(requested_extensions, false);
    }

    void Instance::setupPhysicalDevice() {
        physicalDevice = std::make_unique<PhysicalDevice>(vkHandle());
    }

    constexpr const char* const GetObjectTypeStr(VkDebugReportObjectTypeEXT obj) {
        switch (obj) {
        case VK_DEBUG_REPORT_OBJECT_TYPE_INSTANCE_EXT:
            return "Instance";
        case VK_DEBUG_REPORT_OBJECT_TYPE_PHYSICAL_DEVICE_EXT:
            return "PhysicalDevice";
        case VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT:
            return "Device";
        case VK_DEBUG_REPORT_OBJECT_TYPE_QUEUE_EXT:
            return "Queue";
        case VK_DEBUG_REPORT_OBJECT_TYPE_SEMAPHORE_EXT:
            return "Semaphore";
        case VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT:
            return "CommandBuffer";
        case VK_DEBUG_REPORT_OBJECT_TYPE_FENCE_EXT:
            return "Fence";
        case VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_MEMORY_EXT:
            return "DeviceMemory";
        case VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT:
            return "Buffer";
        case VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT:
            return "Image";
        case VK_DEBUG_REPORT_OBJECT_TYPE_EVENT_EXT:
            return "Event";
        case VK_DEBUG_REPORT_OBJECT_TYPE_QUERY_POOL_EXT:
            return "QueryPool";
        case VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_VIEW_EXT:
            return "BufferView";
        case VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_VIEW_EXT:
            return "ImageView";
        case VK_DEBUG_REPORT_OBJECT_TYPE_SHADER_MODULE_EXT:
            return "ShaderModule";
        case VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_CACHE_EXT:
            return "PipelineCache";
        case VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_LAYOUT_EXT:
            return "PipelineLayout";
        case VK_DEBUG_REPORT_OBJECT_TYPE_RENDER_PASS_EXT:
            return "RenderPass";
        case VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_EXT:
            return "Pipeline";
        case VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT_EXT:
            return "DescriptorSet";
        case VK_DEBUG_REPORT_OBJECT_TYPE_SAMPLER_EXT:
            return "Sampler";
        case VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_POOL_EXT:
            return "DescriptorPool";
        case VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_EXT:
            return "DescriptorSet";
        case VK_DEBUG_REPORT_OBJECT_TYPE_FRAMEBUFFER_EXT:
            return "FrameBuffer";
        case VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_POOL_EXT:
            return "CommandPool";
        case VK_DEBUG_REPORT_OBJECT_TYPE_SURFACE_KHR_EXT:
            return "SurfaceKHR";
        case VK_DEBUG_REPORT_OBJECT_TYPE_SWAPCHAIN_KHR_EXT:
            return "SwapchainKHR";
        case VK_DEBUG_REPORT_OBJECT_TYPE_DEBUG_REPORT_CALLBACK_EXT_EXT:
            return "DebugReportCallbackEXT";
        case VK_DEBUG_REPORT_OBJECT_TYPE_DISPLAY_KHR_EXT:
            return "DisplayKHR";
        case VK_DEBUG_REPORT_OBJECT_TYPE_DISPLAY_MODE_KHR_EXT:
            return "DisplayModeKHR";
        case VK_DEBUG_REPORT_OBJECT_TYPE_OBJECT_TABLE_NVX_EXT:
            return "ObjectTableNVX";
        case VK_DEBUG_REPORT_OBJECT_TYPE_INDIRECT_COMMANDS_LAYOUT_NVX_EXT:
            return "IndirectCommandsLayoutNVX";
        case VK_DEBUG_REPORT_OBJECT_TYPE_VALIDATION_CACHE_EXT:
            return "ValidationCacheEXT";
        case VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_UPDATE_TEMPLATE_KHR_EXT:
            return "DescriptorUpdateTemplateKHR";
        case VK_DEBUG_REPORT_OBJECT_TYPE_SAMPLER_YCBCR_CONVERSION_KHR_EXT:
            return "SamplerYCBCR_ConversionKHR";
        default:
            return "UNDEFINED_OBJECT_TYPE";
        }
    }

    VKAPI_ATTR VkBool32 VKAPI_CALL VkDebugCallbackFn(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT type, uint64_t object_handle, 
        size_t location, int32_t message_code, const char * layer_prefix, const char * message, void * user_data) {

        switch (flags) {
        case VK_DEBUG_REPORT_ERROR_BIT_EXT:
            LOG(ERROR) << layer_prefix << "::" << GetObjectTypeStr(type) << "::HANDLE_ID:" << std::to_string(object_handle) << ": Code " << std::to_string(message_code) 
                << " with message of: " << message << "at location " << std::to_string(location);
            break;
        case VK_DEBUG_REPORT_WARNING_BIT_EXT:
            LOG(WARNING) << layer_prefix << "::" << GetObjectTypeStr(type) << "::HANDLE_ID:" << std::to_string(object_handle) << ": Code " << std::to_string(message_code) 
                << " with message of: " << message << "at location " << std::to_string(location);
            break;
        case VK_DEBUG_REPORT_INFORMATION_BIT_EXT:
            LOG(INFO) << layer_prefix << "::" << GetObjectTypeStr(type) << "::HANDLE_ID:" << std::to_string(object_handle) << ": Code " << std::to_string(message_code) 
                << " with message of: " << message << "at location " << std::to_string(location);
            break;
        case VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT:
            LOG(INFO) << layer_prefix << "::" << GetObjectTypeStr(type) << "::HANDLE_ID:" << std::to_string(object_handle) << ": Code " << std::to_string(message_code) 
                << " with message of: " << message << "at location " << std::to_string(location);
            break;
        case VK_DEBUG_REPORT_DEBUG_BIT_EXT:
            LOG(WARNING) << layer_prefix << "::" << GetObjectTypeStr(type) << "::HANDLE_ID:" << std::to_string(object_handle) << ": Code " << std::to_string(message_code) 
                << " with message of: " << message << "at location " << std::to_string(location);
            break;
        }
        return VK_FALSE;
    }

}
