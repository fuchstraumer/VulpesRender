#pragma once
#ifndef VULPES_VK_FRAMEBUFFER_H
#define VULPES_VK_FRAMEBUFFER_H

#include "stdafx.h"
#include "engine/renderer\ForwardDecl.h"
#include "engine/renderer\NonCopyable.h"
#include "engine/renderer/resource/Image.h"
#include "engine/renderer/render/Multisampling.h"
#include "engine/renderer/core/LogicalDevice.h"

namespace vulpes {

	class Framebuffer : public NonCopyable {
	public:

		virtual ~Framebuffer();

		Framebuffer(const Device* parent, const VkFramebufferCreateInfo& create_info);
		Framebuffer& operator=(Framebuffer&& other) noexcept;
		Framebuffer(Framebuffer&& other) noexcept;
		const VkFramebuffer& vkHandle() const noexcept;
		
		void Destroy();

	protected:

		const Device* parent;
		const VkAllocationCallbacks* allocators = nullptr;
		VkFramebuffer handle;
	};

	using hdr_framebuffer_t = std::integral_constant<int, 0>;
	using bloom_framebuffer_t = std::integral_constant<int, 1>;

	template<typename offscreen_framebuffer_type>
	class OffscreenFramebuffer : public Framebuffer {
	public:

		OffscreenFramebuffer(const Device* parent, const VkFramebufferCreateInfo& create_info, const VkExtent3D& extents);

		void Create();

	protected:
		
		// adds new attachment to back of attachments container.
		size_t createAttachment(const VkFormat& attachment_format, const VkImageUsageFlagBits & attachment_usage);
		void createAttachmentView(const size_t& attachment_idx);
		void createAttachments();
		void createAttachmentDescription(const size_t& attachment_idx, const VkImageLayout& final_attachment_layout);
		void createAttachmentDescriptions();
		void createAttachmentReference(const size_t& attachment_idx, const VkImageLayout& final_attachment_layout);
		void createAttachmentReferences();
		void setupSubpassDescription();
		void setupSubpassDependencies();

		void createRenderpass();
		void createFramebuffer();
		void createSampler();

		std::vector<Image> attachments;
		std::vector<VkAttachmentDescription> attachmentDescriptions;
		std::vector<VkAttachmentReference> attachmentReferences;
		std::vector<VkSubpassDependency> subpassDependencies;

		VkRenderPass renderpass;
		
		VkSemaphore semaphore = VK_NULL_HANDLE;
		VkSampler sampler;
		VkExtent3D extents;
		std::vector<VkSubpassDescription> subpassDescriptions;
	};

	template<typename offscreen_framebuffer_type>
	inline void OffscreenFramebuffer<offscreen_framebuffer_type>::Create() {
		
		createAttachments();
		createAttachmentDescriptions();
		createAttachmentReferences();
		setupSubpassDescription();
		setupSubpassDependencies();
		createRenderpass();
		createFramebuffer();
		createSampler();

	}

	template<typename offscreen_framebuffer_type>
	inline size_t OffscreenFramebuffer<offscreen_framebuffer_type>::createAttachment(const VkFormat & attachment_format, const VkImageUsageFlagBits & attachment_usage) {
		
		Image attachment(parent);

		VkImageCreateInfo attachment_create_info{
			VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
			nullptr,
			0,
			VK_IMAGE_TYPE_2D,
			attachment_format,
			extents,
			1, 
			1,
			Multisampling::SampleCount,
			VK_IMAGE_TILING_OPTIMAL,
			VkImageUsageFlags(attachment_usage | VK_IMAGE_USAGE_SAMPLED_BIT),
			VK_SHARING_MODE_EXCLUSIVE,
			0,
			nullptr,
			VK_IMAGE_LAYOUT_UNDEFINED,
		};

		attachment.Create(attachment_create_info);

		attachments.push_back(std::move(attachment));

		return attachments.size() - 1;
	}

	template<typename offscreen_framebuffer_type>
	inline void OffscreenFramebuffer<offscreen_framebuffer_type>::createAttachmentView(const size_t& attachment_idx) {

		auto&& attachment = attachments[attachment_idx];

		VkImageAspectFlags aspect_flags = 0;
		auto image_usage = attachment.CreateInfo().usage;
		if (image_usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT) {
			aspect_flags = VK_IMAGE_ASPECT_COLOR_BIT;
		}
		else if (image_usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT) {
			aspect_flags = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
		}

		VkImageViewCreateInfo attachment_view_create_info = vk_image_view_create_info_base;
		attachment_view_create_info.image = attachment.vkHandle();
		attachment_view_create_info.format = attachment.Format();
		attachment_view_create_info.subresourceRange = VkImageSubresourceRange{ aspect_flags, 0, 1, 0, 1 };

		attachment.CreateView(attachment_view_create_info);

	}

	template<>
	inline void OffscreenFramebuffer<hdr_framebuffer_t>::createAttachments() {

		size_t idx = createAttachment(VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);
		createAttachmentView(idx);

		idx = createAttachment(VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);
		createAttachmentView(idx);

		idx = createAttachment(parent->FindDepthFormat(), VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);
		createAttachmentView(idx);

	}

