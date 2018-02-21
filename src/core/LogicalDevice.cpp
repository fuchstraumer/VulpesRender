#include "vpr_stdafx.h"
#include "core/LogicalDevice.hpp"
#include "core/PhysicalDevice.hpp"
#include "core/Instance.hpp"
#include "alloc/Allocator.hpp"
#include "util/easylogging++.h"

namespace vpr {

    constexpr const char* const RECOMMENDED_REQUIRED_EXTENSION = "VK_KHR_swapchain";
    constexpr static std::array<const char*, 2> RECOMMENDED_OPTIONAL_EXTENSIONS {
        VK_KHR_DEDICATED_ALLOCATION_EXTENSION_NAME, VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME
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

    Device::Device(const Instance* instance, const PhysicalDevice * device, bool use_recommended_extensions) : parent(device), parentInstance(instance) {
        if (use_recommended_extensions) {
            create(&RECOMMENDED_EXTENSIONS, nullptr, 0);
        }
        else {
            create(nullptr, nullptr, 0);
        }
    }

    Device::Device(const Instance* instance, const PhysicalDevice* dvc, const VprExtensionPack* extensions, const char* const* layer_names, 
        const uint32_t layer_count) : parent(dvc), parentInstance(instance) {
        create(extensions, layer_names, layer_count);
    }

    void Device::create(const VprExtensionPack* extensions, const char* const* layers, const uint32_t layer_count) {

        createInfo = vk_device_create_info_base;
        VerifyPresentationSupport();
        setupQueues();
        setupValidation(layers, layer_count);
        setupExtensions(extensions);

        // This has to be here so that the queue_infos vector persists through device creation.
        std::vector<VkDeviceQueueCreateInfo> queue_infos;
        for (const auto& queue_info_entry : queueInfos) {
            queue_infos.push_back(queue_info_entry.second);
        }

        createInfo.queueCreateInfoCount = static_cast<uint32_t>(queue_infos.size());
        createInfo.pQueueCreateInfos = queue_infos.data();
        createInfo.pEnabledFeatures = &parent->Features;

        VkResult result = vkCreateDevice(parent->vkHandle(), &createInfo, nullptr, &handle);
        VkAssert(result);

        vkAllocator = std::make_unique<Allocator>(this, enableDedicatedAllocations);

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
            prepareRequiredExtensions(extensions, all_extensions);
            prepareOptionalExtensions(extensions, all_extensions);
            checkDedicatedAllocExtensions(all_extensions);
            enabledExtensions = all_extensions;
            createInfo.enabledExtensionCount = static_cast<uint32_t>(enabledExtensions.size());
            createInfo.ppEnabledExtensionNames = enabledExtensions.data();
        }
        else {
            createInfo.enabledExtensionCount = 0;
            createInfo.ppEnabledExtensionNames = nullptr;
            enableDedicatedAllocations = false;
        }

    }

    void Device::setupGraphicsQueues() {
        QueueFamilyIndices.Graphics = parent->GetQueueFamilyIndex(VK_QUEUE_GRAPHICS_BIT);
        auto create_info = SetupQueueFamily(parent->GetQueueFamilyProperties(VK_QUEUE_GRAPHICS_BIT));
        create_info.queueFamilyIndex = QueueFamilyIndices.Graphics;
        NumGraphicsQueues = create_info.queueCount;
        queue_priorities.graphics = std::vector<float>(NumGraphicsQueues, 1.0f);
        create_info.pQueuePriorities = queue_priorities.graphics.data();
        queueInfos.insert(std::make_pair(VK_QUEUE_GRAPHICS_BIT, create_info));
    }

    void Device::setupComputeQueues() {
        QueueFamilyIndices.Compute = parent->GetQueueFamilyIndex(VK_QUEUE_COMPUTE_BIT);
        if (QueueFamilyIndices.Compute != QueueFamilyIndices.Graphics) {
            auto compute_info = SetupQueueFamily(parent->GetQueueFamilyProperties(VK_QUEUE_COMPUTE_BIT));
            compute_info.queueFamilyIndex = QueueFamilyIndices.Compute;
            NumComputeQueues = compute_info.queueCount;
            queue_priorities.compute = std::vector<float>(NumComputeQueues, 1.0f);
            compute_info.pQueuePriorities = queue_priorities.compute.data();
            queueInfos.insert(std::make_pair(VK_QUEUE_COMPUTE_BIT, compute_info));
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
            auto transfer_info = SetupQueueFamily(parent->GetQueueFamilyProperties(VK_QUEUE_TRANSFER_BIT));
            transfer_info.queueFamilyIndex = QueueFamilyIndices.Transfer;
            NumTransferQueues = transfer_info.queueCount;
            queue_priorities.transfer = std::vector<float>(NumTransferQueues, 1.0f);
            transfer_info.pQueuePriorities = queue_priorities.transfer.data();
            queueInfos.insert(std::make_pair(VK_QUEUE_TRANSFER_BIT, transfer_info));
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
            auto sparse_info = SetupQueueFamily(parent->GetQueueFamilyProperties(VK_QUEUE_SPARSE_BINDING_BIT));
            sparse_info.queueFamilyIndex = QueueFamilyIndices.SparseBinding;
            NumSparseBindingQueues = sparse_info.queueCount;
            queue_priorities.sparse_binding = std::vector<float>(NumSparseBindingQueues, 1.0f);
            sparse_info.pQueuePriorities = queue_priorities.sparse_binding.data();
            queueInfos.insert(std::make_pair(VK_QUEUE_SPARSE_BINDING_BIT, sparse_info));
        }
        else {
            auto queue_properties = parent->GetQueueFamilyProperties(VK_QUEUE_SPARSE_BINDING_BIT);
            if (queue_properties.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                QueueFamilyIndices.SparseBinding = QueueFamilyIndices.Graphics;
                NumSparseBindingQueues = NumGraphicsQueues;
            }
        }
    }

