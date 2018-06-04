#include "vpr_stdafx.h"
#include "resource/ShaderModule.hpp"
#include "core/LogicalDevice.hpp"
#include "easylogging++.h"

namespace vpr {

    ShaderModule::ShaderModule(const Device* device, const char * filename, const VkShaderStageFlagBits & _stages, const char * shader_name) : 
        ShaderModule(device, std::string(filename), _stages, shader_name) {}

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

        std::vector<uint32_t> binary_src;
        LoadCodeFromFile(filename.c_str(), binary_src);

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
        
        std::vector<uint32_t> binary_src;
        LoadCodeFromFile(filename, binary_src);
        VkResult result = vkCreateShaderModule(device->vkHandle(), &createInfo, allocators, &handle);
        VkAssert(result);
    }
    

    ShaderModule::~ShaderModule(){
        if (handle != VK_NULL_HANDLE) {
            vkDestroyShaderModule(parent->vkHandle(), handle, allocators);
        }
    }

    void ShaderModule::LoadCodeFromFile(const char * filename, std::vector<uint32_t>& dest_vector) {
        try {
            std::vector<char> input_buff;
            std::ifstream input(filename, std::ios::binary | std::ios::in | std::ios::ate);
            input.exceptions(std::ios::failbit | std::ios::badbit);
            const uint32_t code_size = static_cast<uint32_t>(input.tellg());
            if (code_size <= 1) {
                throw std::runtime_error("File opened for loading shader code from file is invalid!");
            }
            input_buff.resize(code_size);
            input.seekg(0, std::ios::beg);
            input.read(input_buff.data(), code_size);
            input.close();
            createInfo.codeSize = code_size;
            dest_vector.resize(input_buff.size() / sizeof(uint32_t) + 1);
            memcpy(dest_vector.data(), input_buff.data(), input_buff.size());
            createInfo.pCode = dest_vector.data();

        }
        catch (std::ifstream::failure&) {
            LOG(ERROR) << "OBJECTS::RESOURCE::SHADER_MODULE: Failure opening or reading shader file: " << std::string(filename);
            throw(std::runtime_error("OBJECTS::RESOURCE::SHADER_MODULE: Failure opening or reading shader file."));
        }
    }

    ShaderModule::ShaderModule(ShaderModule && other) noexcept : handle(std::move(other.handle)), stages(std::move(other.stages)), 
        createInfo(std::move(other.createInfo)), pipelineInfo(std::move(other.pipelineInfo)), parent(std::move(other.parent))
        { other.handle = VK_NULL_HANDLE; }

    ShaderModule & ShaderModule::operator=(ShaderModule && other) noexcept {
        handle = std::move(other.handle);
        stages = std::move(other.stages);
        createInfo = std::move(other.createInfo);
        pipelineInfo = std::move(other.pipelineInfo);
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
