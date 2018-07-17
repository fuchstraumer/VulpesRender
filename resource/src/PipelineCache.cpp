#include "vpr_stdafx.h"
#include "PipelineCache.hpp"
#include "easylogging++.h"
INITIALIZE_EASYLOGGINGPP
#include "vkAssert.hpp"
#ifdef USE_EXPERIMENTAL_FILESYSTEM
#include <experimental/filesystem>
#endif

namespace vpr {

    void SetLoggingRepository_VprResource(void* repo) {
        LOG(INFO) << "Updating easyloggingpp storage pointer in vpr_resource module...";
        el::Helpers::setStorage(*(el::base::type::StoragePointer*)repo);
    }
    
    constexpr static VkPipelineCacheCreateInfo base_create_info{ VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO, nullptr, 0, 0, nullptr };

    PipelineCache::PipelineCache(const VkDevice& _parent, const VkPhysicalDevice& host_device, const size_t hash_id) : parent(_parent), createInfo(base_create_info), 
        hostPhysicalDevice(host_device), hashID(std::move(hash_id)), handle(VK_NULL_HANDLE) {
        
        std::string cache_dir = std::string("./rsrc/shader_cache/");
            
#ifdef USE_EXPERIMENTAL_FILESYSTEM
        std::experimental::filesystem::path cache_path(cache_dir);

        if (!std::experimental::filesystem::exists(cache_path)) {
            LOG(INFO) << "Shader cache path didn't exist, creating...";
            std::experimental::filesystem::create_directories(cache_path);
        }
#endif
            
        std::string fname = cache_dir + std::to_string(hash_id) + std::string(".vkdat");
        filename = _strdup(fname.c_str());

        // Attempts to load cache from file: if failed, doesn't matter much.
        LoadCacheFromFile(fname.c_str());

        VkResult result = vkCreatePipelineCache(parent, &createInfo, nullptr, &handle);
        VkAssert(result);

    }

    PipelineCache::~PipelineCache() {
        if (handle != VK_NULL_HANDLE) {
            VkResult saved = saveToFile();
            VkAssert(saved);
            vkDestroyPipelineCache(parent, handle, nullptr);
        }

        if (filename) {
            free(filename);
        }
    }

    PipelineCache::PipelineCache(PipelineCache&& other) noexcept : parent(std::move(other.parent)), createInfo(std::move(other.createInfo)),
        hashID(std::move(other.hashID)), handle(std::move(other.handle)), filename(std::move(other.filename)) {
        other.handle = VK_NULL_HANDLE; 
        other.filename = nullptr;
    }

    PipelineCache& PipelineCache::operator=(PipelineCache&& other) noexcept {
        parent = std::move(other.parent);
        createInfo = std::move(other.createInfo);
        hashID = std::move(other.hashID);
        handle = std::move(other.handle);
        other.handle = VK_NULL_HANDLE;
        filename = std::move(other.filename);
        other.filename = nullptr;
        return *this;
    }
 
    bool PipelineCache::Verify(const int8_t* cache_header) const {

        const uint32_t* header = reinterpret_cast<const uint32_t*>(cache_header);
        
        if (header[0] != 32) {
            return false;
        }

        if (header[1] != static_cast<uint32_t>(VK_PIPELINE_CACHE_HEADER_VERSION_ONE)) {
            return false;
        }

        VkPhysicalDeviceProperties properties;
        vkGetPhysicalDeviceProperties(hostPhysicalDevice, &properties);

        if (header[2] != properties.vendorID) {
            return false;
        }

        if (header[3] != properties.deviceID) {
            return false;
        }

        uint8_t cache_uuid[VK_UUID_SIZE] = {};
        memcpy(cache_uuid, cache_header + 16, VK_UUID_SIZE);

        if (memcmp(cache_uuid, properties.pipelineCacheUUID, sizeof(cache_uuid)) != 0) {
            LOG(WARNING) << "Pipeline cache UUID incorrect, requires rebuilding.";
            return false;
        }

        return true;
    }

    void PipelineCache::LoadCacheFromFile(const char * _filename) {
        /*
        check for pre-existing cache file.
        */
        std::ifstream cache(_filename, std::ios::in);
        if (cache) {

            // get header (4 uint32_t, 16 int8_t) = (32 int8_t)
            std::vector<int8_t> header(64);
            cache.get(reinterpret_cast<char*>(header.data()), 64);

            // Check to see if header data matches current device.
            if (Verify(header.data())) {
                LOG_IF(VERBOSE_LOGGING, INFO) << "Found valid pipeline cache data with ID # " << std::to_string(hashID) << " .";
                std::string cache_str((std::istreambuf_iterator<char>(cache)), std::istreambuf_iterator<char>());
                uint32_t cache_size = static_cast<uint32_t>(cache_str.size() * sizeof(char));
                createInfo.initialDataSize = cache_size;
                createInfo.pInitialData = cache_str.data();
            }
            else {
                LOG_IF(VERBOSE_LOGGING, INFO) << "Pre-existing cache file isn't valid: creating new pipeline cache.";
                createInfo.initialDataSize = 0;
                createInfo.pInitialData = nullptr;
            }
        }
    }

    const VkPipelineCache& PipelineCache::vkHandle() const{
        return handle;
    }

    void PipelineCache::MergeCaches(const uint32_t num_caches, const VkPipelineCache* caches) {
        vkMergePipelineCaches(parent, handle, num_caches, caches);
    }
    
    VkResult PipelineCache::saveToFile() const {

        VkResult result = VK_SUCCESS;
        size_t cache_size;

        if (!parent) {
            LOG(ERROR) << "Attempted to delete/save a non-existent cache!";
            return VK_ERROR_DEVICE_LOST;
        }

        // works like enumerate calls: get size first, then use size to get data.
        result = vkGetPipelineCacheData(parent, handle, &cache_size, nullptr);
        VkAssert(result);

        if (cache_size != 0) {
            try {
                std::ofstream file(filename, std::ios::out | std::ios::trunc);

                void* cache_data;
                cache_data = malloc(cache_size);

                result = vkGetPipelineCacheData(parent, handle, &cache_size, cache_data);
                VkAssert(result);

                file.write(reinterpret_cast<const char*>(cache_data), cache_size);
                file.close();

                free(cache_data);
                LOG(INFO) << "Saved pipeline cache data to file successfully";

                return VK_SUCCESS;
            }
            catch (std::ofstream::failure&) {
                LOG(WARNING) << "Saving of pipeline cache to file failed with unindentified exception in std::ofstream.";
                return VK_ERROR_VALIDATION_FAILED_EXT;
            }
        }
        else {
            LOG(WARNING) << "Cache data was reported empty by Vulkan: errors possible.";
            return VK_SUCCESS;
        }
        
    }

}
