#include "vpr_stdafx.h"
#include "PhysicalDevice.hpp"
#include "easylogging++.h"
#include <set>
#include <variant>

namespace vpr
{

    static std::map<int32_t, VkPhysicalDevice> physicalDevices;

    // Gets best available physical device and removes it from the map, as it's no longer available.
    static VkPhysicalDevice GetBestAvailPhysicalDevice()
    {
        auto best_device = (physicalDevices.rbegin());
        VkPhysicalDevice result = best_device->second;
        physicalDevices.erase(best_device->first);
        return result;
    }

    static inline int32_t ScoreDevice(const VkPhysicalDevice& dvc, const uint32_t apiVersion)
    {
        int32_t score = 0;
        VkPhysicalDeviceFeatures features;
        VkPhysicalDeviceProperties properties;
        if (apiVersion == VK_API_VERSION_1_0)
        {
            vkGetPhysicalDeviceFeatures(dvc, &features);
            vkGetPhysicalDeviceProperties(dvc, &properties);
        }
        else if (apiVersion > VK_API_VERSION_1_0)
        {
            VkPhysicalDeviceFeatures2 features2
            {
                VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
                nullptr,
                VkPhysicalDeviceFeatures{}
            };

            vkGetPhysicalDeviceFeatures2(dvc, &features2);
            features = features2.features;

            VkPhysicalDeviceProperties2 properties2
            {
                VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2,
                nullptr,
                VkPhysicalDeviceProperties{}
            };

            vkGetPhysicalDeviceProperties2(dvc, &properties2);
            properties = properties2.properties;

        }
        else
        {
            return std::numeric_limits<int32_t>::min();
        }

        if (!features.geometryShader)
        {
            score -= 1000;
        }

        if (!features.tessellationShader)
        {
            score -= 1000;
        }

        if (!features.samplerAnisotropy)
        {
            score -= 250;
        }

        if (!features.imageCubeArray)
        {
            score -= 250;
        }

        if (!features.fullDrawIndexUint32)
        {
            score -= 500;
        }

        if (features.multiDrawIndirect)
        {
            score += 250;
        }

        if (features.textureCompressionETC2)
        {
            score += 250;
        }

        if (features.textureCompressionASTC_LDR)
        {
            score += 250;
        }

        score += (10 * properties.limits.maxBoundDescriptorSets);
        score += (5 * 
            ( 
                properties.limits.maxPerStageDescriptorUniformBuffers +
                properties.limits.maxPerStageDescriptorSampledImages +
                properties.limits.maxPerStageDescriptorSamplers +
                properties.limits.maxPerStageDescriptorStorageImages
            )
        );

        if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
        {
            score += 3000;
        }

        auto tex_sizes =
        {
            properties.limits.maxImageDimension1D,
            properties.limits.maxImageDimension2D,
            properties.limits.maxImageDimension3D,
        };

        for (auto& sz : tex_sizes)
        {
            score += sz / 100;
        }

        return score;

    }

    void PopulatePhysicalDeviceMap(const VkInstance& parent_instance, const uint32_t apiVersion)
    {

        // Enumerate devices.
        uint32_t dvc_cnt = 0;
        vkEnumeratePhysicalDevices(parent_instance, &dvc_cnt, nullptr);
        std::vector<VkPhysicalDevice> devices(dvc_cnt);
        vkEnumeratePhysicalDevices(parent_instance, &dvc_cnt, devices.data());

        for (const auto& dvc : devices)
        {
            physicalDevices.emplace(ScoreDevice(dvc, apiVersion), dvc);
        }

    }

    // Properties for devices created on Vulkan 1.0 instances
    struct VulkanBasePhysicalDeviceProps
    {
        VkPhysicalDeviceProperties Properties;
        VkPhysicalDeviceFeatures Features;
        VkPhysicalDeviceMemoryProperties MemoryProperties;
        VkPhysicalDeviceSubgroupProperties SubgroupProperties;
        std::vector<VkQueueFamilyProperties> QueueFamilyProperties;
    };

    // Properties for devices created on Vulkan 1.0+ instances
    struct VulkanEdgePhysicalDeviceProps
    {
        VkPhysicalDeviceProperties2 Properties;
        VkPhysicalDeviceFeatures Features;
        VkPhysicalDeviceMemoryProperties2 MemoryProperties;
        VkPhysicalDeviceSubgroupProperties SubgroupProperties;
        std::vector<VkQueueFamilyProperties2> QueueFamilyProperties;
    };

