#pragma once
#ifndef VULPES_VK_MULTISAMPLING_H
#define VULPES_VK_MULTISAMPLING_H
#include "vpr_stdafx.h"
#include "ForwardDecl.hpp"
#include "resource/Image.hpp"

namespace vpr {

    /** MSAA in Vulkan requires manually creating, binding, and managing the images that will be multisampled from, along with 
    *   making sure to update them on swapchain recreation, setting the correct resolve images in the renderpass, and attaching
    *   them to the current renderpass (if enabled).
    *
    *   This class handles creating the required images, for both the color and depth buffers. The multisampling sample count is
    *   set based on the MSAA_SampleCount member of the VulpesInstanceConfig struct.
    *   \ingroup Rendering
    */
    class VPR_API Multisampling {
        Multisampling(const Multisampling&) = delete;
        Multisampling& operator=(const Multisampling&) = delete;
    public:

        Multisampling(const Device* dvc, const Swapchain* swapchain, const VkSampleCountFlagBits& sample_count);
        ~Multisampling();

        // Objects we sample from
        std::unique_ptr<Image> ColorBufferMS, DepthBufferMS;
        static VkSampleCountFlagBits SampleCount;
    private:

        VkSampleCountFlagBits sampleCount;
        const Device* device;
    };

}

#endif // !VULPES_VK_Multisampling_H
