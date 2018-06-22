#include "vpr_stdafx.h"
#include "render/Swapchain.hpp"
#include "common/vkAssert.hpp"
#include "common/CreateInfoBase.hpp"
#include "core/Instance.hpp"
#include "core/PhysicalDevice.hpp"
#include "core/LogicalDevice.hpp"
#include "GLFW/glfw3.h"
#include <vector>
#include <algorithm>

namespace vpr {
    
    struct SwapchainImpl {

        SwapchainImpl(const Instance* instance, const Device* device);

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

        void destroy();
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

    SwapchainImpl::SwapchainImpl(const Instance * _instance, const Device * _device) : instance(_instance), device(_device), info(_device->GetPhysicalDevice().vkHandle(), _instance->vkSurface()) {
        create();
    }

    void SwapchainImpl::create() {
        setParameters();
        setupCreateInfo();

        VkResult result = vkCreateSwapchainKHR(device->vkHandle(), &createInfo, nullptr, &handle);
        VkAssert(result);

        setupSwapImages();
        setupImageViews();
    }

    SwapchainImpl::SwapchainInfo::SwapchainInfo(const VkPhysicalDevice & dvc, const VkSurfaceKHR& sfc){
        
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(dvc, sfc, &Capabilities);
        uint32_t fmt_cnt = 0;
        
        vkGetPhysicalDeviceSurfaceFormatsKHR(dvc, sfc, &fmt_cnt, nullptr);
        if (fmt_cnt != 0) {
            Formats.resize(fmt_cnt);
            vkGetPhysicalDeviceSurfaceFormatsKHR(dvc, sfc, &fmt_cnt, Formats.data());
        }

        uint32_t present_cnt = 0;
        vkGetPhysicalDeviceSurfacePresentModesKHR(dvc, sfc, &present_cnt, nullptr);
        
        if (present_cnt != 0) {
            PresentModes.resize(present_cnt);
            vkGetPhysicalDeviceSurfacePresentModesKHR(dvc, sfc, &present_cnt, PresentModes.data());
        }

    }