    class PhysicalDeviceImpl
    {
        PhysicalDeviceImpl(const PhysicalDeviceImpl&) = delete;
        PhysicalDeviceImpl& operator=(const PhysicalDeviceImpl&) = delete;
    public:

        PhysicalDeviceImpl(const VkInstance& instance);
        PhysicalDeviceImpl(PhysicalDeviceImpl&& other) noexcept;
        PhysicalDeviceImpl& operator=(PhysicalDeviceImpl&& other) noexcept;
        ~PhysicalDeviceImpl();

        uint32_t GetMemoryTypeIdx(const uint32_t type_bitfield, const VkMemoryPropertyFlags property_flags, VkBool32* memory_type_found) const noexcept;
        
        void getAttributes() noexcept;
        void retrieveQueueFamilyProperties() noexcept;

        uint32_t GetQueueFamilyIndex(const VkQueueFlagBits queue_bits) const noexcept;

        
        VkPhysicalDevice handle{ VK_NULL_HANDLE };
        // set at creation time based on current installed instance version and hardware support
        uint32_t apiVersion{ 0u };
        std::variant<VulkanBasePhysicalDeviceProps, VulkanEdgePhysicalDeviceProps> deviceProperties;
    };

    PhysicalDeviceImpl::PhysicalDeviceImpl(const VkInstance& instance)
    {

        uint32_t apiVersion = 0u;
        vkEnumerateInstanceVersion(&apiVersion);

        if (physicalDevices.empty())
        {
            PopulatePhysicalDeviceMap(instance, apiVersion);
        }

        handle = GetBestAvailPhysicalDevice();

        getAttributes();
        retrieveQueueFamilyProperties();
    }

    PhysicalDeviceImpl::PhysicalDeviceImpl(PhysicalDeviceImpl&& other) noexcept
        : handle(std::move(other.handle)), deviceProperties(std::move(other.deviceProperties))
    {
        other.handle = VK_NULL_HANDLE;
    }

    PhysicalDeviceImpl& PhysicalDeviceImpl::operator=(PhysicalDeviceImpl&& other) noexcept
    {
        deviceProperties = std::move(other.deviceProperties);
        handle = std::move(other.handle);
        other.handle = VK_NULL_HANDLE;
        return *this;
    }

    PhysicalDeviceImpl::~PhysicalDeviceImpl() {
    }

    uint32_t PhysicalDeviceImpl::GetMemoryTypeIdx(const uint32_t type_bitfield, const VkMemoryPropertyFlags property_flags, VkBool32* memory_type_found) const noexcept
    {
        if (std::holds_alternative<VulkanBasePhysicalDeviceProps>(deviceProperties))
        {
            auto bitfield = type_bitfield;
            auto& MemoryProperties = std::get<VulkanBasePhysicalDeviceProps>(deviceProperties).MemoryProperties;
            const uint32_t num_memory_types = MemoryProperties.memoryTypeCount;

            for (uint32_t i = 0; i < num_memory_types; ++i)
            {
                if (bitfield & 1)
                {
                    // check if property flags match
                    if ((MemoryProperties.memoryTypes[i].propertyFlags & property_flags) == property_flags)
                    {
                        if (memory_type_found)
                        {
                            *memory_type_found = true;
                        }
                        return i;
                    }
                }
                bitfield >>= 1;
            }
        }
        else if (std::holds_alternative<VulkanEdgePhysicalDeviceProps>(deviceProperties))
        {
            auto bitfield = type_bitfield;
            auto& MemoryProperties = std::get<VulkanEdgePhysicalDeviceProps>(deviceProperties).MemoryProperties;
            const uint32_t numMemoryTypes = MemoryProperties.memoryProperties.memoryTypeCount;

            for (uint32_t i = 0; i < numMemoryTypes; ++i)
            {
                if (bitfield & 1)
                {
                    if ((MemoryProperties.memoryProperties.memoryTypes[i].propertyFlags & property_flags) == property_flags)
                    {
                        if (memory_type_found)
                        {
                            *memory_type_found = true;
                        }
                        return i;
                    }
                }
                bitfield >>= 1;
            }
        }
        
        return std::numeric_limits<uint32_t>::max();
    }

