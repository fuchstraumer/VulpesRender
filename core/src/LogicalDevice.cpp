#include "vpr_stdafx.h"
#include "LogicalDevice.hpp"
#include "VkDebugUtils.hpp"
#include "PhysicalDevice.hpp"
#include "Instance.hpp"
#include "vkAssert.hpp"
#include "CreateInfoBase.hpp"
#include "easylogging++.h"
#include <array>
#include <vector>
#include <map>

namespace vpr {

    struct DeviceDataMembers {
        DeviceDataMembers(const Device* p) : device(p) {}
        std::vector<const char*> enabledExtensions;
        // Need to copy some strings, or we'll just end up trying to use the address of passed user strings
        std::vector<std::string> copiedExtensionStrings;
        const Device* device = nullptr;
        std::map<VkQueueFlags, VkDeviceQueueCreateInfo> queueInfos;
        bool enableDedicatedAllocations{ false };
        void prepareRequiredExtensions(const VprExtensionPack* extensions, std::vector<const char*>& output);
        void prepareOptionalExtensions(const VprExtensionPack* extensions, std::vector<const char*>& output) noexcept;
        void checkExtensions(std::vector<const char*>& requested_extensions, bool throw_on_error) const;
        void checkDedicatedAllocExtensions(const std::vector<const char*>& exts);
    };

    constexpr const char* const RECOMMENDED_REQUIRED_EXTENSION = "VK_KHR_swapchain";

    constexpr static std::array<const char*, 3> RECOMMENDED_OPTIONAL_EXTENSIONS {
        VK_KHR_DEDICATED_ALLOCATION_EXTENSION_NAME, 
        VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME
    };

    constexpr VprExtensionPack RECOMMENDED_EXTENSIONS {
        &RECOMMENDED_REQUIRED_EXTENSION,
        1,
        &RECOMMENDED_OPTIONAL_EXTENSIONS[0],
        static_cast<uint32_t>(RECOMMENDED_OPTIONAL_EXTENSIONS.size())
    };

    struct queue_priorities_t {
        std::vector<float> graphics;
        std::vector<float> transfer;
        std::vector<float> compute;
        std::vector<float> sparse_binding;
    } queue_priorities;


    vkQueueFamilyIndices::vkQueueFamilyIndices() : Graphics(std::numeric_limits<uint32_t>::max()), Compute(std::numeric_limits<uint32_t>::max()),
        Transfer(std::numeric_limits<uint32_t>::max()), SparseBinding(std::numeric_limits<uint32_t>::max()), Present(std::numeric_limits<uint32_t>::max()) {}

    Device::Device(const Instance* instance, const PhysicalDevice* dvc, VkSurfaceKHR _surface, const VprExtensionPack* extensions, const char* const* layer_names,
        const uint32_t layer_count) : parent(dvc), parentInstance(instance), debugUtilsHandler(nullptr), dataMembers(nullptr), surface(_surface) {
        if (extensions != nullptr) {
            create(extensions, layer_names, layer_count);
        }
        else {
            create(&RECOMMENDED_EXTENSIONS, nullptr, 0);
        }
    }

    Device::~Device(){
        if (dataMembers) {
            delete dataMembers;
        }
        if (debugUtilsHandler) {
            delete debugUtilsHandler;
        }
        vkDestroyDevice(handle, nullptr);
    }

    const VkDevice & Device::vkHandle() const{
        return handle;
    }

    bool Device::HasDedicatedComputeQueues() const {
        if (QueueFamilyIndices.Compute != QueueFamilyIndices.Graphics) {
            return true;
        }
        return false;
    }

    bool Device::DedicatedAllocationExtensionsEnabled() const noexcept {
        return dataMembers->enableDedicatedAllocations;
    }

