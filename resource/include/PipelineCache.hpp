#pragma once
#ifndef VULPES_VK_PIPELINE_CACHE_H
#define VULPES_VK_PIPELINE_CACHE_H
#include "vpr_stdafx.h"

namespace vpr {

    /**A PipelineCache is a wrapper around a VkPipelineCache that takes care of several important details that are otherwise
    *  difficult to handle: saving and loading pipeline cache data from a file, verifying integrity of pipeline cache files,
    *  and cleaning up / repairing old, unused, and outdated pipeline cache files. 
    *   
    *  This object can be used to increase the speed of creating graphics pipeline objects, and is especially useful for pipelines that are:
    *  - very complex and slow to create/recreate after a swapchain recreation
    *  - frequently created as part of other objects
    *  - for use with dynamic shader editing and recompiliation, which requires a pipeline recreation to propagate changes
    * 
    * The pipeline cache is especially helpful when used in MoltenVk - if one is using the runtime Metal shader compiler, then the cache is 
    * used to store the converted Metal shader code. By saving and reloading this cache data, then, one can avoid the significant cost of 
    * cross-compiling their shaders everytime they restart the program.
    * \ingroup Resources 
    */
    class VPR_API PipelineCache {
        PipelineCache(const PipelineCache& other) = delete;
        PipelineCache& operator=(const PipelineCache& other) = delete;
    public:

        /** Creates a pipeline cache, or loads a pre-existing one from file.
          * \param hash_id This can be nicely setting by using typeid(owning_type).hash_code(), so that types all share a pipline cache and there is still a unique identifier per type.  
          * \param host_phys_device This is required, as pipeline caches are related to the driver of the physical device they belong to. Thus, we need it for validation purposes.
        */
        PipelineCache(const VkDevice& parent, const VkPhysicalDevice& host_phys_device, const size_t hash_id);
        /** Creates a pipeline cache using the data from base_cache_handle
        */
        PipelineCache(const VkDevice& parent, const VkPhysicalDevice& host_phys_device, const VkPipelineCache& base_cache_handle, const size_t hash_id);
        ~PipelineCache();

        PipelineCache(PipelineCache&& other) noexcept;
        PipelineCache& operator=(PipelineCache&& other) noexcept;

        /**Takes a pipeline cache header and checks it for validity.*/
        bool Verify() const;
        VkResult DumpToDisk() const;
        /**Loads and overwrites potential contents with data loaded from the given file.*/
        void LoadCacheFromFile(const char * filename);
        const VkPipelineCache& vkHandle() const;

        /**Merges given cache objects into this object. Useful to generate pipelines across several threads,
         * but merge their cache data back into a single object for later re-use. 
         * 
         * Another potential use can be coalescing all created caches back into a single one at the end of a 
         * program, to simplify the re-loading process later.
         */
        void MergeCaches(const uint32_t num_caches, const VkPipelineCache* caches);

        static void SetCacheDirectory(const char* fname);
        static const char* GetCacheDirectory();

    private:

        void setFilename();
        void copyCacheData(const VkPipelineCache& other_cache);

        char* loadedData = nullptr;
        char* filename = nullptr;
        VkResult saveToFile() const;
        size_t hashID{ 0 };
        VkDevice parent{ VK_NULL_HANDLE };
        VkPhysicalDevice hostPhysicalDevice{ VK_NULL_HANDLE };
        VkPipelineCache handle{ VK_NULL_HANDLE };
        VkPipelineCacheCreateInfo createInfo{ };
    };
    
    /**Pass an easyloggingpp logging repository pointer into this function, and it will be set as
     * the repository for this module to use. That way, all log messages from all modules (even 
     * when using this as a shared library) will go to the same sinks
     * \ingroup Resources
    */
    VPR_API void SetLoggingRepository_VprResource(void* repo);
}

#endif // !VULPES_VK_PIPELINE_CACHE_H
