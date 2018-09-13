#include "vpr_stdafx.h"
#include "ShaderModule.hpp"
#include "vkAssert.hpp"
#include "CreateInfoBase.hpp"
#include "easylogging++.h"

namespace vpr {

    struct ShaderCodeFileLoader {
        void LoadCodeFromFile(const char* filename, std::vector<uint32_t>& dest, VkShaderModuleCreateInfo& create_info) {
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
                create_info.codeSize = code_size;
                dest.resize(input_buff.size() / sizeof(uint32_t) + 1);
                memcpy(dest.data(), input_buff.data(), input_buff.size());
                create_info.pCode = dest.data();

            }
            catch (std::ifstream::failure&) {
                LOG(ERROR) << "OBJECTS::RESOURCE::SHADER_MODULE: Failure opening or reading shader file: " << std::string(filename);
                throw(std::runtime_error("OBJECTS::RESOURCE::SHADER_MODULE: Failure opening or reading shader file."));
            }
        }
    };

    ShaderModule::ShaderModule(const VkDevice& device, const char * filename, const VkShaderStageFlagBits & _stages, const char * shader_name) : pipelineInfo(vk_pipeline_shader_stage_create_info_base), stages(_stages),
        createInfo(vk_shader_module_create_info_base), parent(device), handle(VK_NULL_HANDLE), fileLoader(std::make_unique<ShaderCodeFileLoader>()) {

        pipelineInfo.stage = stages;
        pipelineInfo.pName = "main";

        std::vector<uint32_t> binary_src;
        fileLoader->LoadCodeFromFile(filename, binary_src, createInfo);

        VkResult result = vkCreateShaderModule(parent, &createInfo, allocators, &handle);
        VkAssert(result);

        pipelineInfo.module = handle;
        pipelineInfo.pSpecializationInfo = nullptr;
    }

    ShaderModule::ShaderModule(const VkDevice& device, const VkShaderStageFlagBits& stages, const uint32_t* binary_source, const uint32_t& len) :
        parent(device), handle(VK_NULL_HANDLE), fileLoader(nullptr) {

        createInfo = vk_shader_module_create_info_base;
        createInfo.codeSize = len;
        createInfo.pCode = binary_source;

        VkResult result = vkCreateShaderModule(parent, &createInfo, allocators, &handle);
        VkAssert(result);

        pipelineInfo = vpr::vk_pipeline_shader_stage_create_info_base;
        pipelineInfo.module = handle;
        pipelineInfo.pName = "main";
        pipelineInfo.stage = stages;

    }

    ShaderModule::ShaderModule(const VkDevice& device, const char * filename, VkPipelineShaderStageCreateInfo & create_info) : 
        pipelineInfo(create_info), createInfo(vk_shader_module_create_info_base), parent(device), handle(VK_NULL_HANDLE),
        fileLoader(std::make_unique<ShaderCodeFileLoader>()) {
        
        std::vector<uint32_t> binary_src;
        fileLoader->LoadCodeFromFile(filename, binary_src, createInfo);
        VkResult result = vkCreateShaderModule(parent, &createInfo, allocators, &handle);
        VkAssert(result);
    }
    

    ShaderModule::~ShaderModule(){
        if (handle != VK_NULL_HANDLE) {
            vkDestroyShaderModule(parent, handle, allocators);
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