    bool Device::HasExtension(const char* name) const noexcept {
        auto iter = std::find_if(std::cbegin(dataMembers->enabledExtensions), std::cend(dataMembers->enabledExtensions), [name](const char* str) {
            return strcmp(name, str) == 0;
        });
        if (iter != std::cend(dataMembers->enabledExtensions)) {
            return true;
        }
        else {
            return false;
        }
    }

    void Device::GetEnabledExtensions(size_t * num_extensions, char ** extensions) const {
        *num_extensions = dataMembers->enabledExtensions.size();
        if (extensions != nullptr) {
            for (size_t i = 0; i < *num_extensions; ++i) {
                extensions[i] = _strdup(dataMembers->enabledExtensions[i]);
            }
        }
    }

    void Device::UpdateSurface(VkSurfaceKHR new_surface) {
        surface = new_surface;
        verifyPresentationSupport();
    }

    void Device::checkSurfaceSupport(const VkSurfaceKHR& surf) {
        VkBool32 supported;
        vkGetPhysicalDeviceSurfaceSupportKHR(parent->vkHandle(), QueueFamilyIndices.Present, surf, &supported);
        assert(supported);
    }

    VkQueue Device::GraphicsQueue(const uint32_t & idx) const{
        assert(idx < NumGraphicsQueues);
        VkQueue result;
        vkGetDeviceQueue(vkHandle(), QueueFamilyIndices.Graphics, idx, &result);
        return result;
    }

    VkQueue Device::TransferQueue(const uint32_t & idx) const{
        static bool logged_warning = false;
        if ((QueueFamilyIndices.Transfer == QueueFamilyIndices.Graphics) && !logged_warning) {
            LOG(WARNING) << "Retrieving queue that supports transfer ops, but isn't dedicated transfer queue. This warning is only issued once - but retrieval is likely occuring multiple times.";
            logged_warning = true;
        }
        VkQueue result;
        vkGetDeviceQueue(vkHandle(), QueueFamilyIndices.Transfer, idx, &result);
        return result;

    }

    VkQueue Device::ComputeQueue(const uint32_t & idx) const{
        static bool logged_warning = false;
        if ((QueueFamilyIndices.Compute == QueueFamilyIndices.Graphics) && !logged_warning) {
            LOG(WARNING) << "Retrieving queue that supports compute ops, but isn't dedicated compute queue. This warning is only issued once - but retrieval is likely occuring multiple times.";
            logged_warning = true;
        }
        VkQueue result;
        vkGetDeviceQueue(vkHandle(), QueueFamilyIndices.Compute, idx, &result);
        return result;

    }

    VkQueue Device::SparseBindingQueue(const uint32_t & idx) const {
        
        if (!dataMembers->queueInfos.count(VK_QUEUE_SPARSE_BINDING_BIT)) {
            LOG(ERROR) << "Current device does not support sparse binding queues!";
            throw std::runtime_error("Requested unsuported queue family (Sparse Binding)");
        }

        LOG_IF(QueueFamilyIndices.Compute == QueueFamilyIndices.Graphics, INFO) << "Retrieving queue that supports sparse binding, but isn't dedicated sparse binding queue.";
        VkQueue result;
        vkGetDeviceQueue(handle, QueueFamilyIndices.SparseBinding, idx, &result);
        return result;

    }

    VkImageTiling Device::GetFormatTiling(const VkFormat & format, const VkFormatFeatureFlags & flags) const {
        VkFormatProperties properties;
        vkGetPhysicalDeviceFormatProperties(parent->vkHandle(), format, &properties);
        if (properties.optimalTilingFeatures & flags) {
            return VK_IMAGE_TILING_OPTIMAL;
        }
        else {
            // Check that the device at least supports the desired features for linear tiling
            if (!(properties.linearTilingFeatures & flags)) {
                LOG(ERROR) << "Could not retrieve VkImageTiling mode for format, indicating that the format is probably not supported on the current device!";
                throw std::runtime_error("Requested format is likely not supported on current device!");
            }
            return VK_IMAGE_TILING_LINEAR;
        }
    }

