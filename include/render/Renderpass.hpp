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
		
        void SetupRenderPassBeginInfo(const std::vector<VkClearValue>& clear_values, const VkExtent2D& render_area);
        void UpdateRenderPassBeginInfo(const VkFramebuffer& current_framebuffer);
		void Destroy();

		const VkRenderPass& vkHandle() const noexcept;

		const VkRenderPassCreateInfo& CreateInfo() const noexcept;
        const VkRenderPassBeginInfo& BeginInfo() const noexcept;

	private:

		const Device* parent;
		VkRenderPass handle;
		VkRenderPassCreateInfo createInfo;
        VkRenderPassBeginInfo beginInfo;
        std::vector<VkClearValue> clearValues;
		const VkAllocationCallbacks* allocators = nullptr;

	};

}
#endif // !VULPES_VK_RENDER_PASS_H
