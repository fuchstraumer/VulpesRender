#ifndef VULPES_VK_FRAMEBUFFER_PICKING_H
#define VULPES_VK_FRAMEBUFFER_PICKING_H
#include "vpr_stdafx.h"
#include "ForwardDecl.h"

namespace vulpes {

    namespace util {

        class ColorBufferPicker {
            ColorBufferPicker(const ColorBufferPicker&) = delete;
            ColorBufferPicker& operator=(const ColorBufferPicker&) = delete;
        public:

            ColorBufferPicker(const Device* dvc);
        };

    }

}
#endif //!VULPES_VK_FRAMEBUFFER_PICKING_H