#pragma once
#ifndef VULPES_VK_RENDER_PASS_H
#define VULPES_VK_RENDER_PASS_H

#include "vpr_stdafx.h"
#include "../ForwardDecl.hpp"

namespace vulpes {

    /** RAII wrapper around a Vulkan object, with a few utility methods to avoid redundancy and clutter in command-recording methods
    *   for various scenes. 
    *   \ingroup Rendering
    */
	class Renderpass {
		Renderpass(const Renderpass&) = delete;
		Renderpass& operator=(const Renderpass&) = delete;
	public:

		Renderpass(const Device* dvc, const VkRenderPassCreateInfo& create_info);
		Renderpass(Renderpass&& other) noexcept;
		Renderpass& operator=(Renderpass&& other) noexcept;
		~Renderpass();
        
        /** Must be setup before trying to use the VkRenderPassBeginInfo object attached to this class. Will cause validation layer and rendering errors otherwise. */
        void SetupBeginInfo(const std::vector<VkClearValue>& clear_values, const VkExtent2D& render_area);
        /** Call each frame or when changing a framebuffer to ensure the correct framebuffer is rendered to. */
        void UpdateBeginInfo(const VkFramebuffer& current_framebuffer);
		void Destroy();

		const VkRenderPass& vkHandle() const noexcept;

        const VkRenderPassCreateInfo& CreateInfo() const noexcept;
        /** This is the object you will need to retrieve inside renderpasses, when calling vkCmdBeginRenderpass. */
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
