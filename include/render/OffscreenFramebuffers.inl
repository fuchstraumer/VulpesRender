#include "OffscreenFramebuffers.hpp"

namespace vulpes {

    template<typename offscreen_framebuffer_type>
	inline OffscreenFramebuffers<offscreen_framebuffer_type>::OffscreenFramebuffers(const Device * _device, const Swapchain* _swapchain) : framebufferCreateInfo(vk_framebuffer_create_info_base), device(_device), swapchain(_swapchain) {
        extents.width = swapchain->Extent.width;
        extents.height = swapchain->Extent.height;
        extents.depth = 1;
		framebufferCreateInfo.width = swapchain->Extent.width;
		framebufferCreateInfo.height = swapchain->Extent.height;
		framebufferCreateInfo.layers = 1;
		framebuffers.resize(swapchain->ImageCount);
	}

	template<typename offscreen_framebuffer_type>
	inline void OffscreenFramebuffers<offscreen_framebuffer_type>::Create() {
		
		createAttachments();
		createAttachmentDescriptions();
		createAttachmentReferences();
		setupSubpassDescriptions();
		setupSubpassDependencies();
		createRenderpass();
		createFramebuffers();
		createSampler();

		Created = true;

	}

	template<typename offscreen_framebuffer_type>
	inline const VkRenderPass & OffscreenFramebuffers<offscreen_framebuffer_type>::GetRenderpass() const noexcept {
		return renderpass->vkHandle();
    }
    
    template<typename offscreen_framebuffer_type>
    inline const VkRenderPassBeginInfo& OffscreenFramebuffers<offscreen_framebuffer_type>::GetRenderpassBeginInfo() const noexcept {
        return renderpass->BeginInfo();
    }

	template<typename offscreen_framebuffer_type>
	inline const VkFramebuffer & OffscreenFramebuffers<offscreen_framebuffer_type>::GetFramebuffer(const size_t & idx) const noexcept {
        renderpass->UpdateBeginInfo(framebuffers[idx]);
		return framebuffers[idx];
	}

	template<typename offscreen_framebuffer_type>
	inline const Image& OffscreenFramebuffers<offscreen_framebuffer_type>::GetAttachment(const size_t& idx) const noexcept {
		return attachments[idx];
	}

