#include "stdafx.h"
#include "DepthStencil.h"
#include "engine/renderer\core\LogicalDevice.h"

namespace vulpes {

	DepthStencil::DepthStencil(const Device * _parent, const VkExtent3D& extents, CommandPool* cmd, VkQueue & queue) : Image(_parent) {
		format = parent->FindDepthFormat();
		Create(extents, format, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_IMAGE_LAYOUT_UNDEFINED);
		TransitionLayout(VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, cmd, queue);
		CreateView(VK_IMAGE_ASPECT_DEPTH_BIT);
	}

	VkAttachmentDescription DepthStencil::DepthAttachment() const noexcept {
		return VkAttachmentDescription{
			0,
			format,
			VK_SAMPLE_COUNT_1_BIT,
			VK_ATTACHMENT_LOAD_OP_CLEAR,
			VK_ATTACHMENT_STORE_OP_DONT_CARE,
			VK_ATTACHMENT_LOAD_OP_DONT_CARE,
			VK_ATTACHMENT_STORE_OP_DONT_CARE,
			VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
		};
	}


}
