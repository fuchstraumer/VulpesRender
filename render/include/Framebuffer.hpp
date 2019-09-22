#pragma once
#ifndef VULPES_VK_FRAMEBUFFER_H
#define VULPES_VK_FRAMEBUFFER_H
#include "vpr_stdafx.h"
#include "ForwardDecl.hpp"

namespace vpr
{

    /**The Framebuffer class merely handles lifetime of a VkFramebuffer object. All important information on
    *  how to setup this class is provided by the VkFramebufferCreateInfo struct in the constructor. Destroy()
    *  method has been publicly exposed to allow one to keep the object around, but destroy and re-create it for
    *  swapchain events.
    *  \ingroup Rendering
    */
    class VPR_API Framebuffer
    {
        Framebuffer(const Framebuffer&) = delete;
        Framebuffer& operator=(const Framebuffer&) = delete;
    public:
        Framebuffer(const VkDevice& parent, const VkFramebufferCreateInfo& create_info);
        ~Framebuffer();

        Framebuffer& operator=(Framebuffer&& other) noexcept;
        Framebuffer(Framebuffer&& other) noexcept;

        const VkFramebuffer& vkHandle() const noexcept;
        /**Destroys the object by calling vkDestroyFramebuffer() on it.*/
        void Destroy();

    private:
        VkDevice parent;
        VkFramebuffer handle;
    };

}
#endif // !VULPES_VK_FRAMEBUFFER_H
