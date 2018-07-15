#include "vpr_stdafx.h"
#include "Instance.hpp"
#include "PhysicalDevice.hpp"
#include "LogicalDevice.hpp"
#include "vkAssert.hpp"
#include "CreateInfoBase.hpp"
#include "easylogging++.h"
INITIALIZE_EASYLOGGINGPP
#define GLFW_INCLUDE_VULKAN
#include "GLFW/glfw3.h"
#include <vulkan/vulkan.h>
#include <string>

namespace vpr {

    VKAPI_ATTR VkBool32 VKAPI_CALL VkDebugCallbackFn(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT type, uint64_t object_handle, 
        size_t location, int32_t message_code, const char * layer_prefix, const char * message, void * user_data);

    struct InstanceExtensionHandler {
        InstanceExtensionHandler(Instance::instance_layers layers) : validationLayers(layers) {}
        std::vector<const char*> extensionStrings;
        std::vector<std::string> copiedExtensionStrings;
        Instance::instance_layers validationLayers; 
        void extensionSetup(const VprExtensionPack* extensions);
        void prepareRequiredExtensions(const VprExtensionPack* extensions, std::vector<const char*>& output);
        void prepareOptionalExtensions(const VprExtensionPack* extensions, std::vector<const char*>& output);
        void extensionCheck(std::vector<const char*>& extensions, bool throw_on_error) const;
        void checkOptionalExtensions(std::vector<const char*>& optional_extensions) const;
        void checkRequiredExtensions(std::vector<const char*>& required_extensions) const;
    };

    Instance::Instance(instance_layers layers, const VkApplicationInfo* info) : Instance(layers, info, nullptr) {}

    Instance::Instance(instance_layers layers_flags, const VkApplicationInfo * info, const VprExtensionPack* extensions, const char* const* layers, const uint32_t layer_count) :
        extensionHandler(new InstanceExtensionHandler(layers_flags)), createInfo{ VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO, nullptr, 0, nullptr } {
        VkApplicationInfo our_info = *info;

        extensionHandler->extensionSetup(extensions);
        createInfo.ppEnabledExtensionNames = extensionHandler->extensionStrings.data();
        createInfo.enabledExtensionCount = static_cast<uint32_t>(extensionHandler->extensionStrings.size());
        checkApiVersionSupport(&our_info);
        prepareValidation(layers, layer_count);

        createInfo.pApplicationInfo = &our_info;
        VkResult err = vkCreateInstance(&createInfo, nullptr, &handle);
        VkAssert(err);

        if (extensionHandler->validationLayers != instance_layers::Disabled) {
            prepareValidationCallbacks();
        }

    }

    Instance::~Instance() {
        if (debugCallback && (extensionHandler->validationLayers != instance_layers::Disabled)) {
            auto func = (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(handle, "vkDestroyDebugReportCallbackEXT");
            func(handle, debugCallback, nullptr);
        }
        vkDestroyInstance(handle, nullptr);
    }

    bool Instance::ValidationEnabled() const noexcept {
        return (extensionHandler->validationLayers != instance_layers::Disabled);
    }

    bool Instance::HasExtension(const char* name) const noexcept {
        // have to use strcmp or we'll just compare addresses, heh
        auto iter = std::find_if(std::cbegin(extensionHandler->extensionStrings), std::cend(extensionHandler->extensionStrings), [name](const char* str) {
            return strcmp(name, str) == 0;
        });
        if (iter != std::cend(extensionHandler->extensionStrings)) {
            return true;
        }
        else {
            return false;
        }
    }

    const VkInstance& Instance::vkHandle() const noexcept {
        return handle;
    }

    void Instance::DebugMessage(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT obj_type, uint64_t obj, size_t location, int32_t msg_code, const char * layer, const char * message) {
        if (debugMessageFn != nullptr) {
            debugMessageFn(handle, flags, obj_type, obj, location, msg_code, layer, message);
        }
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
        if (extensionHandler->validationLayers == instance_layers::Disabled) {
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
                constexpr static const char* const default_layers[1] = { "VK_LAYER_LUNARG_standard_validation" };
                createInfo.ppEnabledLayerNames = default_layers;
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
        static const VkDebugReportCallbackCreateInfoEXT create_info{
            VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT,
            nullptr,
            VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT,
            VkDebugCallbackFn
        };

        auto func = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(handle, "vkCreateDebugReportCallbackEXT");
        assert(func);
        VkResult result = func(handle, &create_info, nullptr, &debugCallback);
        VkAssert(result);

        debugMessageFn = reinterpret_cast<PFN_vkDebugReportMessageEXT>(vkGetInstanceProcAddr(handle, "vkDebugReportMessageEXT"));
        assert(debugMessageFn);

    }

    void InstanceExtensionHandler::extensionSetup(const VprExtensionPack* extensions) {

        std::vector<const char*> all_extensions;
        prepareRequiredExtensions(extensions, all_extensions);
        if (extensions != nullptr) {
            if (extensions->OptionalExtensionNames != nullptr) {
                prepareOptionalExtensions(extensions, all_extensions);
            }
        }
        extensionStrings = all_extensions;

    }

    void InstanceExtensionHandler::prepareRequiredExtensions(const VprExtensionPack* extensions, std::vector<const char*>& output) {

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
                    copiedExtensionStrings.emplace_back(std::string(elem));
                    output.emplace_back(copiedExtensionStrings.back().c_str());
                }
            }
        }
    
