#include "vpr_stdafx.h"
#include "render/Swapchain.hpp"
#include "core/Instance.hpp"
#include "core/PhysicalDevice.hpp"
#include "core/LogicalDevice.hpp"
#include "GLFW/glfw3.h"

namespace vpr {

    SwapchainInfo::SwapchainInfo(const VkPhysicalDevice & dvc, const VkSurfaceKHR& sfc){
        
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

    VkSurfaceFormatKHR SwapchainInfo::GetBestFormat() const{
        
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

    VkPresentModeKHR SwapchainInfo::GetBestPresentMode() const{
        
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

    VkExtent2D SwapchainInfo::ChooseSwapchainExtent(GLFWwindow* window) const{
        
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

    Swapchain::~Swapchain(){
        Destroy();
    }

    void Swapchain::Init(const Instance * _instance, const PhysicalDevice * _phys_device, const Device * _device) {
        
        instance = _instance;
        device = _device;
        phys_device = _phys_device;

        setParameters();
        setupCreateInfo();

        VkResult result = vkCreateSwapchainKHR(device->vkHandle(), &createInfo, AllocCallbacks, &handle);
        VkAssert(result);

        setupSwapImages();
        setupImageViews();

    }

    void Swapchain::Recreate() {
        
        Info.reset();
        
        ImageCount = 0;
        setParameters();
        setupCreateInfo();

        VkResult result = vkCreateSwapchainKHR(device->vkHandle(), &createInfo, AllocCallbacks, &handle);
        VkAssert(result);

        setupSwapImages();
        setupImageViews();

    }

    void Swapchain::setParameters() {

        Info = std::make_unique<SwapchainInfo>(phys_device->vkHandle(), instance->vkSurface());
        surfaceFormat = Info->GetBestFormat();
        ColorFormat = surfaceFormat.format;
        presentMode = Info->GetBestPresentMode();
        Extent = Info->ChooseSwapchainExtent(instance->GetGLFWwindow());

        // Create one more image than minspec to implement triple buffering (in hope we got mailbox present mode)
        ImageCount = Info->Capabilities.minImageCount + 1;
        if (Info->Capabilities.maxImageCount > 0 && ImageCount > Info->Capabilities.maxImageCount) {
            ImageCount = Info->Capabilities.maxImageCount;
        }

    }

    void Swapchain::setupCreateInfo() {

        createInfo = vk_swapchain_create_info_base;
        createInfo.surface = instance->vkSurface();
        createInfo.imageFormat = surfaceFormat.format;
        createInfo.imageColorSpace = surfaceFormat.colorSpace;
        createInfo.imageExtent = Extent;
        createInfo.imageArrayLayers = 1;
        createInfo.minImageCount = ImageCount;
        createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

        uint32_t indices[2] = { static_cast<uint32_t>(device->QueueFamilyIndices.Present), static_cast<uint32_t>(device->QueueFamilyIndices.Graphics) };
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

        createInfo.preTransform = Info->Capabilities.currentTransform;
        createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        createInfo.presentMode = presentMode;
        createInfo.clipped = VK_TRUE;
        createInfo.oldSwapchain = VK_NULL_HANDLE;

    }

    void Swapchain::setupSwapImages() {

        // Setup swap images
        vkGetSwapchainImagesKHR(device->vkHandle(), handle, &ImageCount, nullptr);
        Images.resize(ImageCount);
        vkGetSwapchainImagesKHR(device->vkHandle(), handle, &ImageCount, Images.data());

    }

    void Swapchain::setupImageViews() {

        // Setup image views
        ImageViews.resize(ImageCount);
        for (uint32_t i = 0; i < ImageCount; ++i) {
            VkImageViewCreateInfo iv_info = vk_image_view_create_info_base;
            iv_info.image = Images[i];
            iv_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
            iv_info.format = ColorFormat;
            iv_info.components = { VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY };
            iv_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            iv_info.subresourceRange.baseMipLevel = 0;
            iv_info.subresourceRange.levelCount = 1;
            iv_info.subresourceRange.baseArrayLayer = 0;
            iv_info.subresourceRange.layerCount = 1;
            VkResult result = vkCreateImageView(device->vkHandle(), &iv_info, AllocCallbacks, &ImageViews[i]);
            VkAssert(result);
        }

    }

    void Swapchain::Destroy(){
        for (const auto& view : ImageViews) {
            if (view != VK_NULL_HANDLE) {
                vkDestroyImageView(device->vkHandle(), view, AllocCallbacks);
            }
        }
        if (handle != VK_NULL_HANDLE) {
            vkDestroySwapchainKHR(device->vkHandle(), handle, AllocCallbacks);
        }
    }

    const VkSwapchainKHR& Swapchain::vkHandle() const{
        return handle;
    }

    Swapchain::operator VkSwapchainKHR() const{
        return handle;
    }

}
