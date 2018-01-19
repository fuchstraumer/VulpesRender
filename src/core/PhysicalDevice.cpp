#include "vpr_stdafx.h"
#include "core/PhysicalDevice.hpp"
#include "util/easylogging++.h"

bool check_required_features(const VkPhysicalDeviceFeatures& req, const VkPhysicalDeviceFeatures& supported) {
    if (req.robustBufferAccess) {
        return supported.robustBufferAccess;
    }
    if (req.fullDrawIndexUint32) {
        return supported.fullDrawIndexUint32;
    }
    if (req.imageCubeArray) {
        return supported.imageCubeArray;
    }
    if (req.independentBlend) {
        return supported.independentBlend;
    }
    if (req.geometryShader) {
        return supported.geometryShader;
    }
    if (req.tessellationShader) {
        return supported.tessellationShader;
    }
    if (req.sampleRateShading) {
        return supported.sampleRateShading;
    }
    if (req.dualSrcBlend) {
        return supported.dualSrcBlend;
    }
    if (req.logicOp) {
        return supported.logicOp;
    }
    if (req.multiDrawIndirect) {
        return supported.multiDrawIndirect;
    }
    if (req.drawIndirectFirstInstance) {
        return supported.drawIndirectFirstInstance;
    }
    if (req.depthClamp) {
        return supported.depthClamp;
    }
    if (req.depthBiasClamp) {
        return supported.depthBiasClamp;
    }
    if (req.fillModeNonSolid) {
        return supported.fillModeNonSolid;
    }
    if (req.depthBounds) {
        return supported.depthBounds;
    }
    if (req.wideLines) {
        return supported.wideLines;
    }
    if (req.largePoints) {
        return supported.largePoints;
    }
    if (req.alphaToOne) {
        return supported.alphaToOne;
    }
    if (req.multiViewport) {
        return supported.multiViewport;
    }
    if (req.samplerAnisotropy) {
        return supported.samplerAnisotropy;
    }
    if (req.textureCompressionETC2) {
        return supported.textureCompressionETC2;
    }
    return true;
}

namespace vpr {

    PhysicalDevice::PhysicalDevice(const VkInstance& instance_handle) {

        handle = std::move(getBestDevice(instance_handle));

        getAttributes();
        retrieveQueueFamilyProperties();

    }

    PhysicalDevice::PhysicalDevice(const VkInstance& handle, const VkPhysicalDeviceFeatures& req_features) : 
        handle(std::move(getBestDevice(handle))) {
        
        getAttributes();
        if (check_required_features(req_features, Features)) {
            LOG(ERROR) << "Current physical device doesn't support all required features!";
            throw std::runtime_error("Incompatible physical device, features required are not available.");
        }
        retrieveQueueFamilyProperties();
        
    }

    void PhysicalDevice::getAttributes() noexcept {

        vkGetPhysicalDeviceProperties(handle, &Properties);
        vkGetPhysicalDeviceFeatures(handle, &Features);
        vkGetPhysicalDeviceMemoryProperties(handle, &MemoryProperties);

    }

    void PhysicalDevice::retrieveQueueFamilyProperties() noexcept {

        uint32_t queue_family_cnt = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(handle, &queue_family_cnt, nullptr);
        queueFamilyProperties.resize(queue_family_cnt);
        vkGetPhysicalDeviceQueueFamilyProperties(handle, &queue_family_cnt, queueFamilyProperties.data());

    }

    static inline uint32_t ScoreDevice(const VkPhysicalDevice& dvc) {

        uint32_t score = 0;
        VkPhysicalDeviceFeatures features;
        VkPhysicalDeviceProperties properties;
        vkGetPhysicalDeviceFeatures(dvc, &features);
        vkGetPhysicalDeviceProperties(dvc, &properties);

        if (!features.geometryShader || !features.tessellationShader) {
            score -= 500;
        }

        if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
            score += 1000;
        }

