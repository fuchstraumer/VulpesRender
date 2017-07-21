#include "stdafx.h"
#include "Renderpass.h"
#include "engine/renderer\core\LogicalDevice.h"
namespace vulpes {
	Renderpass::Renderpass(const Device* dvc, const VkRenderPassCreateInfo & create_info) : parent(dvc) {
		VkResult result = vkCreateRenderPass(dvc->vkHandle(), &create_info, allocators, &handle);
		VkAssert(result);
	}

	Renderpass::~Renderpass(){
		vkDestroyRenderPass(parent->vkHandle(), handle, allocators);
	}

	void Renderpass::Destroy(){
		vkDestroyRenderPass(parent->vkHandle(), handle, allocators);
	}

	const VkRenderPass & Renderpass::vkHandle() const noexcept{
		return handle;
	}

	Renderpass::operator VkRenderPass() const noexcept{
		return handle;
	}

	const VkRenderPassCreateInfo & Renderpass::CreateInfo() const noexcept{
		return createInfo;
	}

}