	template<typename offscreen_framebuffer_type>
	inline void OffscreenFramebuffer<offscreen_framebuffer_type>::createAttachmentDescription(const size_t & attachment_idx, const VkImageLayout & final_attachment_layout) {

		VkAttachmentDescription attachment_description = vk_attachment_description_base;

		attachment_description.samples = Multisampling::SampleCount;

		attachment_description.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		attachment_description.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		attachment_description.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachment_description.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

		attachment_description.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		attachment_description.finalLayout = final_attachment_layout;
		attachment_description.format = attachments[attachment_idx].Format();
		
		attachmentDescriptions.push_back(std::move(attachment_description));

	}

	template<>
	inline void OffscreenFramebuffer<hdr_framebuffer_t>::createAttachmentDescriptions() {

		createAttachmentDescription(0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
		createAttachmentDescription(1, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
		createAttachmentDescription(2, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
		
	}

	template<typename offscreen_framebuffer_type>
	inline void OffscreenFramebuffer<offscreen_framebuffer_type>::createAttachmentReference(const size_t & attachment_idx, const VkImageLayout & final_attachment_layout) {

		attachmentReferences.push_back(VkAttachmentReference{ attachment_idx, final_attachment_layout });

	}

	template<>
	inline void OffscreenFramebuffer<hdr_framebuffer_t>::createAttachmentReferences() {

		attachmentReferences.push_back(VkAttachmentReference{ 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL });
		attachmentReferences.push_back(VkAttachmentReference{ 1, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL });
		attachmentReferences.push_back(VkAttachmentReference{ 2, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL });

	}

	template<>
	inline void OffscreenFramebuffer<hdr_framebuffer_t>::setupSubpassDescription() {

		auto subpass_description = VkSubpassDescription{
			0,
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			0,
			nullptr,
			2, 
			attachmentReferences.data(),
			nullptr,
			&attachmentReferences[2],
			0,
			nullptr
		};

		subpassDescriptions.push_back(std::move(subpass_description));

	}

	template<>
	inline void OffscreenFramebuffer<hdr_framebuffer_t>::setupSubpassDependencies() {

		VkSubpassDependency first_dependency{
			VK_SUBPASS_EXTERNAL,
			0,
			VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			VK_ACCESS_MEMORY_READ_BIT,
			VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
			VK_DEPENDENCY_BY_REGION_BIT,
		};

		VkSubpassDependency second_dependency{
			0,
			VK_SUBPASS_EXTERNAL,
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
			VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
			VK_ACCESS_MEMORY_READ_BIT,
			VK_DEPENDENCY_BY_REGION_BIT,
		};

		subpassDependencies.push_back(std::move(first_dependency));
		subpassDependencies.push_back(std::move(second_dependency));

	}

	template<typename offscreen_framebuffer_type>
	inline void OffscreenFramebuffer<offscreen_framebuffer_type>::createRenderpass() {

		VkRenderPassCreateInfo renderpass_info = vk_renderpass_create_info_base;
		renderpass_info.attachmentCount = static_cast<uint32_t>(attachmentDescriptions.size());
		renderpass_info.pAttachments = attachmentDescriptions.data();
		renderpass_info.subpassCount = static_cast<uint32_t>(subpassDescriptions.size());
		renderpass_info.pSubpasses = subpassDescriptions.data();
		renderpass_info.dependencyCount = static_cast<uint32_t>(subpassDependencies.size());
		renderpass_info.pDependencies = subpassDependencies.data();

		VkResult result = vkCreateRenderPass(parent->vkHandle(), &renderpass_info, nullptr, &renderpass);
		VkAssert(result);

	}

	template<typename offscreen_framebuffer_type>
	inline void OffscreenFramebuffer<offscreen_framebuffer_type>::createFramebuffer() {

		std::vector<VkImageView> attachment_views;

		for (const auto& attachment : attachments) {
			attachment_views.push_back(attachment.View());
		}

		VkFramebufferCreateInfo framebuffer_info = vk_framebuffer_create_info_base;
		framebuffer_info.renderPass = renderpass;
		framebuffer_info.attachmentCount = static_cast<uint32_t>(attachment_views.size());
		framebuffer_info.pAttachments = attachment_views.data();
		framebuffer_info.width = extents.width;
		framebuffer_info.height = extents.height;
		framebuffer_info.layers = 1;

		VkResult result = vkCreateFramebuffer(parent->vkHandle(), &framebuffer_info, nullptr, &handle);
		VkAssert(result);

	}

	template<typename offscreen_framebuffer_type>
	inline void OffscreenFramebuffer<offscreen_framebuffer_type>::createSampler() {

		VkSamplerCreateInfo sampler_info = vk_sampler_create_info_base;
		sampler_info.magFilter = VK_FILTER_NEAREST;
		sampler_info.minFilter = VK_FILTER_NEAREST;

		VkResult result = vkCreateSampler(parent->vkHandle(), &sampler_info, nullptr, &sampler);
		VkAssert(result);

	}


}
#endif // !VULPES_VK_FRAMEBUFFER_H
