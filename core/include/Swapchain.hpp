#pragma once
#ifndef VULPES_VK_SWAPCHAIN_H
#define VULPES_VK_SWAPCHAIN_H
#include "vpr_stdafx.h"
#include "ForwardDecl.hpp"
#include <memory>

namespace vpr {

    struct SwapchainImpl;

    /**Used to select which of the presentation modes are to be used by a Swapchain. Consult Vulkan docs for more info on presentation modes. Currently missing shared presentation modes for the mobile-specific swapchain extensions for better presentation.
     * \ingroup Core
    */
    enum class vertical_sync_mode : uint32_t {
        /**Aliases to immediate present mode. No buffering, high incidences of tearing.*/
        None = 0,
        /**Aliases to simple FIFO mode - vertical sync and double-buffering, effectively.*/
        VerticalSync = 1,
        /**Aliases to relaxed FIFO mode - if a frame is missed, tearing is allowed. Efficient and most effective on mobile platforms.*/
        VerticalSyncRelaxed = 2,
        /**Aliases to Vulkan's mailbox mode, which effectively becomes triple-buffering.*/
        VerticalSyncMailbox = 3,
        /**Aliases to demand-refresh shared mode. Application may refresh as it wishes, but will also guarantee that it refreshes on a call to present.*/
        SharedDemandRefresh = 4,
        /**Alias to the continued-refresh mode. Swapchain will continously refresh the contents of the screen as it sees fit, and makes no guarantee of a refresh upon a call to present.*/
        SharedContinuousRefresh = 5,
    };
    

    /**This class abstracts away much of the detailed work and boilerplate code required to setup a swapchain in Vulkan. Swapchain recreation
    *  can be done by calling the suitable method, or by using the static method as well.
    *  \ingroup Core
    */
    class VPR_API Swapchain {
        Swapchain(const Swapchain&) = delete;
        Swapchain& operator=(const Swapchain&) = delete;
    public:

        /**Creates a new swapchain on the specified device.
         * \param platform_window Is either a GLFWwindow/SDL_Window pointer, or an ANativeWindow pointer
         * \param surface Surface this swapchain will be presenting images to
         * \param sync_mode One of the potential vertical_sync_mode enum values specified what presentation mode to use. Defaults to mailbox.
        */
        Swapchain(const Device* _device, void* platform_window, VkSurfaceKHR surface, vertical_sync_mode sync_mode);
        ~Swapchain();

        /**Required as part of a swapchain recreation event - must take a parameter to the new surface to use for the updated swapchain.*/
        void Recreate(VkSurfaceKHR surface);
        void Destroy();

        const VkSwapchainKHR& vkHandle() const noexcept;
        /**Returns current extent of the swapchain. Set based on platform window values - which are generally set before this object is created, or is set based on limits for the platform to satisfy initialization requirements.*/
        const VkExtent2D& Extent() const noexcept;
        /**Number of images this swapchain has created and expects to use during the rendering process.*/
        const uint32_t& ImageCount() const noexcept;
        const VkColorSpaceKHR& ColorSpace() const noexcept;
        const VkFormat& ColorFormat() const noexcept;
        /**Returns handles to the implementation-created and managed backing images used for presentation.*/
        const VkImage& Image(const size_t& idx) const;
        /**Returns handles to the implementation-created and managed backing image views used for presentation.*/
        const VkImageView& ImageView(const size_t& idx) const;

    private:
        std::unique_ptr<SwapchainImpl> impl;
    };

    /**Pass a swapchain and surface pointer to this to have the swapchain and surface destroyed and recreated
    *  in the proper order. If done incorrectly, the validation layers will give you errors about a surface being
    *  destroyed before it's swapchain is (in the best case), or crash in the worst case
    * \ingroup Core
    */
    void VPR_API RecreateSwapchainAndSurface(Swapchain* swap, SurfaceKHR* surface);

}
#endif // !VULPES_VK_SWAPCHAIN_H
