#pragma once
#ifndef VULPES_VK_SWAPCHAIN_H
#define VULPES_VK_SWAPCHAIN_H
#include "vpr_stdafx.h"
#include "ForwardDecl.hpp"

namespace vpr {

    struct SwapchainImpl;

    namespace vertical_sync_mode {
        enum e : uint32_t {
            None = 0,
            VerticalSync = 1,
            VerticalSyncRelaxed = 2,
            VerticalSyncMailbox = 3
        };
    }

    /** This class abstracts away much of the detailed work and boilerplate code required to setup a swapchain in Vulkan. Init() only needs to be called once 
    *   during runtime: recreating the swapchain is easily accomplished using the appropriate recreation method. 
    *   \ingroup Rendering
    */
    class VPR_API Swapchain {
        Swapchain(const Swapchain&) = delete;
        Swapchain& operator=(const Swapchain&) = delete;
    public:

        Swapchain(const Instance* _instance, const Device* _device, uint32_t sync_mode);
        ~Swapchain();
        void Recreate();
        void Destroy();

        const VkSwapchainKHR& vkHandle() const noexcept;
        const VkExtent2D& Extent() const noexcept;
        const uint32_t& ImageCount() const noexcept;
        const VkColorSpaceKHR& ColorSpace() const noexcept;
        const VkFormat& ColorFormat() const noexcept;
        const VkImage& Image(const size_t& idx) const;
        const VkImageView& ImageView(const size_t& idx) const;

    private:
        SwapchainImpl* impl;
    };
}
#endif // !VULPES_VK_SWAPCHAIN_H
