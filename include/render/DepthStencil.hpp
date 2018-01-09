#pragma once
#ifndef VULPES_VK_DEPTH_STENCIL_H
#define VULPES_VK_DEPTH_STENCIL_H
#include "vpr_stdafx.h"
#include "resource/Image.hpp"

namespace vpr {

    /** The DepthStencil class is exactly what it says on the tin: it creates the depth-stencil image, 
    *   finding the appropriate format for the current system, and then transitions the layout into a
    *   ready-to-use layout.
    *   \todo Update the latter two parameters of the constructor to use a transfer pool (reducing to one parameter)
    *   \ingroup Rendering
    */
    class VPR_API DepthStencil : public Image {
    public:

        DepthStencil(const Device* _parent, const VkExtent3D& extents);

        ~DepthStencil() = default;

        VkAttachmentDescription DepthAttachment() const noexcept;
    };

}
#endif // !VULPES_VK_DEPTH_STENCIL_H