        if (validationLayers != Instance::instance_layers::Disabled) {
            output.emplace_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
        }

        checkRequiredExtensions(output);
    }

    void InstanceExtensionHandler::prepareOptionalExtensions(const VprExtensionPack* extensions, std::vector<const char*>& output) {
        std::vector<const char*> optional_extensions{ extensions->OptionalExtensionNames, 
            extensions->OptionalExtensionNames + extensions->OptionalExtensionCount };

        checkOptionalExtensions(optional_extensions);

        for(auto& elem : optional_extensions) {
            copiedExtensionStrings.emplace_back(std::string(elem));
            output.emplace_back(copiedExtensionStrings.back().c_str());
        }

    }

    void InstanceExtensionHandler::extensionCheck(std::vector<const char*>& extensions, bool throw_on_error) const {
        uint32_t queried_extension_count = 0;
        vkEnumerateInstanceExtensionProperties(nullptr, &queried_extension_count, nullptr);
        std::vector<VkExtensionProperties> queried_extensions(queried_extension_count);
        vkEnumerateInstanceExtensionProperties(nullptr, &queried_extension_count, queried_extensions.data());

        auto iter = std::remove_if(std::begin(extensions), std::end(extensions), [queried_extensions, throw_on_error](const char* name) {
            auto req_found = std::find_if(queried_extensions.cbegin(), queried_extensions.cend(), [name](const VkExtensionProperties& properties) {
                return strcmp(properties.extensionName, name) == 0;
            });
            bool result = (req_found != queried_extensions.cend());
            if (throw_on_error && !result) {
                LOG(ERROR) << "Required extension \"" << name << "\" not supported for instance in construction!";
                throw std::runtime_error("Instance does not support a required extension!");
            }
            else if (!result) {
                LOG(WARNING) << "Extension with name \"" << name << "\" requested but isn't supported. Removing from list attached to Instance's creation info.";
            }
            return !result;
        });

        extensions.erase(iter, std::end(extensions));
    }

    void InstanceExtensionHandler::checkRequiredExtensions(std::vector<const char*>& required_extensions) const {
        extensionCheck(required_extensions, true);
    }

    void InstanceExtensionHandler::checkOptionalExtensions(std::vector<const char*>& requested_extensions) const {
        extensionCheck(requested_extensions, false);
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
                << " with message of: " << message << " at location " << std::to_string(location);
            break;
        case VK_DEBUG_REPORT_WARNING_BIT_EXT:
            LOG(WARNING) << layer_prefix << "::" << GetObjectTypeStr(type) << "::HANDLE_ID:" << std::to_string(object_handle) << ": Code " << std::to_string(message_code) 
                << " with message of: " << message << " at location " << std::to_string(location);
            break;
        case VK_DEBUG_REPORT_INFORMATION_BIT_EXT:
            LOG(INFO) << layer_prefix << "::" << GetObjectTypeStr(type) << "::HANDLE_ID:" << std::to_string(object_handle) << ": Code " << std::to_string(message_code) 
                << " with message of: " << message << " at location " << std::to_string(location);
            break;
        case VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT:
            LOG(INFO) << layer_prefix << "::" << GetObjectTypeStr(type) << "::HANDLE_ID:" << std::to_string(object_handle) << ": Code " << std::to_string(message_code) 
                << " with message of: " << message << " at location " << std::to_string(location);
            break;
        case VK_DEBUG_REPORT_DEBUG_BIT_EXT:
            LOG(WARNING) << layer_prefix << "::" << GetObjectTypeStr(type) << "::HANDLE_ID:" << std::to_string(object_handle) << ": Code " << std::to_string(message_code) 
                << " with message of: " << message << " at location " << std::to_string(location);
            break;
        }
        return VK_FALSE;
    }

}
