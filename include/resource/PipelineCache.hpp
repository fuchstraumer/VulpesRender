#pragma once
#ifndef VULPES_VK_PIPELINE_CACHE_H
#define VULPES_VK_PIPELINE_CACHE_H
#include "vpr_stdafx.h"
#include "ForwardDecl.hpp"

namespace vpr {

    /** A PipelineCache is a wrapper around a VkPipelineCache that takes care of several important details that are otherwise
    *   difficult to handle: saving and loading pipeline cache data from a file, verifying integrity of pipeline cache files,
    *   and cleaning up / repairing old, unused, and outdated pipeline cache files. 
    *   
    *   This object can be used to increase the speed of creating graphics pipeline objects, and is especially useful for pipelines that are:
    *   - very complex and slow to create/recreate after a swapchain recreation
    *   - frequently created as part of other objects
    *   - for use with dynamic shader editing and recompiliation, which requires a pipeline recreation to propagate changes
    *   \ingroup Resources 
    */
    class VPR_API PipelineCache {
        PipelineCache(const PipelineCache& other) = delete;
        PipelineCache& operator=(const PipelineCache& other) = delete;
    public:

        /** Creates a pipeline cache, or loads a pre-existing one from file.
        *   \param hash_id: This can be nicely setting by using typeid(owning_type).hash_code(), so that types all share a pipline cache and there is still a unique identifier per type.  */
        PipelineCache(const Device* parent, const uint16_t& hash_id);

        ~PipelineCache();

        /** Takes a pipeline cache header and checks it for validity.
        *
        */
        bool Verify(const std::vector<int8_t>& cache_header) const;

        void LoadCacheFromFile(const char * filename);

        const VkPipelineCache& vkHandle() const;

    private:

        std::string filename;
        VkResult saveToFile() const;
        uint16_t hashID;
        const Device* parent;
        VkPipelineCache handle;
        VkPipelineCacheCreateInfo createInfo;

    };

}

#endif // !VULPES_VK_PIPELINE_CACHE_H
