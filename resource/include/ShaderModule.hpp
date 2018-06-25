#pragma once
#ifndef VULPES_VK_SHADER_MODULE_H
#define VULPES_VK_SHADER_MODULE_H
#include "vpr_stdafx.h"
#include <memory>

namespace vpr {

    struct ShaderCodeFileLoader;

    /** A thoroughly thin wrapper around a VkShaderModule object, whose primary utility beyond RAII resource management is
    *   setting up the VkPipelineShaderStageCreateInfo required when creating/setting up an objects graphics pipeline.
    *   \ingroup Resources
    */
    class VPR_API ShaderModule {
        ShaderModule(const ShaderModule&) = delete;
        ShaderModule& operator=(const ShaderModule&) = delete;
    public:
    
        ShaderModule(const VkDevice& device, const char* filename, const VkShaderStageFlagBits& stages, const char* shader_name = nullptr);
        ShaderModule(const VkDevice& device, const VkShaderStageFlagBits& stages, const uint32_t* binary_source, const uint32_t& binary_source_length);
        ShaderModule(const VkDevice& device, const char* filename, VkPipelineShaderStageCreateInfo& create_info);
        ~ShaderModule();


        ShaderModule(ShaderModule&& other) noexcept;
        ShaderModule& operator=(ShaderModule&& other) noexcept;

        const VkShaderModule& vkHandle() const noexcept;

        const VkShaderStageFlagBits& StageBits() const noexcept;
        const VkShaderModuleCreateInfo& CreateInfo() const noexcept;
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
