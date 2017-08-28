#pragma once
#ifndef VULPES_VK_RENDER_PASS_H
#define VULPES_VK_RENDER_PASS_H

#include "vpr_stdafx.h"
#include "../ForwardDecl.hpp"

namespace vulpes {

	class Renderpass {
		Renderpass(const Renderpass&) = delete;
		Renderpass& operator=(const Renderpass&) = delete;
	public:

		Renderpass(const Device* dvc, const VkRenderPassCreateInfo& create_info);
		Renderpass(Renderpass&& other) noexcept;
		Renderpass& operator=(Renderpass&& other) noexcept;
		~Renderpass();
		
		void Destroy();

		const VkRenderPass& vkHandle() const noexcept;
		operator VkRenderPass() const noexcept;

		const VkRenderPassCreateInfo& CreateInfo() const noexcept;

	private:
		const Device* parent;
		VkRenderPass handle;
		VkRenderPassCreateInfo createInfo;
		const VkAllocationCallbacks* allocators = nullptr;
	};

}
#endif // !VULPES_VK_RENDER_PASS_H
