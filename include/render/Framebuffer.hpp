#pragma once
#ifndef VULPES_VK_FRAMEBUFFER_H
#define VULPES_VK_FRAMEBUFFER_H

#include "vpr_stdafx.h"
#include "../ForwardDecl.hpp"
#include "../resource/Image.hpp"
#include "../render/Multisampling.hpp"
#include "../core/LogicalDevice.hpp"


namespace vulpes {

	class Framebuffer {
		Framebuffer(const Framebuffer&) = delete;
		Framebuffer& operator=(const Framebuffer&) = delete;
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


}
#endif // !VULPES_VK_FRAMEBUFFER_H