    VkFormat Device::FindSupportedFormat(const VkFormat* formats, const size_t num_formats, const VkImageTiling & tiling, const VkFormatFeatureFlags & flags) const {
        for (size_t i = 0; i < num_formats; ++i) {
            VkFormatProperties properties;
            vkGetPhysicalDeviceFormatProperties(parent->vkHandle(), formats[i], &properties);
            if (tiling == VK_IMAGE_TILING_LINEAR && (properties.linearTilingFeatures & flags) == flags) {
                return formats[i];
            }
            else if (tiling == VK_IMAGE_TILING_OPTIMAL && (properties.optimalTilingFeatures & flags) == flags) {
                return formats[i];
            }
        }
        LOG(ERROR) << "Could not find texture format that supports requested tiling and feature flags: ( " << std::to_string(tiling) << " , " << std::to_string(flags) << " )";
        throw std::runtime_error("Could not find valid texture format.");
    }

    VkFormat Device::FindDepthFormat() const{
        static const std::vector<VkFormat> format_options{ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT };
        return FindSupportedFormat(format_options.data(), format_options.size(), VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
    }

    uint32_t Device::GetMemoryTypeIdx(const uint32_t & type_bitfield, const VkMemoryPropertyFlags & property_flags, VkBool32 * memory_type_found) const{
        return parent->GetMemoryTypeIdx(type_bitfield, property_flags, memory_type_found);
    }

    uint32_t Device::GetPhysicalDeviceID() const noexcept{
        return parent->GetProperties().deviceID;
    }

    const PhysicalDevice & Device::GetPhysicalDevice() const noexcept{
        return *parent;
    }

    VkPhysicalDeviceProperties Device::GetPhysicalDeviceProperties() const noexcept {
        return parent->GetProperties();
    }

    VkPhysicalDeviceMemoryProperties Device::GetPhysicalDeviceMemoryProperties() const noexcept {
        return parent->GetMemoryProperties();
    }

    const VkDebugUtilsFunctions& Device::DebugUtilsHandler() const {
        // In case we don't have member to return, return fully empty struct
        static const VkDebugUtilsFunctions fallback{ nullptr };
        if (debugUtilsHandler) {
            return *debugUtilsHandler;
        }
        else {
            return fallback;
        }
    }

    VkQueue Device::GeneralQueue(const uint32_t & desired_idx) const {
        
        uint32_t idx = parent->GetQueueFamilyIndex(VkQueueFlagBits(VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_TRANSFER_BIT | VK_QUEUE_COMPUTE_BIT));
        if (idx == std::numeric_limits<uint32_t>::max()) {
            LOG(WARNING) << "Couldn't find a generalized queue supporting compute, graphics, and transfer: trying graphics and transfer.";
            idx = parent->GetQueueFamilyIndex(VkQueueFlagBits(VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_TRANSFER_BIT));
            if (idx == std::numeric_limits<uint32_t>::max()) {
                LOG(WARNING) << "Couldn't find a generalized queue supporting transfer and graphics operations: just returning a graphics queue.";
                idx = QueueFamilyIndices.Graphics;
            }
        }

        VkQueue result;
        vkGetDeviceQueue(vkHandle(), idx, desired_idx, &result);
        return result;

    }

    void Device::create(const VprExtensionPack* extensions, const char* const* layers, const uint32_t layer_count) {

        createInfo = vk_device_create_info_base;
        dataMembers = new DeviceDataMembers(this);
        if (surface != VK_NULL_HANDLE) {
            verifyPresentationSupport();
        }
        setupQueues();
        setupValidation(layers, layer_count);
        setupExtensions(extensions);

        // This has to be here so that the queue_infos vector persists through device creation.
        std::vector<VkDeviceQueueCreateInfo> queue_infos;
        for (const auto& queue_info_entry : dataMembers->queueInfos) {
            queue_infos.emplace_back(queue_info_entry.second);
        }

        createInfo.queueCreateInfoCount = static_cast<uint32_t>(queue_infos.size());
        createInfo.pQueueCreateInfos = queue_infos.data();
        createInfo.pEnabledFeatures = &parent->GetFeatures();

        VkResult result = vkCreateDevice(parent->vkHandle(), &createInfo, nullptr, &handle);
        VkAssert(result);

        setupDebugUtilsHandler();

    }

    void Device::setupQueues() {

        setupGraphicsQueues();
        setupComputeQueues();
        setupTransferQueues();
        setupSparseBindingQueues(); 

    }

    void Device::setupValidation(const char* const* layers, const uint32_t layer_count) {
        
        if (parentInstance->ValidationEnabled()) {
            if ((layer_count == 0) && (layers == nullptr)) {
                constexpr static const char* const default_layer = "VK_LAYER_LUNARG_standard_validation";
                createInfo.enabledLayerCount = 1;
                createInfo.ppEnabledLayerNames = &default_layer;
            }
            else {
                createInfo.enabledLayerCount = layer_count;
                createInfo.ppEnabledLayerNames = layers;
            }
        }

    }

    void Device::setupExtensions(const VprExtensionPack* extensions) {
        
        if (extensions) {
            std::vector<const char*> all_extensions;
            dataMembers->prepareRequiredExtensions(extensions, all_extensions);
            dataMembers->prepareOptionalExtensions(extensions, all_extensions);
            dataMembers->checkDedicatedAllocExtensions(all_extensions);
            dataMembers->enabledExtensions = all_extensions;
            createInfo.enabledExtensionCount = static_cast<uint32_t>(dataMembers->enabledExtensions.size());
            createInfo.ppEnabledExtensionNames = dataMembers->enabledExtensions.data();
        }
        else {
            createInfo.enabledExtensionCount = 0;
            createInfo.ppEnabledExtensionNames = nullptr;
            dataMembers->enableDedicatedAllocations = false;
        }

    }

    void Device::setupGraphicsQueues() {
        QueueFamilyIndices.Graphics = parent->GetQueueFamilyIndex(VK_QUEUE_GRAPHICS_BIT);
        auto create_info = setupQueueFamily(parent->GetQueueFamilyProperties(VK_QUEUE_GRAPHICS_BIT));
        create_info.queueFamilyIndex = QueueFamilyIndices.Graphics;
        NumGraphicsQueues = 1;
        create_info.queueCount = NumGraphicsQueues;
        queue_priorities.graphics = std::vector<float>(NumGraphicsQueues, 1.0f);
        create_info.pQueuePriorities = queue_priorities.graphics.data();
        dataMembers->queueInfos.insert(std::make_pair(VK_QUEUE_GRAPHICS_BIT, create_info));
    }

    void Device::setupComputeQueues() {
        QueueFamilyIndices.Compute = parent->GetQueueFamilyIndex(VK_QUEUE_COMPUTE_BIT);
        if (QueueFamilyIndices.Compute != QueueFamilyIndices.Graphics) {
            auto compute_info = setupQueueFamily(parent->GetQueueFamilyProperties(VK_QUEUE_COMPUTE_BIT));
            compute_info.queueFamilyIndex = QueueFamilyIndices.Compute;
            NumComputeQueues = compute_info.queueCount;
            compute_info.queueCount = NumComputeQueues;
            queue_priorities.compute = std::vector<float>(NumComputeQueues, 1.0f);
            compute_info.pQueuePriorities = queue_priorities.compute.data();
            dataMembers->queueInfos.insert(std::make_pair(VK_QUEUE_COMPUTE_BIT, compute_info));
        }
        else {
            auto queue_properties = parent->GetQueueFamilyProperties(VK_QUEUE_COMPUTE_BIT);
            if (queue_properties.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                QueueFamilyIndices.Compute = QueueFamilyIndices.Graphics;
                NumComputeQueues = NumGraphicsQueues;

            }
        }
    }

    void Device::setupTransferQueues() {
        QueueFamilyIndices.Transfer = parent->GetQueueFamilyIndex(VK_QUEUE_TRANSFER_BIT);
        if (QueueFamilyIndices.Transfer != QueueFamilyIndices.Graphics) {
            auto transfer_info = setupQueueFamily(parent->GetQueueFamilyProperties(VK_QUEUE_TRANSFER_BIT));
            transfer_info.queueFamilyIndex = QueueFamilyIndices.Transfer;
            NumTransferQueues = transfer_info.queueCount;
            queue_priorities.transfer = std::vector<float>(NumTransferQueues, 1.0f);
            transfer_info.pQueuePriorities = queue_priorities.transfer.data();
            dataMembers->queueInfos.insert(std::make_pair(VK_QUEUE_TRANSFER_BIT, transfer_info));
        }
        else {
            auto queue_properties = parent->GetQueueFamilyProperties(VK_QUEUE_TRANSFER_BIT);
            if (queue_properties.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                QueueFamilyIndices.Transfer = QueueFamilyIndices.Graphics;
                NumTransferQueues = NumGraphicsQueues;
            }
        }
    }

    void Device::setupSparseBindingQueues() {
        QueueFamilyIndices.SparseBinding = parent->GetQueueFamilyIndex(VK_QUEUE_SPARSE_BINDING_BIT);
        if (QueueFamilyIndices.SparseBinding != QueueFamilyIndices.Graphics) {
            auto sparse_info = setupQueueFamily(parent->GetQueueFamilyProperties(VK_QUEUE_SPARSE_BINDING_BIT));
            sparse_info.queueFamilyIndex = QueueFamilyIndices.SparseBinding;
            NumSparseBindingQueues = sparse_info.queueCount;
            queue_priorities.sparse_binding = std::vector<float>(NumSparseBindingQueues, 1.0f);
            sparse_info.pQueuePriorities = queue_priorities.sparse_binding.data();
            dataMembers->queueInfos.insert(std::make_pair(VK_QUEUE_SPARSE_BINDING_BIT, sparse_info));
        }
        else {
            auto queue_properties = parent->GetQueueFamilyProperties(VK_QUEUE_SPARSE_BINDING_BIT);
            if (queue_properties.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                QueueFamilyIndices.SparseBinding = QueueFamilyIndices.Graphics;
                NumSparseBindingQueues = NumGraphicsQueues;
            }
        }
    }

    void Device::verifyPresentationSupport() {
    
        // Check presentation support
        VkBool32 present_support = false;
        for (uint32_t i = 0; i < 3; ++i) {
            vkGetPhysicalDeviceSurfaceSupportKHR(parent->vkHandle(), i, surface, &present_support);
            if (present_support) {
                QueueFamilyIndices.Present = i;
                break;
            }
        }

        if (!present_support) {
            LOG(ERROR) << "No queues found that support presentation to a surface.";
            throw std::runtime_error("No queues found that support presentation to a surface!");
        }

    }

    VkDeviceQueueCreateInfo Device::setupQueueFamily(const VkQueueFamilyProperties & family_properties) {
        VkDeviceQueueCreateInfo result = vk_device_queue_create_info_base;
        result.queueCount = family_properties.queueCount;
        return result;
    }

    void DeviceDataMembers::prepareRequiredExtensions(const VprExtensionPack* extensions, std::vector<const char*>& output) {
        std::vector<const char*> required_extensions{ extensions->RequiredExtensionNames, 
            extensions->RequiredExtensionNames + extensions->RequiredExtensionCount };
        checkExtensions(required_extensions, true);

        for (auto&& elem : required_extensions) {
            copiedExtensionStrings.emplace_back(std::string(elem));
            output.emplace_back(copiedExtensionStrings.back().c_str());
        }
    }

    void DeviceDataMembers::prepareOptionalExtensions(const VprExtensionPack* extensions, std::vector<const char*>& output) noexcept {
        std::vector<const char*> optional_extensions{ extensions->OptionalExtensionNames, 
            extensions->OptionalExtensionNames + extensions->OptionalExtensionCount };
        checkExtensions(optional_extensions, false);

        for (auto&& elem : optional_extensions) {
            copiedExtensionStrings.emplace_back(std::string(elem));
            output.emplace_back(copiedExtensionStrings.back().c_str());
        }

    }

    void DeviceDataMembers::checkExtensions(std::vector<const char*>& extensions, bool throw_on_error) const {
        uint32_t queried_count = 0;
        vkEnumerateDeviceExtensionProperties(device->GetPhysicalDevice().vkHandle(), nullptr, &queried_count, nullptr);
        std::vector<VkExtensionProperties> queried_extensions(queried_count);
        vkEnumerateDeviceExtensionProperties(device->GetPhysicalDevice().vkHandle(), nullptr, &queried_count, queried_extensions.data());

        auto iter = std::remove_if(std::begin(extensions), std::end(extensions), [queried_extensions, throw_on_error](const char* name) {
            const auto has_extension = [queried_extensions](const char* name) {
                const auto req_found = std::find_if(queried_extensions.cbegin(), queried_extensions.cend(), [name](const VkExtensionProperties& p) {
                    return strcmp(p.extensionName, name) == 0;
                });
                return req_found != queried_extensions.end();
            };

            if (!has_extension(name)) {
                if (throw_on_error) {
                    LOG(ERROR) << "Current VkDevice does not support extension \"" << name << "\" that is required!";
                    throw std::runtime_error("Could not enable/use required extension for the logical device.");
                }
                else {
                    LOG(WARNING) << "Requested device extension with name \"" << name << "\" is not available, removing from list.";
                }
                return true;
            }
            else {
                return false;
            }
        });

        extensions.erase(iter, extensions.end());
    }
    
    void DeviceDataMembers::checkDedicatedAllocExtensions(const std::vector<const char*>& exts) {
        constexpr std::array<const char*, 2> mem_extensions{ VK_KHR_DEDICATED_ALLOCATION_EXTENSION_NAME, VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME };
        auto iter = std::find(exts.cbegin(), exts.cend(), mem_extensions[0]);
        if (iter != exts.cend()) {
            iter = std::find(exts.cbegin(), exts.cend(), mem_extensions[1]);
            if (iter != exts.cend()) {
                LOG_IF(VERBOSE_LOGGING, INFO) << "Both extensions required to enable better dedicated allocations have been enabled/found.";
                enableDedicatedAllocations = true;
            }    
            else {
                LOG_IF(VERBOSE_LOGGING, WARNING) << "Only one of the extensions required for better allocations was found - cannot enable/use.";
                enableDedicatedAllocations = false;
            }
        }
        else {
            enableDedicatedAllocations = false;
        }
    }

    void Device::setupDebugUtilsHandler() {
        auto check_loaded_pfn = [](const void* ptr, const char* fname) {
            if (!ptr) {
                LOG(ERROR) << "Failed to load function pointer " << fname << "for debug utils extension!";
            }
        };

        if (!parentInstance->ValidationEnabled()) {
            LOG(WARNING) << "Cannot load requested VkDebugUtils function pointers, as validation layers are not enabled!";
            return;
        }

        if (parentInstance->HasExtension(VK_EXT_DEBUG_UTILS_EXTENSION_NAME)) {
            debugUtilsHandler = new VkDebugUtilsFunctions();
            debugUtilsHandler->vkSetDebugUtilsObjectName = 
                reinterpret_cast<PFN_vkSetDebugUtilsObjectNameEXT>(vkGetDeviceProcAddr(handle, "vkSetDebugUtilsObjectNameEXT"));
            check_loaded_pfn((void*)debugUtilsHandler->vkSetDebugUtilsObjectName, "vkSetDebugUtilsObjectName");
            debugUtilsHandler->vkSetDebugUtilsObjectTag =
                reinterpret_cast<PFN_vkSetDebugUtilsObjectTagEXT>(vkGetDeviceProcAddr(handle, "vkSetDebugUtilsObjectTagEXT"));
            check_loaded_pfn((void*)debugUtilsHandler->vkSetDebugUtilsObjectTag, "vkSetDebugUtilsObjectTag");
            debugUtilsHandler->vkQueueBeginDebugUtilsLabel = 
                reinterpret_cast<PFN_vkQueueBeginDebugUtilsLabelEXT>(vkGetDeviceProcAddr(handle, "vkQueueBeginDebugUtilsLabelEXT"));
            check_loaded_pfn((void*)debugUtilsHandler->vkQueueBeginDebugUtilsLabel, "vkQueueBeginDebugUtilsLabel");
            debugUtilsHandler->vkQueueEndDebugUtilsLabel = 
                reinterpret_cast<PFN_vkQueueEndDebugUtilsLabelEXT>(vkGetDeviceProcAddr(handle, "vkQueueEndDebugUtilsLabelEXT"));
            check_loaded_pfn((void*)debugUtilsHandler->vkQueueEndDebugUtilsLabel, "vkQueueEndDebugUtilsLabel");
            debugUtilsHandler->vkCmdBeginDebugUtilsLabel =
                reinterpret_cast<PFN_vkCmdBeginDebugUtilsLabelEXT>(vkGetDeviceProcAddr(handle, "vkCmdBeginDebugUtilsLabelEXT"));
            check_loaded_pfn((void*)debugUtilsHandler->vkCmdBeginDebugUtilsLabel, "vkCmdBeginDebugUtilsLabel");
            debugUtilsHandler->vkCmdEndDebugUtilsLabel =
                reinterpret_cast<PFN_vkCmdEndDebugUtilsLabelEXT>(vkGetDeviceProcAddr(handle, "vkCmdEndDebugUtilsLabelEXT"));
            check_loaded_pfn((void*)debugUtilsHandler->vkCmdEndDebugUtilsLabel, "vkCmdEndDebugUtilsLabel");
            debugUtilsHandler->vkCmdInsertDebugUtilsLabel =
                reinterpret_cast<PFN_vkCmdInsertDebugUtilsLabelEXT>(vkGetDeviceProcAddr(handle, "vkCmdInsertDebugUtilsLabelEXT"));
            check_loaded_pfn((void*)debugUtilsHandler->vkCmdInsertDebugUtilsLabel, "vkCmdInsertDebugUtilsLabel");
            debugUtilsHandler->vkCreateDebugUtilsMessenger =
                reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(vkGetDeviceProcAddr(handle, "vkCreateDebugUtilsMessengerEXT"));
            check_loaded_pfn((void*)debugUtilsHandler->vkCreateDebugUtilsMessenger, "vkCreateDebugUtilsMessenger");
            debugUtilsHandler->vkDestroyDebugUtilsMessenger =
                reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(vkGetDeviceProcAddr(handle, "vkDestroyDebugUtilsMessengerEXT"));
            check_loaded_pfn((void*)debugUtilsHandler->vkDestroyDebugUtilsMessenger, "vkDestroyDebugUtilsMessenger");
            debugUtilsHandler->vkSubmitDebugUtilsMessage =
                reinterpret_cast<PFN_vkSubmitDebugUtilsMessageEXT>(vkGetDeviceProcAddr(handle, "vkSubmitDebugUtilsMessageEXT"));
            check_loaded_pfn((void*)debugUtilsHandler->vkSubmitDebugUtilsMessage, "vkSubmitDebugUtilsMessage");
        }
    }

}
