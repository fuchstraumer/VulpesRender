#include "vpr_stdafx.h"
#include "render/Renderpass.hpp"
#include "core/LogicalDevice.hpp"
namespace vulpes {
	Renderpass::Renderpass(const Device* dvc, const VkRenderPassCreateInfo & create_info) : parent(dvc), createInfo(create_info), beginInfo(vk_renderpass_begin_info_base) {
		VkResult result = vkCreateRenderPass(dvc->vkHandle(), &create_info, allocators, &handle);
		VkAssert(result);
	}

	Renderpass::Renderpass(Renderpass && other) noexcept : beginInfo(std::move(other.beginInfo)), createInfo(std::move(other.createInfo)), handle(std::move(other.handle)), parent(other.parent), allocators(other.allocators) {
		other.handle = VK_NULL_HANDLE;
	}

	Renderpass & Renderpass::operator=(Renderpass && other) noexcept {
		handle = std::move(other.handle);
		createInfo = std::move(other.createInfo);
        beginInfo = std::move(other.beginInfo);
		parent = other.parent;
		allocators = other.allocators;
		other.handle = VK_NULL_HANDLE;
		return *this;
	}

	Renderpass::~Renderpass(){
		Destroy();
	}

    void Renderpass::SetupBeginInfo(const std::vector<VkClearValue>& clear_values, const VkExtent2D & render_area) {

        clearValues = clear_values;
        clearValues.shrink_to_fit();

        beginInfo = VkRenderPassBeginInfo{
            VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
            nullptr,
            this->vkHandle(),
            VK_NULL_HANDLE,
            VkRect2D{ VkOffset2D{ 0, 0 }, VkExtent2D{ render_area } },
            static_cast<uint32_t>(clearValues.size()),
            clearValues.data()
        };

    }

    void Renderpass::UpdateBeginInfo(const VkFramebuffer & current_framebuffer) {
        beginInfo.framebuffer = current_framebuffer;
    }

	void Renderpass::Destroy(){
		if (handle != VK_NULL_HANDLE) {
			vkDestroyRenderPass(parent->vkHandle(), handle, allocators);
			handle = VK_NULL_HANDLE;
		}
	}

	const VkRenderPass & Renderpass::vkHandle() const noexcept{
		return handle;
	}

	const VkRenderPassCreateInfo & Renderpass::CreateInfo() const noexcept{
		return createInfo;
	}

    const VkRenderPassBeginInfo & Renderpass::BeginInfo() const noexcept {
        return beginInfo;
    }

}
