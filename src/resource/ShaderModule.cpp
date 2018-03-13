#include "vpr_stdafx.h"
#include "resource/ShaderModule.hpp"
#include "core/LogicalDevice.hpp"
#include "util/easylogging++.h"

namespace vpr {

    ShaderModule::ShaderModule(const Device* device, const char * filename, const VkShaderStageFlagBits & _stages, const char * shader_name) : 
        pipelineInfo(vk_pipeline_shader_stage_create_info_base), stages(_stages), createInfo(vk_shader_module_create_info_base), parent(device),
        handle(VK_NULL_HANDLE) {
        
        pipelineInfo.stage = stages;

        if (shader_name != nullptr) {
            pipelineInfo.pName = shader_name;
        }
        else {
            pipelineInfo.pName = "main";
        }

        LoadCodeFromFile(filename);

        VkResult result = vkCreateShaderModule(device->vkHandle(), &createInfo, allocators, &handle);
        VkAssert(result);

        pipelineInfo.module = handle;
        pipelineInfo.pSpecializationInfo = nullptr;

    }

    ShaderModule::ShaderModule(const Device * device, const std::string & filename, const VkShaderStageFlagBits & _stages, 
        const char * shader_entry_point) : pipelineInfo(vk_pipeline_shader_stage_create_info_base), stages(_stages), 
        createInfo(vk_shader_module_create_info_base), parent(device), handle(VK_NULL_HANDLE) {
        
        pipelineInfo.stage = stages;

        if (shader_entry_point != nullptr) {
            pipelineInfo.pName = shader_entry_point;
        }
        else {
            pipelineInfo.pName = "main";
        }

        LoadCodeFromFile(filename.c_str());

        VkResult result = vkCreateShaderModule(device->vkHandle(), &createInfo, allocators, &handle);
        VkAssert(result);

        pipelineInfo.module = handle;
        pipelineInfo.pSpecializationInfo = nullptr;
    }

    ShaderModule::ShaderModule(const Device* device, const VkShaderStageFlagBits& stages, const uint32_t* binary_source, const uint32_t& len) :
        parent(device), handle(VK_NULL_HANDLE) {

        createInfo = vk_shader_module_create_info_base;
        createInfo.codeSize = len;
        createInfo.pCode = binary_source;

        VkResult result = vkCreateShaderModule(device->vkHandle(), &createInfo, allocators, &handle);
        VkAssert(result);

        pipelineInfo.module = handle;
        pipelineInfo.pName = "main";
        pipelineInfo.stage = stages;

    }

    ShaderModule::ShaderModule(const Device * device, const char * filename, VkPipelineShaderStageCreateInfo & create_info) : 
        pipelineInfo(create_info), createInfo(vk_shader_module_create_info_base), parent(device), handle(VK_NULL_HANDLE) {

        LoadCodeFromFile(filename);
        VkResult result = vkCreateShaderModule(device->vkHandle(), &createInfo, allocators, &handle);
        VkAssert(result);
    }
    

    ShaderModule::~ShaderModule(){
        if (handle != VK_NULL_HANDLE) {
            vkDestroyShaderModule(parent->vkHandle(), handle, allocators);
        }
    }

    void ShaderModule::LoadCodeFromFile(const char * filename) {
        try {
            std::vector<char> input_buff;
            std::ifstream input(filename, std::ios::binary | std::ios::in | std::ios::ate);
            input.exceptions(std::ios::failbit | std::ios::badbit);
            codeSize = static_cast<uint32_t>(input.tellg());
            assert(codeSize > 0);
            input_buff.resize(codeSize);
            input.seekg(0, std::ios::beg);
            input.read(input_buff.data(), codeSize);
            input.close();
            createInfo.codeSize = codeSize;
            code.resize(input_buff.size() / sizeof(uint32_t) + 1);
            memcpy(code.data(), input_buff.data(), input_buff.size());
            createInfo.pCode = code.data();

        }
        catch (std::ifstream::failure&) {
            LOG(ERROR) << "OBJECTS::RESOURCE::SHADER_MODULE: Failure opening or reading shader file: " << std::string(filename);
            throw(std::runtime_error("OBJECTS::RESOURCE::SHADER_MODULE: Failure opening or reading shader file."));
        }
    }

    ShaderModule::ShaderModule(ShaderModule && other) noexcept : handle(std::move(other.handle)), stages(std::move(other.stages)), 
        createInfo(std::move(other.createInfo)), pipelineInfo(std::move(other.pipelineInfo)), code(std::move(other.code)), 
        codeSize(std::move(other.codeSize)), parent(std::move(other.parent)) { other.handle = VK_NULL_HANDLE; }

    ShaderModule & ShaderModule::operator=(ShaderModule && other) noexcept {
        handle = std::move(other.handle);
        stages = std::move(other.stages);
        createInfo = std::move(other.createInfo);
        pipelineInfo = std::move(other.pipelineInfo);
        code = std::move(other.code);
        codeSize = std::move(other.codeSize);
        parent = std::move(other.parent);
        other.handle = VK_NULL_HANDLE;
        return *this;
    }

    const VkShaderModule & ShaderModule::vkHandle() const noexcept{
        return handle;
    }

    const VkShaderStageFlagBits & ShaderModule::StageBits() const noexcept{
        return stages;
    }

    const VkShaderModuleCreateInfo & ShaderModule::CreateInfo() const noexcept{
        return createInfo;
    }

    const VkPipelineShaderStageCreateInfo & ShaderModule::PipelineInfo() const noexcept{
        return pipelineInfo;
    }

}
