#include "vpr_stdafx.h"
#include "core/LogicalDevice.hpp"
#include "core/PhysicalDevice.hpp"
#include "core/Instance.hpp"
#include "alloc/Allocator.hpp"
#include "util/easylogging++.h"

namespace vpr {

    constexpr const char* const SWAPCHAIN_EXTENSION_NAME = "VK_KHR_swapchain";
    constexpr std::array<const char*, 3> recommended_extensions {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        VK_KHR_DEDICATED_ALLOCATION_EXTENSION_NAME,
        VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME
    };

    Device::Device(const Instance* instance, const PhysicalDevice * device, bool use_recommended_extensions) : parent(device), parentInstance(instance) {
        if (use_recommended_extensions) {
            create(static_cast<uint32_t>(recommended_extensions.size()), recommended_extensions.data(), 0, nullptr);
        }
        else {
            create(0, nullptr, 0, nullptr);
        }
    }

    Device::Device(const Instance* instance, const PhysicalDevice* dvc, const uint32_t extension_count, const char* const* extension_names,
        const uint32_t layer_count, const char* const* layer_names) : parent(dvc), parentInstance(instance) {
        create(extension_count, extension_names, layer_count, layer_names);
    }

    void Device::create(const uint32_t ext_count, const char* const* exts, const uint32_t layer_count, const char* const* layers) {
        setupGraphicsQueues();
        setupComputeQueues();
        setupTransferQueues();
        setupSparseBindingQueues();

        // Local vector copy of QueueCreateInfos: need raw data ptr for createInfo,
        // and easier to insert/include the presentation queues this way.
        // also this way it's stored linearly as Vulkan expects it to be
        std::vector<VkDeviceQueueCreateInfo> queue_infos;
        for (const auto& queue_info_entry : queueInfos) {
            queue_infos.push_back(queue_info_entry.second);
        }

        VerifyPresentationSupport();

        createInfo = vk_device_create_info_base;
        createInfo.queueCreateInfoCount = static_cast<uint32_t>(queue_infos.size());
        createInfo.pQueueCreateInfos = queue_infos.data();

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

        if ((ext_count != 0) && (exts != nullptr)) {
            std::vector<const char*> req_extensions{ exts, exts + ext_count };
            checkRequestedExtensions(req_extensions);
            auto iter = std::find(req_extensions.cbegin(), req_extensions.cend(), SWAPCHAIN_EXTENSION_NAME);
            if (iter == req_extensions.cend()) {
                LOG(WARNING) << SWAPCHAIN_EXTENSION_NAME << " not requested as a device extension. Cannot render or create a swapchain object!";
            }

            checkDedicatedAllocExtensions(req_extensions);
            createInfo.enabledExtensionCount = static_cast<uint32_t>(req_extensions.size());
            createInfo.ppEnabledExtensionNames = req_extensions.data();
        }
        else {
            createInfo.enabledExtensionCount = 0;
            createInfo.ppEnabledExtensionNames = nullptr;
            enableDedicatedAllocations = false;
        }

        VkResult result = vkCreateDevice(parent->vkHandle(), &createInfo, nullptr, &handle);
        VkAssert(result);

        vkAllocator = std::make_unique<Allocator>(this, enableDedicatedAllocations);

    }

    void Device::setupGraphicsQueues() {
        QueueFamilyIndices.Graphics = parent->GetQueueFamilyIndex(VK_QUEUE_GRAPHICS_BIT);
        auto create_info = SetupQueueFamily(parent->GetQueueFamilyProperties(VK_QUEUE_GRAPHICS_BIT));
        create_info.queueFamilyIndex = QueueFamilyIndices.Graphics;
        NumGraphicsQueues = create_info.queueCount;
        const std::vector<float> priorities(NumGraphicsQueues, 1.0f);
        create_info.pQueuePriorities = priorities.data();
        queueInfos.insert(std::make_pair(VK_QUEUE_GRAPHICS_BIT, create_info));
    }

    void Device::setupComputeQueues() {
        QueueFamilyIndices.Compute = parent->GetQueueFamilyIndex(VK_QUEUE_COMPUTE_BIT);
        if (QueueFamilyIndices.Compute != QueueFamilyIndices.Graphics) {
            auto compute_info = SetupQueueFamily(parent->GetQueueFamilyProperties(VK_QUEUE_COMPUTE_BIT));
            compute_info.queueFamilyIndex = QueueFamilyIndices.Compute;
            NumComputeQueues = compute_info.queueCount;
            const std::vector<float> priorities(NumComputeQueues, 1.0f);
            compute_info.pQueuePriorities = priorities.data();
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
            const std::vector<float> priorities(NumTransferQueues, 1.0f);
            transfer_info.pQueuePriorities = priorities.data();
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
            const std::vector<float> priorities(NumSparseBindingQueues, 1.0f);
            sparse_info.pQueuePriorities = priorities.data();
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

        LOG_IF(QueueFamilyIndices.Compute == QueueFamilyIndices.Graphics, INFO) << "Retrieving queue that supports transfer ops, but isn't dedicated transfer queue.";

        assert(idx < NumTransferQueues);
        VkQueue result;
        vkGetDeviceQueue(vkHandle(), QueueFamilyIndices.Transfer, idx, &result);
        assert(result != VK_NULL_HANDLE);
        return result;

    }

    VkQueue Device::ComputeQueue(const uint32_t & idx) const{

        LOG_IF(QueueFamilyIndices.Compute == QueueFamilyIndices.Graphics, INFO) << "Retrieving queue that supports compute ops, but isn't dedicated compute queue.";
        assert(idx < NumComputeQueues);
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

        assert(idx < NumSparseBindingQueues);
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
            assert(properties.linearTilingFeatures & flags);
            return VK_IMAGE_TILING_LINEAR;
        }
    }

    VkFormat Device::FindSupportedFormat(const std::vector<VkFormat>& options, const VkImageTiling & tiling, const VkFormatFeatureFlags & flags) const {
        for (const auto& fmt : options) {
            VkFormatProperties properties;
            vkGetPhysicalDeviceFormatProperties(parent->vkHandle(), fmt, &properties);
            if (tiling == VK_IMAGE_TILING_LINEAR && (properties.linearTilingFeatures & flags) == flags) {
                return fmt;
            }
            else if (tiling == VK_IMAGE_TILING_OPTIMAL && (properties.optimalTilingFeatures & flags) == flags) {
                return fmt;
            }
        }
        LOG(ERROR) << "Could not find texture format that supports requested tiling and feature flags: ( " << std::to_string(tiling) << " , " << std::to_string(flags) << " )";
        throw std::runtime_error("Could not find valid texture format.");
    }

    VkFormat Device::FindDepthFormat() const{
        return FindSupportedFormat({ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT }, VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
    }

    VkFormat Device::GetSwapchainColorFormat() const {
        return VK_FORMAT_B8G8R8A8_UNORM;
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

    void Device::checkRequestedExtensions(std::vector<const char*>& extensions) {
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
                LOG(WARNING) << "Requested device extension with name \"" << *iter << "\" is not available, removing from list.";
                extensions.erase(iter++);
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