	template<typename offscreen_framebuffer_type>
	inline size_t OffscreenFramebuffers<offscreen_framebuffer_type>::createAttachment(const VkFormat & attachment_format, const VkImageUsageFlagBits & attachment_usage) {
		
		Image attachment(device);

		const VkImageCreateInfo attachment_create_info{
			VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
			nullptr,
			0,
			VK_IMAGE_TYPE_2D,
			attachment_format,
			extents,
			1, 
			1,
			VK_SAMPLE_COUNT_1_BIT,
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
	inline void OffscreenFramebuffers<offscreen_framebuffer_type>::createAttachmentView(const size_t& attachment_idx) {

		auto&& attachment = attachments[attachment_idx];

		VkImageAspectFlags aspect_flags = 0;
		auto image_usage = attachment.CreateInfo().usage;
		if (image_usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT) {
			aspect_flags = VK_IMAGE_ASPECT_COLOR_BIT;
		}
		else if (image_usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT) {
			aspect_flags = VK_IMAGE_ASPECT_DEPTH_BIT;
		}

		VkImageViewCreateInfo attachment_view_create_info = vk_image_view_create_info_base;
		attachment_view_create_info.image = attachment.vkHandle();
		attachment_view_create_info.format = attachment.Format();
		attachment_view_create_info.subresourceRange = VkImageSubresourceRange{ aspect_flags, 0, 1, 0, 1 };

		attachment.CreateView(attachment_view_create_info);

	}

	template<>
	inline void OffscreenFramebuffers<hdr_framebuffer_t>::createAttachments() {

		size_t idx = createAttachment(VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);
		createAttachmentView(idx);

		idx = createAttachment(VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);
		createAttachmentView(idx);

		idx = createAttachment(device->FindDepthFormat(), VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);
		createAttachmentView(idx);

	}

	template<>
	inline void OffscreenFramebuffers<ssao_framebuffer_t>::createAttachments() {

		size_t idx = createAttachment(VK_FORMAT_R8_UNORM, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);
		createAttachmentView(idx);

		idx = createAttachment(VK_FORMAT_R8_UNORM, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);
		createAttachmentView(idx);

	}

	template<>
	inline void OffscreenFramebuffers<g_buffer_t>::createAttachments() {

		// add a dummy attachment for colorbuffer (output from fragment shader). view will be taken from swapchain.
		attachments.push_back(vulpes::Image(device));

		// position 
		size_t idx = createAttachment(VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);
		createAttachmentView(idx);

		// normal
		idx = createAttachment(VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);
		createAttachmentView(idx);

		// color
		idx = createAttachment(VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);
		createAttachmentView(idx);

		// depth
		idx = createAttachment(device->FindDepthFormat(), VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);
		createAttachmentView(idx);

	}

	template<>
	inline void OffscreenFramebuffers<picking_framebuffer_t>::createAttachments() {

		size_t idx = createAttachment(VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);
		createAttachmentView(idx);

		idx = createAttachment(device->FindDepthFormat(), VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);
		createAttachmentView(idx);

        idx = createAttachment(VK_FORMAT_R16_UINT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);
        createAttachmentView(idx);

	}

	template<typename offscreen_framebuffer_type>
	inline void OffscreenFramebuffers<offscreen_framebuffer_type>::createAttachmentDescription(const size_t & attachment_idx, const VkImageLayout & final_attachment_layout,  const VkAttachmentLoadOp& load_op, const VkAttachmentStoreOp& store_op) {

		VkAttachmentDescription attachment_description = vk_attachment_description_base;

		attachment_description.samples = VK_SAMPLE_COUNT_1_BIT;

		attachment_description.loadOp = load_op;
		attachment_description.storeOp = store_op;
		attachment_description.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachment_description.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

		attachment_description.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		attachment_description.finalLayout = final_attachment_layout;
		attachment_description.format = attachments[attachment_idx].Format();
		
		attachmentDescriptions.push_back(std::move(attachment_description));

	}

	template<>
	inline void OffscreenFramebuffers<hdr_framebuffer_t>::createAttachmentDescriptions() {

		createAttachmentDescription(0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
		createAttachmentDescription(1, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
		createAttachmentDescription(2, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
		
	}

	template<>
	inline void OffscreenFramebuffers<ssao_framebuffer_t>::createAttachmentDescriptions() {

		createAttachmentDescription(0, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		createAttachmentDescription(1, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	}

	template<>
	inline void OffscreenFramebuffers<g_buffer_t>::createAttachmentDescriptions() {
		
		createAttachmentDescription(0, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE);
		createAttachmentDescription(1, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_ATTACHMENT_LOAD_OP_CLEAR);
		createAttachmentDescription(2, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_ATTACHMENT_LOAD_OP_CLEAR);
		createAttachmentDescription(3, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_ATTACHMENT_LOAD_OP_CLEAR);
		createAttachmentDescription(4, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_ATTACHMENT_LOAD_OP_CLEAR);

	}

    template<>
    inline void OffscreenFramebuffers<picking_framebuffer_t>::createAttachmentDescriptions() {
        createAttachmentDescription(0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_DONT_CARE);
        createAttachmentDescription(1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_DONT_CARE);
        createAttachmentDescription(2, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_STORE);
    }

	//template<>
	//inline void OffscreenFramebuffers<picking_framebuffer_t>::createAttachmentDescriptions() {

	//	createAttachmentDescription(0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_DONT_CARE);
	//	createAttachmentDescription(1, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_STORE);
	//	createAttachmentDescription(2, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_DONT_CARE);
	//	// just need to copy to buffer before end of renderpass (so in same series of rendering commands, using pipeline barriers)

	//}

	template<typename offscreen_framebuffer_type>
	inline void OffscreenFramebuffers<offscreen_framebuffer_type>::createAttachmentReference(const size_t & attachment_idx, const VkImageLayout & final_attachment_layout) {

		attachmentReferences.push_back(VkAttachmentReference{ attachment_idx, final_attachment_layout });

	}

	template<>
	inline void OffscreenFramebuffers<hdr_framebuffer_t>::createAttachmentReferences() {

		attachmentReferences.push_back(VkAttachmentReference{ 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL });
		attachmentReferences.push_back(VkAttachmentReference{ 1, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL });
		attachmentReferences.push_back(VkAttachmentReference{ 2, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL });

	}

	template<>
	inline void OffscreenFramebuffers<ssao_framebuffer_t>::createAttachmentReferences() {

		attachmentReferences.push_back(VkAttachmentReference{ 0, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL });
		attachmentReferences.push_back(VkAttachmentReference{ 1, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL });

	}

	template<> 
	inline void OffscreenFramebuffers<g_buffer_t>::createAttachmentReferences() {
		
		// G-buffer pass.
		attachmentReferences.push_back(VkAttachmentReference{ 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL });
		attachmentReferences.push_back(VkAttachmentReference{ 1, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL });
		attachmentReferences.push_back(VkAttachmentReference{ 2, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL });
		attachmentReferences.push_back(VkAttachmentReference{ 3, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL });
		attachmentReferences.push_back(VkAttachmentReference{ 4, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL });
		
		// Input (composition pass)
		attachmentReferences.push_back(VkAttachmentReference{ 0, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL });
		attachmentReferences.push_back(VkAttachmentReference{ 1, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL });
		attachmentReferences.push_back(VkAttachmentReference{ 2, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL });

		// Third subpass - forward transparency.
		attachmentReferences.push_back(VkAttachmentReference{ 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL });

	}

	template<>
	inline void OffscreenFramebuffers<picking_framebuffer_t>::createAttachmentReferences() {

		attachmentReferences.push_back(VkAttachmentReference{ 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL });
		attachmentReferences.push_back(VkAttachmentReference{ 2, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL });
		attachmentReferences.push_back(VkAttachmentReference{ 1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL });

	}

	template<>
	inline void OffscreenFramebuffers<hdr_framebuffer_t>::setupSubpassDescriptions() {

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
	inline void OffscreenFramebuffers<g_buffer_t>::setupSubpassDescriptions() {

		// First pass, write to Gbuffer.
		auto subpass_description_0 = VkSubpassDescription{
			0,
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			0,
			nullptr,
			4,
			&attachmentReferences[0],
			nullptr,
			&attachmentReferences[4],
			0,
			nullptr
		};

		subpassDescriptions.push_back(std::move(subpass_description_0));

		// Composition pass.
		static const uint32_t preserve_attachment_idx = 1;
		auto subpass_description_1 = VkSubpassDescription{
			0,
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			3,
			&attachmentReferences[5],
			1,
			&attachmentReferences[0],
			nullptr,
			&attachmentReferences[4],
			1,
			&preserve_attachment_idx
		};

		subpassDescriptions.push_back(std::move(subpass_description_1));

		// Transparency pass
		auto subpass_description_2 = VkSubpassDescription{
			0,
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			1,
			&attachmentReferences[1],
			1, 
			&attachmentReferences[0],
			nullptr,
			&attachmentReferences[4],
			0,
			nullptr,
		};

		subpassDescriptions.push_back(std::move(subpass_description_2));

	}

	template<>
	inline void OffscreenFramebuffers<picking_framebuffer_t>::setupSubpassDescriptions() {

		auto subpass_description = VkSubpassDescription {
			0,
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			0,
			nullptr,
			2,
			&attachmentReferences[0],
			nullptr,
			&attachmentReferences[2],
			0,
			nullptr
		};

		subpassDescriptions.push_back(subpass_description);

	}

	template<>
	inline void OffscreenFramebuffers<hdr_framebuffer_t>::setupSubpassDependencies() {

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

	template<>
	inline void OffscreenFramebuffers<g_buffer_t>::setupSubpassDependencies() {

		VkSubpassDependency g_buffer_dependency{
			VK_SUBPASS_EXTERNAL,
			0,
			VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			VK_ACCESS_MEMORY_READ_BIT,
			VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
			VK_DEPENDENCY_BY_REGION_BIT,
		};

		VkSubpassDependency transition_dependency {
			0,
			1,
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
			VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
			VK_ACCESS_SHADER_READ_BIT,
			VK_DEPENDENCY_BY_REGION_BIT,
		};

		VkSubpassDependency transparency_dependency{
			1,
			2,
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
			VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
			VK_ACCESS_SHADER_READ_BIT,
			VK_DEPENDENCY_BY_REGION_BIT,
		};

		VkSubpassDependency composition_dependency{
			0,
			VK_SUBPASS_EXTERNAL,
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
			VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
			VK_ACCESS_MEMORY_READ_BIT,
			VK_DEPENDENCY_BY_REGION_BIT,
		};

		subpassDependencies.push_back(g_buffer_dependency);
		subpassDependencies.push_back(transition_dependency);
		subpassDependencies.push_back(transparency_dependency);
		subpassDependencies.push_back(composition_dependency);

	}

	template<>
	inline void OffscreenFramebuffers<picking_framebuffer_t>::setupSubpassDependencies() {

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
	inline void OffscreenFramebuffers<offscreen_framebuffer_type>::createRenderpass() {

		VkRenderPassCreateInfo renderpass_info = vk_render_pass_create_info_base;
		renderpass_info.attachmentCount = static_cast<uint32_t>(attachmentDescriptions.size());
		renderpass_info.pAttachments = attachmentDescriptions.data();
		renderpass_info.subpassCount = static_cast<uint32_t>(subpassDescriptions.size());
		renderpass_info.pSubpasses = subpassDescriptions.data();
		renderpass_info.dependencyCount = static_cast<uint32_t>(subpassDependencies.size());
		renderpass_info.pDependencies = subpassDependencies.data();

        renderpass = std::make_unique<Renderpass>(device, renderpass_info);

    }

    template<>
    inline void OffscreenFramebuffers<picking_framebuffer_t>::setupRenderpassBeginInfo() {
        const std::vector<VkClearValue> clear_values {
            VkClearValue{ 0.0f, 0.0f, 0.0f, 0.0f },
            VkClearValue{ 0.0f, 0.0f, 0.0f, 0.0f},
            VkClearValue{ 1.0f, 0 }
        };
        renderpass->SetupBeginInfo(clear_values, swapchain->Extent);
    }

	template<typename offscreen_framebuffer_type>
	inline void OffscreenFramebuffers<offscreen_framebuffer_type>::createFramebuffers() {

		std::vector<VkImageView> attachment_views;

		for (const auto& attachment : attachments) {
			attachment_views.push_back(attachment.View());
		}

        framebufferCreateInfo.attachmentCount = static_cast<uint32_t>(attachment_views.size());
        framebufferCreateInfo.pAttachments = attachment_views.data();
		framebufferCreateInfo.renderPass = renderpass->vkHandle();

		for(size_t i = 0; i < framebuffers.size(); ++i) {
            VkResult result = vkCreateFramebuffer(device->vkHandle(), &framebufferCreateInfo, nullptr, &framebuffers[i]);
            VkAssert(result);
        }

	}

    template<>
    inline void OffscreenFramebuffers<g_buffer_t>::createFramebuffers() {
        
		std::vector<VkImageView> attachment_views(5);
		attachment_views[1] = attachments[1].View();
		attachment_views[2] = attachments[2].View();
		attachment_views[3] = attachments[3].View();
		attachment_views[4] = attachments[4].View();

        //framebufferCreateInfo.extent = extents;
        framebufferCreateInfo.renderPass = renderpass->vkHandle();

		for (size_t i = 0; i < framebuffers.size(); ++i) {
			attachment_views[0] = swapchain->ImageViews[i];
			framebufferCreateInfo.attachmentCount = static_cast<uint32_t>(attachment_views.size());
			framebufferCreateInfo.pAttachments = attachment_views.data();
			VkResult result = vkCreateFramebuffer(device->vkHandle(), &framebufferCreateInfo, nullptr, &framebuffers[i]);
			VkAssert(result);
		}

    }

	template<typename offscreen_framebuffer_type>
	inline void OffscreenFramebuffers<offscreen_framebuffer_type>::createSampler() {

		VkSamplerCreateInfo sampler_info = vk_sampler_create_info_base;
		sampler_info.magFilter = VK_FILTER_NEAREST;
		sampler_info.minFilter = VK_FILTER_NEAREST;

		VkResult result = vkCreateSampler(device->vkHandle(), &sampler_info, nullptr, &sampler);
		VkAssert(result);

	}

}