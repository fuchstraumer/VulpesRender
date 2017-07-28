#pragma once
#ifndef VULPES_VK_DEFERRED_H
#define VULPES_VK_DEFERRED_H

#include "vpr_stdafx.h"
#include "../ForwardDecl.h"
#include "Framebuffer.h"

namespace vulpes {

	template<typename renderpass_type>
	struct DeferredPass {

		DeferredPass(const Device* device) : Parent(device) {};

		void Begin();
		void End();

		VkRenderPass Renderpass;
		VkRenderPassBeginInfo RenderpassBeginInfo;
		OffscreenFramebuffer<renderpass_type> RenderTarget;
		VkSemaphore Semaphore = VK_NULL_HANDLE;
		const Device* Parent;
	};

	template<typename renderpass_type>
	void DeferredPass<renderpass_type>::Begin() {

		VkResult result = VK_SUCCESS;

		if (Semaphore == VK_NULL_HANDLE) {
			static const VkSemaphoreCreateInfo semaphore_info{ VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO, nullptr, 0 };
			result = vkCreateSemaphore(Parent->vkHandle(), &semaphore_info, nullptr, &Semaphore);
			VkAssert(result);
		}


	}

}

#endif // !VULPES_VK_DEFERRED_H
