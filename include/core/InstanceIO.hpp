#ifndef VULPES_VK_INSTANCE_IO_H
#define VULPES_VK_INSTANCE_IO_H
#include "vpr_stdafx.h"

namespace vulpes {

    struct VulpesIO {
        
        std::array<bool, 6> Mouse;
        std::array<bool, 1024> Keys;
        glm::vec2 MousePos;
        glm::vec2 MouseDelta;

    private:

        glm::vec2 lastMousePos;
    };

}

#endif //!VULPES_VK_INSTANCE_IO_H