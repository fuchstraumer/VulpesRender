#include "vpr_stdafx.h"
#include "PipelineCache.hpp"
#include "easylogging++.h"
#if !defined(VPR_BUILD_STATIC)
INITIALIZE_EASYLOGGINGPP
#endif
#include "vkAssert.hpp"
#ifdef __APPLE_CC__
#include <boost/filesystem.hpp>
#else
#include <experimental/filesystem>
#endif

namespace vpr
{

    namespace fs = std::experimental::filesystem;
    static const std::string cacheSubdirectoryString{ "/shader_cache/" };
    static fs::path cachePath = fs::path(fs::temp_directory_path() / cacheSubdirectoryString);
    static std::string cacheString{ cachePath.string() };

    void SetLoggingRepository_VprResource(void* repo)
    {
        el::Helpers::setStorage(*(el::base::type::StoragePointer*)repo);
        LOG(INFO) << "Updating easyloggingpp storage pointer in vpr_resource module...";
    }
    
    constexpr static VkPipelineCacheCreateInfo base_create_info{ VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO, nullptr, 0, 0, nullptr };

    PipelineCache::PipelineCache(const VkDevice& _parent, const VkPhysicalDevice& host_device, const size_t hash_id) : parent(_parent), createInfo(base_create_info), 
        hostPhysicalDevice(host_device), hashID(hash_id), handle(VK_NULL_HANDLE)
    {

        setFilename();
        LoadCacheFromFile(filename);

        VkResult result = vkCreatePipelineCache(parent, &createInfo, nullptr, &handle);
        VkAssert(result);

    }

    PipelineCache::PipelineCache(const VkDevice& _parent, const VkPhysicalDevice& host_device, const VkPipelineCache& parent_cache, const size_t hash_id) : parent(_parent), createInfo(base_create_info),
        hostPhysicalDevice(host_device), hashID(hash_id), handle(VK_NULL_HANDLE)
    {
        setFilename();
        copyCacheData(parent_cache);

        VkResult result = vkCreatePipelineCache(parent, &createInfo, nullptr, &handle);
        VkAssert(result);
    }

    PipelineCache::~PipelineCache() {
        if (handle != VK_NULL_HANDLE)
        {
            VkResult saved = saveToFile();
            VkAssert(saved);
            vkDestroyPipelineCache(parent, handle, nullptr);
        }

        if (filename)
        {
            free(filename);
        }

        if (loadedData)
        {
            free(loadedData);
        }
    }

    PipelineCache::PipelineCache(PipelineCache&& other) noexcept : parent(std::move(other.parent)), createInfo(std::move(other.createInfo)),
        hashID(std::move(other.hashID)), handle(std::move(other.handle)), filename(std::move(other.filename)), loadedData(std::move(other.loadedData))
    {
        other.handle = VK_NULL_HANDLE; 
        other.filename = nullptr;
        other.loadedData = nullptr;
    }

    PipelineCache& PipelineCache::operator=(PipelineCache&& other) noexcept
    {
        parent = std::move(other.parent);
        createInfo = std::move(other.createInfo);
        hashID = std::move(other.hashID);
        handle = std::move(other.handle);
        other.handle = VK_NULL_HANDLE;
        filename = std::move(other.filename);
        other.filename = nullptr;
        loadedData = std::move(other.loadedData);
        other.loadedData = nullptr;
        return *this;
    }
 
    bool PipelineCache::Verify() const
    {

        uint32_t headerLength{ 0u };
        uint32_t cacheHeaderVersion{ 0u };
        uint32_t vendorID{ 0u };
        uint32_t deviceID{ 0u };
        uint8_t cacheUUID[VK_UUID_SIZE];

#ifdef _MSC_VER
        memcpy_s(&headerLength, sizeof(uint32_t), loadedData + 0u, sizeof(uint32_t));
        memcpy_s(&cacheHeaderVersion, sizeof(uint32_t), loadedData + sizeof(uint32_t), sizeof(uint32_t));
        memcpy_s(&vendorID, sizeof(uint32_t), loadedData + sizeof(uint32_t) * 2u, sizeof(uint32_t));
        memcpy_s(&deviceID, sizeof(uint32_t), loadedData + sizeof(uint32_t) * 3u, sizeof(uint32_t));
        memcpy_s(cacheUUID, sizeof(uint8_t) * VK_UUID_SIZE, loadedData + 16u, VK_UUID_SIZE);
#else // not all unix compilers have this, and MSVC is the only one that complains if we don't use it anyways
        memcpy(&headerLength, sizeof(uint32_t), loadedData + 0u, sizeof(uint32_t));
        memcpy(&cacheHeaderVersion, sizeof(uint32_t), loadedData + sizeof(uint32_t), sizeof(uint32_t));
        memcpy(&vendorID, sizeof(uint32_t), loadedData + sizeof(uint32_t) * 2u, sizeof(uint32_t));
        memcpy(&deviceID, sizeof(uint32_t), loadedData + sizeof(uint32_t) * 3u, sizeof(uint32_t));
        memcpy(cacheUUID, sizeof(uint8_t) * VK_UUID_SIZE, loadedData + 16u, VK_UUID_SIZE);
#endif

        if (headerLength != 32)
        {
            return false;
        }

        if (cacheHeaderVersion != static_cast<uint32_t>(VK_PIPELINE_CACHE_HEADER_VERSION_ONE))
        {
            return false;
        }

        VkPhysicalDeviceProperties properties;
        vkGetPhysicalDeviceProperties(hostPhysicalDevice, &properties);

        if (vendorID != properties.vendorID)
        {
            return false;
        }

        if (deviceID != properties.deviceID)
        {
            return false;
        }

        if (memcmp(cacheUUID, properties.pipelineCacheUUID, sizeof(cacheUUID)) != 0)
        {
            LOG(WARNING) << "Pipeline cache UUID incorrect, requires rebuilding.";
            return false;
        }

        return true;
    }

