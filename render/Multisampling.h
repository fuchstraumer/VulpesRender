#pragma once
#ifndef VULPES_VK_MULTISAMPLING_H
#define VULPES_VK_MULTISAMPLING_H

#include "vpr_stdafx.h"
#include "ForwardDecl.h"
#include "NonCopyable.h"
#include "resource/Image.h"
namespace vulpes {

	class Multisampling {
		Multisampling(const Multisampling&) = delete;
		Multisampling& operator=(const Multisampling&) = delete;
	public:

		Multisampling(const Device* dvc, const Swapchain* swapchain, const VkSampleCountFlagBits& sample_count, const uint32_t& width, const uint32_t& height);


		// Objects we sample from
		std::unique_ptr<Image> ColorBufferMS, DepthBufferMS;
		VkSampleCountFlagBits sampleCount;
		static VkSampleCountFlagBits SampleCount;
		const Device* device;
	};

}

#endif // !VULPES_VK_Multisampling_H
