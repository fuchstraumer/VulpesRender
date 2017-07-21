#pragma once
#ifndef VULPES_VK_DEPTH_STENCIL_H
#define VULPES_VK_DEPTH_STENCIL_H
#include "stdafx.h"
#include "engine/renderer\resource\Image.h"

namespace vulpes {

	class DepthStencil : public Image {
	public:

		DepthStencil(const Device* _parent, const VkExtent3D& extents, CommandPool* cmd, VkQueue & queue);

		~DepthStencil() = default;

		VkAttachmentDescription DepthAttachment() const noexcept;
	};

}
#endif // !VULPES_VK_DEPTH_STENCIL_H
