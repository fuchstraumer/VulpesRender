#include "stdafx.h"
#include "Framebuffer.h"
#include "engine/renderer\core\LogicalDevice.h"
namespace vulpes {

	Framebuffer::~Framebuffer() {
		Destroy();
	}

	Framebuffer::Framebuffer(const Device * _parent, const VkFramebufferCreateInfo & create_info) : parent(_parent) {
		VkResult result = vkCreateFramebuffer(parent->vkHandle(), &create_info, allocators, &handle);
		VkAssert(result);
	}

	Framebuffer::Framebuffer(Framebuffer&& other) noexcept : handle(std::move(other.handle)), parent(std::move(other.parent)), allocators(std::move(other.allocators)) {}

	Framebuffer& Framebuffer::operator=(Framebuffer&& other) noexcept {
		handle = std::move(other.handle);
		parent = std::move(other.parent);
		allocators = std::move(other.allocators);
		other.handle = VK_NULL_HANDLE;
		return *this;
	}
	
	const VkFramebuffer & Framebuffer::vkHandle() const noexcept{
		return handle;
	}

	void Framebuffer::Destroy(){
		vkDestroyFramebuffer(parent->vkHandle(), handle, allocators);
	}


}