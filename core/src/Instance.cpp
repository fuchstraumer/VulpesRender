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

    void SetLoggingRepository_VprCore(void* repo) {
        el::Helpers::setStorage(*(el::base::type::StoragePointer*)repo);
        LOG(INFO) << "Updated easyloggingpp storage pointer in vpr_core module...";
    }

    void* GetLoggingRepository_VprCore() {
        static el::base::type::StoragePointer ptr = el::Helpers::storage();
        return ptr.get();
    }

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
            
        if (!glfwVulkanSupported()) {
            LOG(ERROR) << "Vulkan is not supported on the current hardware!";
            throw std::runtime_error("Vulkan not supported!");
        }

        extensionHandler->extensionSetup(extensions);
        createInfo.ppEnabledExtensionNames = extensionHandler->extensionStrings.data();
        createInfo.enabledExtensionCount = static_cast<uint32_t>(extensionHandler->extensionStrings.size());
        checkApiVersionSupport(&our_info);
        prepareValidation(layers, layer_count);

        createInfo.pApplicationInfo = &our_info;
        VkResult err = vkCreateInstance(&createInfo, nullptr, &handle);
        VkAssert(err);

    }

    Instance::~Instance() {
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

    void Instance::GetEnabledExtensions(size_t* num_extensions, char** extensions) const {
        *num_extensions = extensionHandler->extensionStrings.size();
        if (extensions != nullptr) {
            for (size_t i = 0; i < *num_extensions; ++i) {
#ifdef __APPLE_CC__
                extensions[i] = strdup(extensionHandler->extensionStrings[i]);
#else
                extensions[i] = _strdup(extensionHandler->extensionStrings[i]);
#endif
            }
        }
    }

    const VkInstance& Instance::vkHandle() const noexcept {
        return handle;
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

}