    void Device::VerifyPresentationSupport() {
    
        // Check presentation support
        VkBool32 present_support = false;
        for (uint32_t i = 0; i < 3; ++i) {
            vkGetPhysicalDeviceSurfaceSupportKHR(parent->vkHandle(), i, parentInstance->vkSurface(), &present_support);
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

    VkDeviceQueueCreateInfo Device::SetupQueueFamily(const VkQueueFamilyProperties & family_properties) {
        VkDeviceQueueCreateInfo result = vk_device_queue_create_info_base;
        result.queueCount = family_properties.queueCount;
        std::vector<float> queue_priorities;
        queue_priorities.assign(result.queueCount, 1.0f);
        result.pQueuePriorities = std::move(queue_priorities.data());
        return result;
    }

    Device::~Device(){
        vkAllocator.reset();
        vkDestroyDevice(handle, nullptr);
    }

    const VkDevice & Device::vkHandle() const{
        return handle;
    }

    void Device::CheckSurfaceSupport(const VkSurfaceKHR& surf) {
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
        
        if (!queueInfos.count(VK_QUEUE_SPARSE_BINDING_BIT)) {
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
        return parent->Properties.deviceID;
    }

    const PhysicalDevice & Device::GetPhysicalDevice() const noexcept{
        return *parent;
    }

    VkPhysicalDeviceProperties Device::GetPhysicalDeviceProperties() const noexcept {
        return parent->Properties;
    }

    VkPhysicalDeviceMemoryProperties Device::GetPhysicalDeviceMemoryProperties() const noexcept {
        return parent->MemoryProperties;
    }

    bool Device::HasDedicatedComputeQueues() const {
        if (QueueFamilyIndices.Compute != QueueFamilyIndices.Graphics) {
            return true;
        }
        return false;
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

    void Device::prepareRequiredExtensions(const VprExtensionPack* extensions, std::vector<const char*>& output) const {
        std::vector<const char*> required_extensions{ extensions->RequiredExtensionNames, 
            extensions->RequiredExtensionNames + extensions->RequiredExtensionCount };
        checkExtensions(required_extensions, true);

        for (auto&& elem : required_extensions) {
            output.push_back(std::move(elem));
        }
    }

    void Device::prepareOptionalExtensions(const VprExtensionPack* extensions, std::vector<const char*>& output) const noexcept{
        std::vector<const char*> optional_extensions{ extensions->OptionalExtensionNames, 
            extensions->OptionalExtensionNames + extensions->OptionalExtensionCount };
        checkExtensions(optional_extensions, false);

        for (auto&& elem : optional_extensions) {
            output.push_back(std::move(elem));
        }

    }

    void Device::checkExtensions(std::vector<const char*>& extensions, bool throw_on_error) const {
        uint32_t queried_count = 0;
        vkEnumerateDeviceExtensionProperties(parent->vkHandle(), nullptr, &queried_count, nullptr);
        std::vector<VkExtensionProperties> queried_extensions(queried_count);
        vkEnumerateDeviceExtensionProperties(parent->vkHandle(), nullptr, &queried_count, queried_extensions.data());

        const auto has_extension = [queried_extensions](const char* name) {
            const auto req_found = std::find_if(queried_extensions.cbegin(), queried_extensions.cend(), [name](const VkExtensionProperties& p) {
                return strcmp(p.extensionName, name) == 0;
            });
            return req_found != queried_extensions.end();
        };

        auto iter = extensions.begin();
        while (iter != extensions.end()) {
            if (!has_extension(*iter)) {
                if (throw_on_error) {
                    LOG(ERROR) << "Current VkDevice does not support extension \"" << *iter << "\" that is required!";
                    throw std::runtime_error("Could not enable/use required extension for the logical device.");
                }
                else {
                    LOG(WARNING) << "Requested device extension with name \"" << *iter << "\" is not available, removing from list.";
                    extensions.erase(iter++);
                }
            }
            else {
                ++iter;
            }
        }
    }
    
    void Device::checkDedicatedAllocExtensions(const std::vector<const char*>& exts) {
        constexpr std::array<const char*, 2> mem_extensions{ VK_KHR_DEDICATED_ALLOCATION_EXTENSION_NAME, VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME };
        auto iter = std::find(exts.cbegin(), exts.cend(), mem_extensions[0]);
        if (iter != exts.cend()) {
            iter = std::find(exts.cbegin(), exts.cend(), mem_extensions[1]);
            if (iter != exts.cend()) {
                LOG(INFO) << "Both extensions required to enable better dedicated allocations have been enabled/found.";
                enableDedicatedAllocations = true;
            }    
            else {
                LOG(WARNING) << "Only one of the extensions required for better allocations was found - cannot enable/use.";
                enableDedicatedAllocations = false;
            }
        }
        else {
            enableDedicatedAllocations = false;
        }
    }


}