    VkSurfaceFormatKHR SwapchainImpl::SwapchainInfo::GetBestFormat() const{
        
        if (Formats.size() == 1 && Formats.front().format == VK_FORMAT_UNDEFINED) {
            return { VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
        }
        else {
            for (const auto& fmt : Formats) {
                if (fmt.format == VK_FORMAT_B8G8R8A8_UNORM && fmt.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
                    return { VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
                }
            }
            return Formats.front();
        }

    }

    VkPresentModeKHR SwapchainImpl::SwapchainInfo::GetBestPresentMode() const{
        
        VkPresentModeKHR result = VK_PRESENT_MODE_FIFO_KHR;
        for (const auto& mode : PresentModes) {
            // Best mix of all modes.
            if (mode == VK_PRESENT_MODE_MAILBOX_KHR) {
                return VK_PRESENT_MODE_MAILBOX_KHR;
            }
            else if (mode == VK_PRESENT_MODE_IMMEDIATE_KHR) {
                // FIFO not always supported by driver, in which case this is our best bet.
                result = VK_PRESENT_MODE_IMMEDIATE_KHR;
            }
        }

        return result;

    }

    VkExtent2D SwapchainImpl::SwapchainInfo::ChooseSwapchainExtent(GLFWwindow* window) const{
        
        if (Capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
            return Capabilities.currentExtent;
        }
        else {
            int wx, wy;
            glfwGetWindowSize(window, &wx, &wy);
            VkExtent2D actual_extent = { static_cast<uint32_t>(wx), static_cast<uint32_t>(wy) };
            actual_extent.width = std::max(Capabilities.minImageExtent.width, std::min(Capabilities.maxImageExtent.width, actual_extent.width));
            actual_extent.height = std::max(Capabilities.minImageExtent.height, std::min(Capabilities.maxImageExtent.height, actual_extent.height));
            return actual_extent;
        }

    }

    Swapchain::Swapchain(const Instance * _instance, const Device * _device) : impl(std::make_unique<SwapchainImpl>(_instance, _device)) {}

    Swapchain::~Swapchain(){
        Destroy();
    }

    void Swapchain::Recreate() {
        const Instance* inst = impl->instance;
        const Device* dvc = impl->device;
        impl.reset();
        impl = std::make_unique<SwapchainImpl>(inst, dvc);
        impl->imageCount = 0;
        impl->info = SwapchainImpl::SwapchainInfo(impl->device->GetPhysicalDevice().vkHandle(), impl->instance->vkSurface());
        impl->create();
    }

    void SwapchainImpl::destroy() {
        for (const auto& view : imageViews) {
            if (view != VK_NULL_HANDLE) {
                vkDestroyImageView(device->vkHandle(), view, nullptr);
            }
        }
        if (handle != VK_NULL_HANDLE) {
            vkDestroySwapchainKHR(device->vkHandle(), handle, nullptr);
        }
    }

    void SwapchainImpl::setParameters() {

        surfaceFormat = info.GetBestFormat();
        colorFormat = surfaceFormat.format;
        presentMode = info.GetBestPresentMode();
        extent = info.ChooseSwapchainExtent(instance->GetGLFWwindow());

        // Create one more image than minspec to implement triple buffering (in hope we got mailbox present mode)
        imageCount = info.Capabilities.minImageCount + 1;
        if (info.Capabilities.maxImageCount > 0 && imageCount > info.Capabilities.maxImageCount) {
            imageCount = info.Capabilities.maxImageCount;
        }

    }

    void SwapchainImpl::setupCreateInfo() {

        createInfo = vk_swapchain_create_info_base;
        createInfo.surface = instance->vkSurface();
        createInfo.imageFormat = surfaceFormat.format;
        createInfo.imageColorSpace = surfaceFormat.colorSpace;
        createInfo.imageExtent = extent;
        createInfo.imageArrayLayers = 1;
        createInfo.minImageCount = imageCount;
        createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

        const uint32_t indices[2] = { static_cast<uint32_t>(device->QueueFamilyIndices.Present), static_cast<uint32_t>(device->QueueFamilyIndices.Graphics) };
        if (device->QueueFamilyIndices.Present != device->QueueFamilyIndices.Graphics) {
            createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
            createInfo.queueFamilyIndexCount = 2;
            createInfo.pQueueFamilyIndices = indices;
        }
        else {
            createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
            createInfo.queueFamilyIndexCount = 0;
            createInfo.pQueueFamilyIndices = nullptr;
        }

        createInfo.preTransform = info.Capabilities.currentTransform;
        createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        createInfo.presentMode = presentMode;
        createInfo.clipped = VK_TRUE;
        createInfo.oldSwapchain = VK_NULL_HANDLE;

    }

    void SwapchainImpl::setupSwapImages() {

        // Setup swap images
        vkGetSwapchainImagesKHR(device->vkHandle(), handle, &imageCount, nullptr);
        images.resize(imageCount);
        vkGetSwapchainImagesKHR(device->vkHandle(), handle, &imageCount, images.data());

    }

    void SwapchainImpl::setupImageViews() {

        // Setup image views
        imageViews.resize(imageCount);
        for (uint32_t i = 0; i < imageCount; ++i) {
            VkImageViewCreateInfo iv_info = vk_image_view_create_info_base;
            iv_info.image = images[i];
            iv_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
            iv_info.format = colorFormat;
            iv_info.components = { VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY };
            iv_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            iv_info.subresourceRange.baseMipLevel = 0;
            iv_info.subresourceRange.levelCount = 1;
            iv_info.subresourceRange.baseArrayLayer = 0;
            iv_info.subresourceRange.layerCount = 1;
            VkResult result = vkCreateImageView(device->vkHandle(), &iv_info, nullptr, &imageViews[i]);
            VkAssert(result);
        }

    }

    void Swapchain::Destroy(){
        impl->destroy();
        impl.reset();
    }

    const VkSwapchainKHR& Swapchain::vkHandle() const noexcept {
        return impl->handle;
    }

    const VkExtent2D& Swapchain::Extent() const noexcept {
        return impl->extent;
    }

    const uint32_t& Swapchain::ImageCount() const noexcept {
        return impl->imageCount;
    }

    const VkColorSpaceKHR& Swapchain::ColorSpace() const noexcept {
        return impl->colorSpace;
    }

    const VkFormat& Swapchain::ColorFormat() const noexcept {
        return impl->colorFormat;
    }

    const VkImage& Swapchain::Image(const size_t & idx) const {
        return impl->images[idx];
    }

    const VkImageView & Swapchain::ImageView(const size_t & idx) const {
        return impl->imageViews[idx];
    }

}
