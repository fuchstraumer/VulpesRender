#pragma once
#ifndef VULPES_VK_SHADER_MODULE_H
#define VULPES_VK_SHADER_MODULE_H
#include "vpr_stdafx.h"
#include <memory>

namespace vpr
{

    struct ShaderCodeFileLoader;

    /** A thoroughly thin wrapper around a VkShaderModule object, whose primary utility beyond RAII resource management is
    *   setting up the VkPipelineShaderStageCreateInfo required when creating/setting up an objects graphics pipeline.
    *   \ingroup Resources
    */
    class VPR_API ShaderModule
    {
        ShaderModule(const ShaderModule&) = delete;
        ShaderModule& operator=(const ShaderModule&) = delete;
    public:

        /**Creates a new shader for the given stage by reading from the specified file.
         * \param entry_point This optional parameter decides what point to invoke/execute the given shader from. 
         */
        ShaderModule(const VkDevice& device, const char* filename, const VkShaderStageFlagBits& stages, const char* entry_point = nullptr);
        /**Creates a new shader for the given stage by using the given binary data array. Will be slightly faster, as it doesn't have to open
         * a file and copy that data into the program.
         */
        ShaderModule(const VkDevice& device, const VkShaderStageFlags stages, const uint32_t* binary_source, const uint32_t binary_source_length);
        ShaderModule(const VkDevice& device, const char* filename, VkPipelineShaderStageCreateInfo& create_info);
        ~ShaderModule();

        ShaderModule(ShaderModule&& other) noexcept;
        ShaderModule& operator=(ShaderModule&& other) noexcept;

        const VkShaderModule& vkHandle() const noexcept;

        const VkShaderStageFlagBits& StageBits() const noexcept;
        const VkShaderModuleCreateInfo& CreateInfo() const noexcept;
        /**Retrieves the object required to bind/use this shader in a pipeline. Fields are already filled out: should not be modified.
         * Only potential modification point would be changing the entry point, after creating a fresh copy of this object - this is left
         * to advanced users though, so it won't be explained more here.
         */
        const VkPipelineShaderStageCreateInfo& PipelineInfo() const noexcept;

    private:
        std::unique_ptr<ShaderCodeFileLoader> fileLoader;
        VkDevice parent{ VK_NULL_HANDLE };
        VkShaderStageFlagBits stages{ VK_SHADER_STAGE_FLAG_BITS_MAX_ENUM };
        VkPipelineShaderStageCreateInfo pipelineInfo{};
        VkShaderModuleCreateInfo createInfo{};
        VkShaderModule handle{ VK_NULL_HANDLE };
        const VkAllocationCallbacks* allocators{ nullptr };
    };

}
#endif // !VULPES_VK_SHADER_MODULE_H
