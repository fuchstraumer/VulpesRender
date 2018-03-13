#include "vpr_stdafx.h"
#include "core/PhysicalDevice.hpp"
#include "util/easylogging++.h"
#include <set>

namespace vpr {

    static std::map<int32_t, VkPhysicalDevice> physicalDevices;

    // Gets best available physical device and removes it from the map, as it's no longer available.
    static VkPhysicalDevice GetBestAvailPhysicalDevice() {
        auto best_device = (physicalDevices.rbegin());
        VkPhysicalDevice result = best_device->second;
        physicalDevices.erase(best_device->first);
        return result;
    }

    static inline int32_t ScoreDevice(const VkPhysicalDevice& dvc) {

        int32_t score = 0;
        VkPhysicalDeviceFeatures features;
        VkPhysicalDeviceProperties properties;
        vkGetPhysicalDeviceFeatures(dvc, &features);
        vkGetPhysicalDeviceProperties(dvc, &properties);

        if (!features.geometryShader) {
            score -= 1000;
        }

        if (!features.tessellationShader) {
            score -= 1000;
        }

        if (!features.samplerAnisotropy) {
            score -= 250;
        }

        if (!features.imageCubeArray) {
            score -= 250;
        }

        if (!features.fullDrawIndexUint32) {
            score -= 500;
        }

        if (features.multiDrawIndirect) {
            score += 250;
        }

        if (features.textureCompressionETC2) {
            score += 250;
        }

        if (features.textureCompressionASTC_LDR) {
            score += 250;
        }

        score += (10 * properties.limits.maxBoundDescriptorSets);
        score += (5 * ( 
            properties.limits.maxPerStageDescriptorUniformBuffers +
            properties.limits.maxPerStageDescriptorSampledImages +
            properties.limits.maxPerStageDescriptorSamplers +
            properties.limits.maxPerStageDescriptorStorageImages
            ));

        if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
            score += 1000;
        }

        auto tex_sizes = {
            properties.limits.maxImageDimension1D,
            properties.limits.maxImageDimension2D,
            properties.limits.maxImageDimension3D,
        };

        for (auto& sz : tex_sizes) {
            score += sz / 1000;
        }