    PhysicalDevice::PhysicalDevice(const VkInstance& handle) : impl(std::make_unique<PhysicalDeviceImpl>(handle)) {}

    PhysicalDevice::PhysicalDevice(PhysicalDevice&& other) noexcept : impl(std::move(other.impl))
    {
        other.impl.reset();
    }

    PhysicalDevice & PhysicalDevice::operator=(PhysicalDevice && other) noexcept
    {
        impl = std::move(other.impl);
        other.impl.reset();
        return *this;
    }

    PhysicalDevice::~PhysicalDevice() {}

    void PhysicalDeviceImpl::getAttributes() noexcept
    {
        vkGetPhysicalDeviceProperties(handle, &Properties);
        vkGetPhysicalDeviceFeatures(handle, &Features);
        vkGetPhysicalDeviceMemoryProperties(handle, &MemoryProperties);
    }

    void PhysicalDeviceImpl::retrieveQueueFamilyProperties() noexcept
    {
        uint32_t queue_family_cnt = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(handle, &queue_family_cnt, nullptr);
        queueFamilyProperties.resize(queue_family_cnt);
        vkGetPhysicalDeviceQueueFamilyProperties(handle, &queue_family_cnt, queueFamilyProperties.data());
    }


    uint32_t PhysicalDevice::GetMemoryTypeIdx(const uint32_t type_bitfield, const VkMemoryPropertyFlags property_flags, VkBool32* memory_type_found) const noexcept
    {
        return impl->GetMemoryTypeIdx(type_bitfield, property_flags, memory_type_found);
    }

    uint32_t PhysicalDeviceImpl::GetQueueFamilyIndex(const VkQueueFlagBits bitfield) const noexcept
    {
        if (bitfield & VK_QUEUE_COMPUTE_BIT)
        {
            for (uint32_t i = 0; i < static_cast<uint32_t>(queueFamilyProperties.size()); ++i)
            {
                if ((queueFamilyProperties[i].queueFlags & bitfield) && ((queueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0))
                {
                    return i;
                }
            }
        }

        if (bitfield & VK_QUEUE_TRANSFER_BIT)
        {
            for (uint32_t i = 0; i < static_cast<uint32_t>(queueFamilyProperties.size()); ++i)
            {
                if ((queueFamilyProperties[i].queueFlags & bitfield) && ((queueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0) && ((queueFamilyProperties[i].queueFlags & VK_QUEUE_COMPUTE_BIT) == 0))
                {
                    return i;
                }
            }
        }

        for (uint32_t i = 0; i < static_cast<uint32_t>(queueFamilyProperties.size()); ++i)
        {
            if (queueFamilyProperties[i].queueFlags & bitfield)
            {
                return i;
            }
        }

        LOG(WARNING) << "Failed to find desired queue family with given flags.";
        return std::numeric_limits<uint32_t>::max();
    }

    uint32_t PhysicalDevice::GetQueueFamilyIndex(const VkQueueFlagBits bitfield) const noexcept
    {
        return impl->GetQueueFamilyIndex(bitfield);
    }

    VkQueueFamilyProperties PhysicalDevice::GetQueueFamilyProperties(const VkQueueFlagBits bitfield) const
    {

        uint32_t idx = GetQueueFamilyIndex(bitfield);
        if (idx == std::numeric_limits<uint32_t>::max())
        {
            LOG(WARNING) << "Failed to retrieve queue family properties: couldn't find queue family with given bitfield.";
            return VkQueueFamilyProperties();
        }
        else
        {
            return impl->queueFamilyProperties[idx];
        }
        
    }

    const VkPhysicalDeviceProperties& PhysicalDevice::GetProperties() const noexcept
    {
        return impl->Properties;
    }

    const VkPhysicalDeviceFeatures& PhysicalDevice::GetFeatures() const noexcept
    {
        return impl->Features;
    }

    const VkPhysicalDeviceMemoryProperties& PhysicalDevice::GetMemoryProperties() const noexcept
    {
        return impl->MemoryProperties;
    }

    const VkPhysicalDeviceSubgroupProperties& PhysicalDevice::GetSubgroupProperties() const noexcept
    {
        return impl->SubgroupProperties;
    }

    const VkPhysicalDevice& PhysicalDevice::vkHandle() const noexcept
    {
        return impl->handle;
    }

}
