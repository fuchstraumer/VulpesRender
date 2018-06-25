#include "vpr_stdafx.h"
#include "Renderpass.hpp"
#include "vkAssert.hpp"
#include "CreateInfoBase.hpp"
#include <vector>

namespace vpr {

    struct RenderpassImpl {
        RenderpassImpl() = default;
        ~RenderpassImpl() = default;
        RenderpassImpl(RenderpassImpl&& other) noexcept : clearValues(std::move(other.clearValues)), allocators(nullptr) {}
        RenderpassImpl& operator=(RenderpassImpl&& other) noexcept {
            clearValues = std::move(other.clearValues);
            allocators = other.allocators;
            return *this;
        }
        std::vector<VkClearValue> clearValues;
        const VkAllocationCallbacks* allocators = nullptr;
    };

    Renderpass::Renderpass(const VkDevice& dvc, const VkRenderPassCreateInfo & create_info) : parent(dvc), createInfo(create_info), beginInfo(vk_renderpass_begin_info_base) {
        VkResult result = vkCreateRenderPass(parent, &create_info, impl->allocators, &handle);
        VkAssert(result);
    }

    Renderpass::Renderpass(Renderpass && other) noexcept : beginInfo(std::move(other.beginInfo)), createInfo(std::move(other.createInfo)), handle(std::move(other.handle)), parent(other.parent), impl(std::move(other.impl)) {
        other.handle = VK_NULL_HANDLE;
    }

    Renderpass & Renderpass::operator=(Renderpass && other) noexcept {
        handle = std::move(other.handle);
        createInfo = std::move(other.createInfo);
        beginInfo = std::move(other.beginInfo);
        parent = other.parent;
        impl = std::move(other.impl);
        other.impl.reset();
        other.handle = VK_NULL_HANDLE;
        return *this;
    }

    Renderpass::~Renderpass(){
        Destroy();
    }

    void Renderpass::SetupBeginInfo(const VkClearValue* clear_values, const size_t num_values, const VkExtent2D & render_area) {

        impl->clearValues = std::move(std::vector<VkClearValue>{ clear_values, clear_values + num_values });
        impl->clearValues.shrink_to_fit();

        beginInfo = VkRenderPassBeginInfo{
            VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
            nullptr,
            this->vkHandle(),
            VK_NULL_HANDLE,
            VkRect2D{ VkOffset2D{ 0, 0 }, VkExtent2D{ render_area } },
            static_cast<uint32_t>(impl->clearValues.size()),
            impl->clearValues.data()
        };

    }

    void Renderpass::UpdateBeginInfo(const VkFramebuffer & current_framebuffer) {
        beginInfo.framebuffer = current_framebuffer;
    }

    void Renderpass::Destroy(){
        if (handle != VK_NULL_HANDLE) {
            vkDestroyRenderPass(parent, handle, impl->allocators);
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
