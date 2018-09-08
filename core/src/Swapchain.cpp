#include "vpr_stdafx.h"
#include "Swapchain.hpp"
#include "vkAssert.hpp"
#include "CreateInfoBase.hpp"
#include "Instance.hpp"
#include "PhysicalDevice.hpp"
#include "LogicalDevice.hpp"
#include "SurfaceKHR.hpp"
#include <vector>
#include <algorithm>
#include <unordered_map>
#define GLFW_INCLUDE_VULKAN
#include "GLFW/glfw3.h"
#ifdef APIENTRY
#undef APIENTRY
#endif
#include "easylogging++.h"

namespace vpr {
    
    struct SwapchainImpl {

        SwapchainImpl(const Device* device, GLFWwindow* window, VkSurfaceKHR surface, uint32_t mode);
        ~SwapchainImpl();

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
        uint32_t desiredSyncMode;
        VkSurfaceFormatKHR surfaceFormat;
        VkSwapchainCreateInfoKHR createInfo;
        VkSwapchainKHR handle = VK_NULL_HANDLE;
        VkSwapchainKHR oldHandle = VK_NULL_HANDLE;
        VkSurfaceKHR surface = VK_NULL_HANDLE;
        GLFWwindow* window;
        const Device* device;
    };

    SwapchainImpl::SwapchainImpl(const Device * _device, GLFWwindow* _window, VkSurfaceKHR _surface, uint32_t mode) :
        info(_device->GetPhysicalDevice().vkHandle(), _surface), desiredSyncMode(mode), surface(_surface), window(_window), device(_device) {
        create();
    }

    SwapchainImpl::~SwapchainImpl() {
        destroy();
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
            else if (mode == VK_PRESENT_MODE_FIFO_KHR) {
                return VK_PRESENT_MODE_FIFO_KHR;
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

    Swapchain::Swapchain(const Device * _device, GLFWwindow* window, VkSurfaceKHR surface, uint32_t mode) : impl(new SwapchainImpl(_device, window, surface, mode)) {}

    Swapchain::~Swapchain(){
        Destroy();
    }

    void Swapchain::Recreate(VkSurfaceKHR new_surface) {
        impl->imageCount = 0;
        impl->surface = new_surface;
        impl->info = SwapchainImpl::SwapchainInfo(impl->device->GetPhysicalDevice().vkHandle(), new_surface);
        impl->create();
    }

    void SwapchainImpl::destroy() {
        for (const auto& view : imageViews) {
            if (view != VK_NULL_HANDLE) {
                vkDestroyImageView(device->vkHandle(), view, nullptr);
            }
        }

        if (handle != VK_NULL_HANDLE) {
            oldHandle = handle;
            vkDestroySwapchainKHR(device->vkHandle(), handle, nullptr);
        }

        imageViews.clear(); imageViews.shrink_to_fit();
        handle = VK_NULL_HANDLE;
    }

    static const std::unordered_map<VkPresentModeKHR, std::string> present_mode_strings{
        { VK_PRESENT_MODE_IMMEDIATE_KHR, "VK_PRESENT_MODE_IMMEDIATE_KHR" },
        { VK_PRESENT_MODE_FIFO_KHR, "VK_PRESENT_MODE_FIFO_KHR" },
        { VK_PRESENT_MODE_FIFO_RELAXED_KHR, "VK_PRESENT_MODE_FIFO_RELAXED_KHR" },
        { VK_PRESENT_MODE_MAILBOX_KHR, "VK_PRESENT_MODE_MAILBOX_KHR" },
        { VK_PRESENT_MODE_SHARED_DEMAND_REFRESH_KHR, "VK_PRESENT_MODE_SHARED_DEMAND_REFRESH_KHR" },
        { VK_PRESENT_MODE_SHARED_CONTINUOUS_REFRESH_KHR, "VK_PRESENT_MODE_SHARED_CONTINUOUS_REFRESH_KHR" }
    };

    std::string getPresentModeStr(const VkPresentModeKHR& mode) {
        auto iter = present_mode_strings.find(mode);
        if (iter != std::cend(present_mode_strings)) {
            return iter->second;
        }
        else {
            return "INVALID_PRESENT_MODE_ENUM_VALUE";
        }
    }

    void SwapchainImpl::setParameters() {

        surfaceFormat = info.GetBestFormat();
        colorFormat = surfaceFormat.format;
        extent = info.ChooseSwapchainExtent(window);

        presentMode = info.GetBestPresentMode();
        VkPresentModeKHR desiredMode = VK_PRESENT_MODE_BEGIN_RANGE_KHR;
        switch (desiredSyncMode) {
        case vertical_sync_mode::None:
            desiredMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
            break;
        case vertical_sync_mode::VerticalSync:
            desiredMode = VK_PRESENT_MODE_FIFO_KHR;
            break;
        case vertical_sync_mode::VerticalSyncRelaxed:
            desiredMode = VK_PRESENT_MODE_FIFO_RELAXED_KHR;
            break;
        case vertical_sync_mode::VerticalSyncMailbox:
            desiredMode = VK_PRESENT_MODE_MAILBOX_KHR;
            break;
        default:
            desiredMode = presentMode;
            break;
        }

        if (desiredMode != presentMode) {
            auto& avail_modes = info.PresentModes;
            auto iter = std::find(std::begin(avail_modes), std::end(avail_modes), desiredMode);
            if (iter == std::cend(avail_modes)) {
                LOG(WARNING) << "Desired vertical sync mode is " << getPresentModeStr(desiredMode) << "not available on current hardware!";
                LOG(INFO) << "Falling back to supported present mode " << getPresentModeStr(presentMode);
            }
            else {
                presentMode = *iter;
            }
        }

        // Create one more image than minspec to implement triple buffering (in hope we got mailbox present mode)
        if (presentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
            imageCount = info.Capabilities.minImageCount + 1;
            if (info.Capabilities.maxImageCount > 0 && imageCount > info.Capabilities.maxImageCount) {
                imageCount = info.Capabilities.maxImageCount;
            }
        }
        else if (presentMode == VK_PRESENT_MODE_FIFO_KHR || presentMode ==  VK_PRESENT_MODE_FIFO_RELAXED_KHR) {
            imageCount = info.Capabilities.minImageCount + 1;
            if (info.Capabilities.maxImageCount > 0 && imageCount > info.Capabilities.maxImageCount) {
                imageCount = info.Capabilities.maxImageCount;
            }
        }
        else {
            imageCount = info.Capabilities.minImageCount;
        }

    }

    void SwapchainImpl::setupCreateInfo() {

        createInfo = vk_swapchain_create_info_base;
        createInfo.surface = surface;
        createInfo.imageFormat = surfaceFormat.format;
        createInfo.imageColorSpace = surfaceFormat.colorSpace;
        colorSpace = surfaceFormat.colorSpace;
        createInfo.imageExtent = extent;
        createInfo.imageArrayLayers = 1;
        createInfo.minImageCount = imageCount;
        createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

        const uint32_t indices[2] = { static_cast<uint32_t>(device->QueueFamilyIndices.Present), static_cast<uint32_t>(device->QueueFamilyIndices.Graphics) };
        if ((device->QueueFamilyIndices.Present != device->QueueFamilyIndices.Graphics) && (device->QueueFamilyIndices.Present != std::numeric_limits<uint32_t>::max())) {
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

    void RecreateSwapchainAndSurface(Swapchain* swap, SurfaceKHR* surface) {
        swap->Destroy();
        surface->Recreate();
        swap->Recreate(surface->vkHandle());
    }

}
