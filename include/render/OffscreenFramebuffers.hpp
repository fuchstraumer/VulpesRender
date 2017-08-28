#pragma once
#ifndef VULPES_VK_OFFSCREEN_FRAMEBUFFERS_H
#define VULPES_VK_OFFSCREEN_FRAMEBUFFERS_H

#include "vpr_stdafx.h"
#include "Framebuffer.hpp"
#include "Swapchain.hpp"

namespace vulpes {
    
    using hdr_framebuffer_t = std::integral_constant<int, 0>;
	using bloom_framebuffer_t = std::integral_constant<int, 1>;
	using ssao_framebuffer_t = std::integral_constant<int, 2>;
	using g_buffer_t = std::integral_constant<int, 3>; // Dummy colorbuffer, Position, Normals, Tangents, Depth
	using picking_framebuffer_t = std::integral_constant<int, 4>; // Write triangle index into this Framebuffer 

	template<typename offscreen_framebuffer_type>
	class OffscreenFramebuffers {
	public:


		OffscreenFramebuffers(const Device* parent, const Swapchain* swapchain);

		void Create();
		
		bool Created = false;

		const VkRenderPass& GetRenderpass() const noexcept;
		const VkFramebuffer& GetFramebuffer(const size_t& idx) const noexcept;
		const Image& GetAttachment(const size_t& idx) const noexcept;

	protected:
		
		// adds new attachment to back of attachments container.
		size_t createAttachment(const VkFormat& attachment_format, const VkImageUsageFlagBits & attachment_usage);
		void createAttachmentView(const size_t& attachment_idx);
		void createAttachments();
		void createAttachmentDescription(const size_t& attachment_idx, const VkImageLayout& final_attachment_layout, const VkAttachmentLoadOp& load_op = VK_ATTACHMENT_LOAD_OP_DONT_CARE, const VkAttachmentStoreOp& store_op = VK_ATTACHMENT_STORE_OP_DONT_CARE);
		void createAttachmentDescriptions();
		void createAttachmentReference(const size_t& attachment_idx, const VkImageLayout& final_attachment_layout);
		void createAttachmentReferences();
		void setupSubpassDescriptions();
		void setupSubpassDependencies();

		void createRenderpass();
		virtual void createFramebuffers();
		void createSampler();

		std::vector<Image> attachments;
		std::vector<VkAttachmentDescription> attachmentDescriptions;
		std::vector<VkAttachmentReference> attachmentReferences;
		std::vector<VkSubpassDependency> subpassDependencies;

		VkRenderPass renderpass;
		VkSampler sampler;
		VkExtent3D extents;
		std::vector<VkSubpassDescription> subpassDescriptions;
		std::vector<VkFramebuffer> framebuffers;
		VkFramebufferCreateInfo framebufferCreateInfo;
		const Device* device;
        const Swapchain* swapchain;
	};

}

#include "OffscreenFramebuffers.inl"

#endif //!VULPES_VK_OFFSCREEN_FRAMEBUFFERS_H