        return score;

    }

    void PopulatePhysicalDeviceMap(const VkInstance & parent_instance) {

        // Enumerate devices.
        uint32_t dvc_cnt = 0;
        vkEnumeratePhysicalDevices(parent_instance, &dvc_cnt, nullptr);
        std::vector<VkPhysicalDevice> devices(dvc_cnt);
        vkEnumeratePhysicalDevices(parent_instance, &dvc_cnt, devices.data());

        for (const auto& dvc : devices) {
            physicalDevices.emplace(ScoreDevice(dvc), dvc);
        }

    }


    class PhysicalDeviceImpl {
        PhysicalDeviceImpl(const PhysicalDeviceImpl&) = delete;
        PhysicalDeviceImpl& operator=(const PhysicalDeviceImpl&) = delete;
    public:

        PhysicalDeviceImpl(const VkInstance& instance);
        PhysicalDeviceImpl(PhysicalDeviceImpl&& other) noexcept;
        PhysicalDeviceImpl& operator=(PhysicalDeviceImpl&& other) noexcept;
        ~PhysicalDeviceImpl();

        uint32_t GetMemoryTypeIdx(const uint32_t & type_bitfield, const VkMemoryPropertyFlags & property_flags, VkBool32 * memory_type_found) const noexcept;
        
        void getAttributes() noexcept;
        void retrieveQueueFamilyProperties() noexcept;

        uint32_t GetQueueFamilyIndex(const VkQueueFlagBits & queue_bits) const noexcept;

        VkPhysicalDeviceProperties Properties;
        VkPhysicalDeviceFeatures Features;
        VkPhysicalDeviceMemoryProperties MemoryProperties;
        VkPhysicalDeviceSubgroupProperties SubgroupProperties;
        std::vector<VkQueueFamilyProperties> queueFamilyProperties;
        VkPhysicalDevice handle;
    };

    PhysicalDeviceImpl::PhysicalDeviceImpl(const VkInstance& instance) {

        if (physicalDevices.empty()) {
            PopulatePhysicalDeviceMap(instance);
        }

        handle = GetBestAvailPhysicalDevice();

        getAttributes();
        retrieveQueueFamilyProperties();
    }

    PhysicalDeviceImpl::PhysicalDeviceImpl(PhysicalDeviceImpl && other) noexcept : Properties(std::move(other.Properties)), Features(std::move(other.Features)), MemoryProperties(std::move(other.MemoryProperties)),
        queueFamilyProperties(std::move(other.queueFamilyProperties)), SubgroupProperties(std::move(other.SubgroupProperties)),
        handle(std::move(other.handle)) { other.handle = VK_NULL_HANDLE; }

    PhysicalDeviceImpl & PhysicalDeviceImpl::operator=(PhysicalDeviceImpl && other) noexcept {
        Properties = std::move(other.Properties);
        Features = std::move(other.Features);
        MemoryProperties = std::move(other.MemoryProperties);
        queueFamilyProperties = std::move(other.queueFamilyProperties);
        SubgroupProperties = std::move(other.SubgroupProperties);
        handle = std::move(other.handle);
        return *this;
    }

    PhysicalDeviceImpl::~PhysicalDeviceImpl() {
    }

    uint32_t PhysicalDeviceImpl::GetMemoryTypeIdx(const uint32_t & type_bitfield, const VkMemoryPropertyFlags & property_flags, VkBool32 * memory_type_found) const noexcept {
        auto bitfield = type_bitfield;
        const uint32_t num_memory_types = MemoryProperties.memoryTypeCount;

        for (uint32_t i = 0; i < num_memory_types; ++i) {
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

    PhysicalDevice::PhysicalDevice(const VkInstance& handle) : impl(std::make_unique<PhysicalDeviceImpl>(handle)) {}

    PhysicalDevice::PhysicalDevice(PhysicalDevice && other) noexcept : impl(std::move(other.impl)) {
        other.impl.reset();
    }

    PhysicalDevice & PhysicalDevice::operator=(PhysicalDevice && other) noexcept {
        impl = std::move(other.impl);
        other.impl.reset();
        return *this;
    }

    PhysicalDevice::~PhysicalDevice() { }

    void PhysicalDeviceImpl::getAttributes() noexcept {
        vkGetPhysicalDeviceProperties(handle, &Properties);
        vkGetPhysicalDeviceFeatures(handle, &Features);
        vkGetPhysicalDeviceMemoryProperties(handle, &MemoryProperties);
        VkPhysicalDeviceProperties2 properties2{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2, &SubgroupProperties, Properties };
        vkGetPhysicalDeviceProperties2(handle, &properties2);
    }

    void PhysicalDeviceImpl::retrieveQueueFamilyProperties() noexcept {
        uint32_t queue_family_cnt = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(handle, &queue_family_cnt, nullptr);
        queueFamilyProperties.resize(queue_family_cnt);
        vkGetPhysicalDeviceQueueFamilyProperties(handle, &queue_family_cnt, queueFamilyProperties.data());
    }


    uint32_t PhysicalDevice::GetMemoryTypeIdx(const uint32_t & type_bitfield, const VkMemoryPropertyFlags & property_flags, VkBool32 * memory_type_found) const noexcept {
        return impl->GetMemoryTypeIdx(type_bitfield, property_flags, memory_type_found);
    }

    uint32_t PhysicalDeviceImpl::GetQueueFamilyIndex(const VkQueueFlagBits& bitfield) const noexcept {
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

    uint32_t PhysicalDevice::GetQueueFamilyIndex(const VkQueueFlagBits & bitfield) const noexcept {
        return impl->GetQueueFamilyIndex(bitfield);
    }

    VkQueueFamilyProperties PhysicalDevice::GetQueueFamilyProperties(const VkQueueFlagBits & bitfield) const{

        uint32_t idx = GetQueueFamilyIndex(bitfield);
        if (idx == std::numeric_limits<uint32_t>::max()) {
            LOG(WARNING) << "Failed to retrieve queue family properties: couldn't find queue family with given bitfield.";
            return VkQueueFamilyProperties();
        }
        else {
            return impl->queueFamilyProperties[idx];
        }
        
    }

    const VkPhysicalDeviceProperties & PhysicalDevice::GetProperties() const noexcept {
        return impl->Properties;
    }

    const VkPhysicalDeviceFeatures & PhysicalDevice::GetFeatures() const noexcept {
        return impl->Features;
    }

    const VkPhysicalDeviceMemoryProperties & PhysicalDevice::GetMemoryProperties() const noexcept {
        return impl->MemoryProperties;
    }

    const VkPhysicalDeviceSubgroupProperties& PhysicalDevice::GetSubgroupProperties() const noexcept {
        return impl->SubgroupProperties;
    }

    const VkPhysicalDevice & PhysicalDevice::vkHandle() const noexcept{
        return impl->handle;
    }

}