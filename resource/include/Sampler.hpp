#pragma once
#ifndef VPR_SAMPLER_HPP
#define VPR_SAMPLER_HPP
#include "vpr_stdafx.h"

namespace vpr {

    /**Bare minimum RAII wrapper around Vulkan sampler object. Can be helpful in facilitating an approach
     * wherein descriptors use multiple textures, but each texture shares/uses different samplers in the shader
     * based on the kind of sampling being performed.
     * \ingroup Resources
     */
    class VPR_API Sampler {
        Sampler(const Sampler&) = delete;
        Sampler& operator=(const Sampler&) = delete;
    public:
        Sampler(const VkDevice& dvc, const VkSamplerCreateInfo& info);
        ~Sampler();
        Sampler(Sampler&& other) noexcept;
        Sampler& operator=(Sampler&& other) noexcept;

        const VkSampler& vkHandle() const noexcept;

    private:
        VkDevice device{ VK_NULL_HANDLE };
        VkSampler handle{ VK_NULL_HANDLE };
    };

}

#endif //!VPR_SAMPLER_HPP
