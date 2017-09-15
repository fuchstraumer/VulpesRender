#pragma once
#ifndef VULPES_VK_PIPELINE_LAYOUT_H
#define VULPES_VK_PIPELINE_LAYOUT_H

#include "vpr_stdafx.h"
#include "ForwardDecl.hpp"

namespace vulpes {

    class PipelineLayout {
		PipelineLayout(const PipelineLayout&) = delete;
		PipelineLayout& operator=(const PipelineLayout&) = delete;
	public:

		PipelineLayout(const Device* device);
		~PipelineLayout();
		
		PipelineLayout(PipelineLayout&& other) noexcept;
		PipelineLayout& operator=(PipelineLayout&& other) noexcept;

		void Destroy();

		void Create(const std::vector<VkPushConstantRange>& push_constants);
		void Create(const std::vector<VkDescriptorSetLayout>& set_layouts);
		void Create(const std::vector<VkDescriptorSetLayout>& set_layouts, const std::vector<VkPushConstantRange>& push_constants);

		const VkPipelineLayout& vkHandle() const noexcept;

	private:

		const Device* device;
		VkPipelineLayoutCreateInfo createInfo;
		VkPipelineLayout handle;
    };

}


#endif //!VULPES_VK_PIPELINE_LAYOUT_H