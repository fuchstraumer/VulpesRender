#include "vpr_stdafx.h"
#include "render/Multisampling.hpp"
#include "core/LogicalDevice.hpp"
#include "render/Swapchain.hpp"

VkSampleCountFlagBits vpr::Multisampling::SampleCount = VK_SAMPLE_COUNT_8_BIT;

vpr::Multisampling::Multisampling(const Device * dvc, const Swapchain * swapchain, const VkSampleCountFlagBits & sample_count) : device(dvc), sampleCount(sample_count) {

    /*
        Setup attachments to render into
    */

    VkImageCreateInfo image_info = vk_image_create_info_base;
    image_info.format = swapchain->ColorFormat;
    image_info.extent = VkExtent3D{ swapchain->Extent.width, swapchain->Extent.height, 1 };
    image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
    image_info.samples = sampleCount;
    image_info.usage = VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    image_info.arrayLayers = 1;
    image_info.mipLevels = 1;

    ColorBufferMS = std::make_unique<Image>(device);
    ColorBufferMS->Create(image_info);

    VkImageViewCreateInfo msaa_view_info = vk_image_view_create_info_base;
    msaa_view_info.image = ColorBufferMS->vkHandle();
    msaa_view_info.format = swapchain->ColorFormat;

    ColorBufferMS->CreateView(msaa_view_info);

    /*
        Create depth attachment.
    */

    DepthBufferMS = std::make_unique<Image>(device);

    image_info.format = device->FindDepthFormat();
    image_info.usage = VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

    DepthBufferMS->Create(image_info);

    msaa_view_info.image = DepthBufferMS->vkHandle();
    msaa_view_info.format = device->FindDepthFormat();
    msaa_view_info.subresourceRange = VkImageSubresourceRange{ VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, 1 };

    DepthBufferMS->CreateView(msaa_view_info);

}

vpr::Multisampling::~Multisampling() {
    ColorBufferMS.reset();
    DepthBufferMS.reset();
}