    VkResult PipelineCache::DumpToDisk() const
    {
        return saveToFile();
    }

    void PipelineCache::setFilename()
    {
#ifdef __APPLE_CC__
        namespace fs = boost::filesystem;
#else
        namespace fs = std::experimental::filesystem;
#endif

        if (!fs::exists(cachePath))
        {
            LOG(INFO) << "Shader cache path didn't exist, creating...";
            fs::create_directories(cachePath);
        }

        std::string fname = cacheString + std::to_string(hashID) + std::string(".vkdat");
#ifdef _MSC_VER
        filename = _strdup(fname.c_str());
#else
        filename = strdup(fname.c_str());
#endif
    }

    void PipelineCache::copyCacheData(const VkPipelineCache& other_cache)
    {
        
        size_t cache_size{ 0u };
        VkResult result = vkGetPipelineCacheData(parent, other_cache, &cache_size, nullptr);
        VkAssert(result);

        if (cache_size <= 36u) // size will include header for default-setup cache
        {
            LoadCacheFromFile(filename);
            return;
        }

        loadedData = (char*)malloc(sizeof(char) * cache_size);
        result = vkGetPipelineCacheData(parent, other_cache, &cache_size, loadedData);
        VkAssert(result);

        createInfo.initialDataSize = cache_size;
        createInfo.pInitialData = loadedData;

    }

    void PipelineCache::LoadCacheFromFile(const char* _filename)
    {
        /*
        check for pre-existing cache file.
        */
        std::ifstream cache(_filename, std::ios::ate | std::ios::binary);
        size_t file_size = static_cast<size_t>(cache.tellg());
        cache.seekg(0, std::ios::beg);

        if (cache && (file_size > 0u))
        {

            loadedData = (char*)malloc(sizeof(char) * file_size);
            if (!cache.read(loadedData, file_size))
            {
                LOG(ERROR) << "Failed to read file!";
                throw std::runtime_error("Failed to read file!");
            }

            // Check to see if header data matches current device.
            if (Verify())
            {
                createInfo.initialDataSize = file_size;
                createInfo.pInitialData = loadedData;
            }
            else
            {
                LOG_IF(VERBOSE_LOGGING, INFO) << "Pre-existing cache file isn't valid: creating new pipeline cache.";
                createInfo.initialDataSize = 0;
                createInfo.pInitialData = nullptr;
                cache.close();
                if (!std::experimental::filesystem::remove(_filename))
                {
                    LOG(WARNING) << "Unable to erase pre-existing cache data. Won't be able to write new contents to disk!";
                }
            }
        }
        else
        {
            LOG_IF(VERBOSE_LOGGING, INFO) << "No pre-existing cache found.";
            createInfo.initialDataSize = 0;
            createInfo.pInitialData = nullptr;
        }
    }

    const VkPipelineCache& PipelineCache::vkHandle() const
    {
        return handle;
    }

    void PipelineCache::MergeCaches(const uint32_t num_caches, const VkPipelineCache* caches)
    {
        VkResult result = vkMergePipelineCaches(parent, handle, num_caches, caches);
        if (result != VK_SUCCESS)
        {
            LOG_IF(VERBOSE_LOGGING, WARNING) << "Failed to merge pipeline caches: can suggest that the operation simply wasn't useful.";
        }
    }

    void PipelineCache::SetCacheDirectory(const char* fname)
    {
        if (auto new_path = fs::path(fname); fs::exists(new_path))
        {
            cachePath = new_path;
            cacheString = new_path.string();
        }
    }

    const char* PipelineCache::GetCacheDirectory()
    {
        return cacheString.c_str();
    }
    
    VkResult PipelineCache::saveToFile() const
    {

        VkResult result = VK_SUCCESS;
        size_t cache_size;

        if (!parent)
        {
            LOG(ERROR) << "Attempted to delete/save a non-existent cache!";
            return VK_ERROR_DEVICE_LOST;
        }

        // works like enumerate calls: get size first, then use size to get data.
        result = vkGetPipelineCacheData(parent, handle, &cache_size, nullptr);
        VkAssert(result);

        if (cache_size != 0)
        {
            try
            {
                std::ofstream file(filename, std::ios::out | std::ios::trunc | std::ios::binary);

                void* endCacheData = (char*)malloc(sizeof(char) * cache_size);

                result = vkGetPipelineCacheData(parent, handle, &cache_size, endCacheData);
                VkAssert(result);

                file.write(reinterpret_cast<const char*>(endCacheData), cache_size);
                file.close();

                free(endCacheData);
                LOG(INFO) << "Saved pipeline cache data to file successfully";

                return VK_SUCCESS;
            }
            catch (std::ofstream::failure&)
            {
                LOG(WARNING) << "Saving of pipeline cache to file failed with unindentified exception in std::ofstream.";
                return VK_ERROR_VALIDATION_FAILED_EXT;
            }
        }
        else
        {
            LOG(WARNING) << "Cache data was reported empty by Vulkan: errors possible.";
            return VK_SUCCESS;
        }
        
    }

}
