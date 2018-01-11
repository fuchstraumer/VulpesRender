#pragma once
#ifndef VULPES_VK_SWAPCHAIN_H
#define VULPES_VK_SWAPCHAIN_H
#include "vpr_stdafx.h"
#include "ForwardDecl.hpp"

namespace vpr {

    /** This class abstracts away much of the detailed work and boilerplate code required to setup a swapchain in Vulkan. Init() only needs to be called once 
    *   during runtime: recreating the swapchain is easily accomplished using the appropriate recreation method. 
    *   \todo Clean this up, update it for new coding standards/styles, and make more members private + add const access methods.
    *   \ingroup Rendering.
    */
    class VPR_API Swapchain {
        Swapchain(const Swapchain&) = delete;
        Swapchain& operator=(const Swapchain&) = delete;
    public:

        Swapchain(const Instance* _instance, const Device* _device);
        ~Swapchain();
        void Recreate();
        void Destroy();

        const VkSwapchainKHR& vkHandle() const noexcept;
        const VkExtent2D& Extent() const noexcept;
        const uint32_t& ImageCount() const noexcept;
        const VkColorSpaceKHR& ColorSpace() const noexcept;
        const VkImage& Image(const size_t& idx) const;
        const VkImageView& ImageView(const size_t& idx) const;

    private:


        /** SwapchainInfo takes care of hiding away much of the setup work required to create a swapchain. However, it does contain some data
        *   that may be useful, like the presentation mode being used or the color format of the surface object being used.
        *   \ingroup Rendering
        */
        struct SwapchainInfo {
            SwapchainInfo(const VkPhysicalDevice& dvc, const VkSurfaceKHR& sfc);
            VkSurfaceCapabilitiesKHR Capabilities;
            std::vector<VkSurfaceFormatKHR> Formats;
            std::vector<VkPresentModeKHR> PresentModes;
            VkSurfaceFormatKHR GetBestFormat() const;
            VkPresentModeKHR GetBestPresentMode() const;
            VkExtent2D ChooseSwapchainExtent(GLFWwindow* win) const;
        } info;

        void create();

        VkFormat colorFormat;
        uint32_t imageCount;
        VkColorSpaceKHR colorSpace;
        VkExtent2D extent;

        std::vector<VkImage> images;
        std::vector<VkImageView> imageViews;
        void setParameters();
        void setupCreateInfo();
        void setupSwapImages();
        void setupImageViews();

        VkPresentModeKHR presentMode;
        VkSurfaceFormatKHR surfaceFormat;
        VkSwapchainCreateInfoKHR createInfo;
        VkSwapchainKHR handle = VK_NULL_HANDLE;
        const Instance* instance;
        const PhysicalDevice* phys_device;
        const Device* device;
    };
}
#endif // !VULPES_VK_SWAPCHAIN_H
