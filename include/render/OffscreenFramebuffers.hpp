#pragma once
#ifndef VULPES_VK_OFFSCREEN_FRAMEBUFFERS_H
#define VULPES_VK_OFFSCREEN_FRAMEBUFFERS_H

#include "vpr_stdafx.h"
#include "Framebuffer.hpp"
#include "Swapchain.hpp"
#include "Renderpass.hpp"

namespace vulpes {
    
    using hdr_framebuffer_t = std::integral_constant<int, 0>;
	using bloom_framebuffer_t = std::integral_constant<int, 1>;
	using ssao_framebuffer_t = std::integral_constant<int, 2>;
	using g_buffer_t = std::integral_constant<int, 3>; // Dummy colorbuffer, Position, Normals, Tangents, Depth
	using picking_framebuffer_t = std::integral_constant<int, 4>; // Write triangle index into this Framebuffer 

    /** This is currently non-functional, but will eventually be an easier way of setting up the complicated subpasses, renderpasses,
    *   and subpass dependencies required for offscreen rendering and advanced rendering. The template types are:"
    *   - hdr_framebuffer_t: creates the attachments required for rendering an HDR pass
    *   - bloom_framebuffer_t: creates attachments required for rendering a bloom pass
    *   - ssao_framebuffer_t: creates attachments required for screen-space ambient occlusion 
    *   - g_buffer_t: creates attachments required for a baseline deferred rendering setup
    *   - picking_framebuffer_t: creates attachments required for doing mouse picking using exported colorbuffer data, thus giving increased accuracy.
    *   \ingroup Rendering
    *   \todo Finish implementing and testing these classes.
    */
	template<typename offscreen_framebuffer_type>
	class OffscreenFramebuffers {
	public:


		OffscreenFramebuffers(const Device* parent, const Swapchain* swapchain);

		void Create();
		
		bool Created = false;

        const VkRenderPass& GetRenderpass() const noexcept;
        const VkRenderPassBeginInfo& GetRenderpassBeginInfo() const noexcept;
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
        void setupRenderpassBeginInfo();
		virtual void createFramebuffers();
		void createSampler();

		std::vector<Image> attachments;
		std::vector<VkAttachmentDescription> attachmentDescriptions;
		std::vector<VkAttachmentReference> attachmentReferences;
		std::vector<VkSubpassDependency> subpassDependencies;

		std::unique_ptr<Renderpass> renderpass;
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