        auto tex_sizes = {
            properties.limits.maxImageDimension1D,
            properties.limits.maxImageDimension2D,
            properties.limits.maxImageDimension3D,
        };

        for (auto& sz : tex_sizes) {
            score += sz / 100;
        }

        return score;

    }

    VkPhysicalDevice PhysicalDevice::getBestDevice(const VkInstance & parent_instance) {

        // Enumerate devices.
        uint32_t dvc_cnt = 0;
        vkEnumeratePhysicalDevices(parent_instance, &dvc_cnt, nullptr);
        std::vector<VkPhysicalDevice> devices(dvc_cnt);
        vkEnumeratePhysicalDevices(parent_instance, &dvc_cnt, devices.data());

        std::map<uint32_t, VkPhysicalDevice> physical_devices;

        for (const auto& dvc : devices) {
            physical_devices.insert(std::make_pair(ScoreDevice(dvc), dvc));
        }

        // Get best device handle.
        uint32_t best_score = 0;
        VkPhysicalDevice best_device = VK_NULL_HANDLE;

        for (const auto& dvc_iter : physical_devices) {
            if (dvc_iter.first > best_score) {
                best_score = dvc_iter.first;
                best_device = dvc_iter.second;
            }
        }

        assert(best_score > 0 && best_device != VK_NULL_HANDLE);

        return best_device;

    }

    uint32_t PhysicalDevice::GetMemoryTypeIdx(const uint32_t & type_bitfield, const VkMemoryPropertyFlags & property_flags, VkBool32 * memory_type_found) const noexcept {
        auto bitfield = type_bitfield;

        for (uint32_t i = 0; i < MemoryProperties.memoryTypeCount; ++i) {
            if (bitfield & 1) {
                // check if property flags match
                if ((MemoryProperties.memoryTypes[i].propertyFlags & property_flags) == property_flags) {
                    if (memory_type_found) {
                        *memory_type_found = true;
                    }
                    return i;
                }
            }
            bitfield >>= 1;
        }

        LOG(WARNING) << "Failed to find suitable memory type index.";
        return std::numeric_limits<uint32_t>::max();

    }

    uint32_t PhysicalDevice::GetQueueFamilyIndex(const VkQueueFlagBits & bitfield) const noexcept {
        if (bitfield & VK_QUEUE_COMPUTE_BIT) {
            for (uint32_t i = 0; i < static_cast<uint32_t>(queueFamilyProperties.size()); ++i) {
                if ((queueFamilyProperties[i].queueFlags & bitfield) && ((queueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0)) {
                    return i;
                }
            }
        }

        if (bitfield & VK_QUEUE_TRANSFER_BIT) {
            for (uint32_t i = 0; i < static_cast<uint32_t>(queueFamilyProperties.size()); ++i) {
                if ((queueFamilyProperties[i].queueFlags & bitfield) && ((queueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0) && ((queueFamilyProperties[i].queueFlags & VK_QUEUE_COMPUTE_BIT) == 0)) {
                    return i;
                }
            }
        }

        for (uint32_t i = 0; i < static_cast<uint32_t>(queueFamilyProperties.size()); ++i) {
            if (queueFamilyProperties[i].queueFlags & bitfield) {
                return i;
            }
        }

        LOG(WARNING) << "Failed to find desired queue family with given flags.";
        return std::numeric_limits<uint32_t>::max();
    }

    VkQueueFamilyProperties PhysicalDevice::GetQueueFamilyProperties(const VkQueueFlagBits & bitfield) const{

        uint32_t idx = GetQueueFamilyIndex(bitfield);
        if (idx == std::numeric_limits<uint32_t>::max()) {
            LOG(WARNING) << "Failed to retrieve queue family properties: couldn't find queue family with given bitfield.";
            return VkQueueFamilyProperties();
        }
        else {
            return queueFamilyProperties[idx];
        }
        
    }

    const VkPhysicalDevice & PhysicalDevice::vkHandle() const noexcept{
        return handle;
    }

}