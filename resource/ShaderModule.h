#pragma once
#ifndef VULPES_VK_SHADER_MODULE_H
#define VULPES_VK_SHADER_MODULE_H
#include "stdafx.h"
#include "engine/renderer\ForwardDecl.h"
#include "engine/renderer\NonCopyable.h"

namespace vulpes {

	class ShaderModule : public NonCopyable {
	public:
	
		ShaderModule(const Device* device, const char* filename, const VkShaderStageFlagBits& stages, const char* shader_name = nullptr);
		ShaderModule(const Device* device, const char* filename, VkPipelineShaderStageCreateInfo& create_info);
		~ShaderModule();

		void LoadCodeFromFile(const char* filename);

		ShaderModule(ShaderModule&& other) noexcept;
		ShaderModule& operator=(ShaderModule&& other) noexcept;

		const VkShaderModule& vkHandle() const noexcept;

		const VkShaderStageFlagBits& StageBits() const noexcept;
		const VkShaderModuleCreateInfo& CreateInfo() const noexcept;
		const VkPipelineShaderStageCreateInfo& PipelineInfo() const noexcept;

	private:
		const Device* parent;
		VkShaderStageFlagBits stages;
		VkPipelineShaderStageCreateInfo pipelineInfo;
		VkShaderModuleCreateInfo createInfo;
		VkShaderModule handle;
		uint32_t codeSize;
		std::vector<uint32_t> code;
		const VkAllocationCallbacks* allocators = nullptr;
	};

}
#endif // !VULPES_VK_SHADER_MODULE_H
