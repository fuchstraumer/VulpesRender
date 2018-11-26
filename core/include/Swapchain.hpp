#pragma once
#ifndef VULPES_VK_SWAPCHAIN_H
#define VULPES_VK_SWAPCHAIN_H
#include "vpr_stdafx.h"
#include "ForwardDecl.hpp"
#include <memory>

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

    /** This class abstracts away much of the detailed work and boilerplate code required to setup a swapchain in Vulkan. Swapchain recreation
     *  can be done by calling the suitable method, or by using the static method as well.
    *   \ingroup Core
    */
    class VPR_API Swapchain {
        Swapchain(const Swapchain&) = delete;
        Swapchain& operator=(const Swapchain&) = delete;
    public:

        Swapchain(const Device* _device, void* platform_window, VkSurfaceKHR surface, uint32_t sync_mode);
        ~Swapchain();
        void Recreate(VkSurfaceKHR surface);
        void Destroy();

        const VkSwapchainKHR& vkHandle() const noexcept;
        const VkExtent2D& Extent() const noexcept;
        const uint32_t& ImageCount() const noexcept;
        const VkColorSpaceKHR& ColorSpace() const noexcept;
        const VkFormat& ColorFormat() const noexcept;
        const VkImage& Image(const size_t& idx) const;
        const VkImageView& ImageView(const size_t& idx) const;

    private:
        std::unique_ptr<SwapchainImpl> impl;
    };

    /** Pass a swapchain and surface pointer to this to have the swapchain and surface destroyed and recreated
    *   in the proper order. If done incorrectly, the validation layers will give you errors about a surface being
    *   destroyed before it's swapchain is (in the best case), or crash in the worst case
    */
    void VPR_API RecreateSwapchainAndSurface(Swapchain* swap, SurfaceKHR* surface);

}
#endif // !VULPES_VK_SWAPCHAIN_H
