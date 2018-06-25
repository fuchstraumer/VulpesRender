#include "vpr_stdafx.h"
#include "resource/Image.hpp"
#include "core/LogicalDevice.hpp"
#include "command/CommandPool.hpp"
#include "alloc/Allocator.hpp"
#include "alloc/AllocationRequirements.hpp"
#include "common/vkAssert.hpp"
#include "common/CreateInfoBase.hpp"

namespace vpr {

    Image::Image(const Device * _parent) : parent(_parent), handle(VK_NULL_HANDLE), view(VK_NULL_HANDLE) {}

    Image::Image(Image && other) noexcept : handle(std::move(other.handle)), createInfo(std::move(other.createInfo)), view(std::move(other.view)), 
        memoryAllocation(std::move(other.memoryAllocation)), finalLayout(std::move(other.finalLayout)), parent(std::move(other.parent)) {
        other.handle = VK_NULL_HANDLE;
        other.view = VK_NULL_HANDLE;
    }

    Image & Image::operator=(Image && other) noexcept {
        handle = std::move(other.handle);
        other.handle = VK_NULL_HANDLE;
        view = std::move(other.view);
        other.view = VK_NULL_HANDLE;
        createInfo = std::move(other.createInfo);
        memoryAllocation = std::move(other.memoryAllocation);
        finalLayout = std::move(other.finalLayout);
        parent = std::move(other.parent);
        return *this;
    }

    Image::~Image() {
        Destroy();
    }

    void Image::Destroy(){
        
        if (view != VK_NULL_HANDLE) {
            vkDestroyImageView(parent->vkHandle(), view, nullptr);
            view = VK_NULL_HANDLE;
        }

        if (handle != VK_NULL_HANDLE) {
            parent->GetAllocator()->DestroyImage(handle, memoryAllocation);
            handle = VK_NULL_HANDLE;
        }

    }

    void Image::Create(const VkImageCreateInfo & create_info, const VkMemoryPropertyFlagBits& memory_flags) {
        this->createInfo = create_info;
        CreateImage(handle, memoryAllocation, parent, createInfo, memory_flags);
    }

    void Image::CreateView(const VkImageViewCreateInfo & info){
        VkResult result = vkCreateImageView(parent->vkHandle(), &info, allocators, &view);
        VkAssert(result);
    }

    void Image::CreateView(const VkImageAspectFlags & aspect_flags){
        VkImageViewCreateInfo view_info = vk_image_view_create_info_base;
        view_info.subresourceRange.aspectMask = aspect_flags;
        view_info.image = handle;
        view_info.format = createInfo.format;
        VkResult result = vkCreateImageView(parent->vkHandle(), &view_info, allocators, &view);
        VkAssert(result);
    }

    void Image::TransitionLayout(const VkImageLayout & initial, const VkImageLayout & final, CommandPool* pool, VkQueue & queue) {
        VkImageMemoryBarrier barrier = vk_image_memory_barrier_base;
        barrier = GetMemoryBarrier(handle, createInfo.format, initial, final);
        VkCommandBuffer cmd = pool->StartSingleCmdBuffer();
            vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);    
        pool->EndSingleCmdBuffer(cmd, queue);
    }

    VkImageMemoryBarrier Image::GetMemoryBarrier(const VkImage& image, const VkFormat& img_format, const VkImageLayout & prev, const VkImageLayout & next){

        auto has_stencil = [&]()->bool {
            return img_format == VK_FORMAT_D32_SFLOAT_S8_UINT || img_format == VK_FORMAT_D24_UNORM_S8_UINT || img_format == VK_FORMAT_D32_SFLOAT;
        };

        VkImageMemoryBarrier barrier = vk_image_memory_barrier_base;
        barrier.oldLayout = prev;
        barrier.newLayout = next;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = image;
        barrier.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

        // Set right aspect mask.
        if (next == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
            barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
            if (has_stencil()) {
                barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
            }
        }

        switch (prev) {
        case VK_IMAGE_LAYOUT_UNDEFINED:
            barrier.srcAccessMask = 0;
            break;
        case VK_IMAGE_LAYOUT_PREINITIALIZED:
            barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
            break;
        case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
            barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            break;
        case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
            barrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
            break;
        case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
            break;
        case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            break;
        case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
            barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
            break;
        default:
            break;
        }

        switch (next) {
        case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            break;
        case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
            break;
        case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
            barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            break;
        case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
            barrier.dstAccessMask = barrier.dstAccessMask | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
            break;
        case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
            if (barrier.srcAccessMask == 0) {
                barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
            }
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            break;
        default:
            break;
        }
        
        return barrier;
    }

    void Image::CreateImage(VkImage & dest_image, Allocation& dest_alloc, const Device * parent, const VkImageCreateInfo & create_info, const VkMemoryPropertyFlags & memory_flags) {
        
        AllocationRequirements alloc_reqs;
        alloc_reqs.requiredFlags = memory_flags;

        VkResult result = parent->GetAllocator()->CreateImage(&dest_image, &create_info, alloc_reqs, dest_alloc);
        VkAssert(result);

    }

    const VkImageCreateInfo & Image::CreateInfo() const noexcept {
        return createInfo;
    }

    const VkImage & Image::vkHandle() const noexcept{
        return handle;
    }

    const VkImageView & Image::View() const noexcept{    
        return view;
    }

    const Allocation& Image::MemoryAllocation() const noexcept {
        return memoryAllocation;
    }
    
    VkFormat Image::Format() const noexcept {
        return createInfo.format;
    }

    void Image::SetFormat(VkFormat _format) noexcept {
        createInfo.format = std::move(_format);
    }

    VkImageLayout Image::Layout() const noexcept {
        return finalLayout;
    }

    void Image::SetFinalLayout(VkImageLayout _layout) {
        finalLayout = std::move(_layout);
    }


}
