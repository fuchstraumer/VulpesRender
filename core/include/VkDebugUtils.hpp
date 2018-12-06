#pragma once
#ifndef VULKAN_DEBUG_UTILS_FN_POINTER_STRUCT_HPP
#define VULKAN_DEBUG_UTILS_FN_POINTER_STRUCT_HPP
#include <vulkan/vulkan.h>

namespace vpr {

    /**This structure will be populated with function pointers used to call the functions (as named)
     * when available, or requested. The extension required to support these functions will not be loaded
     * by default, and must be requested. Additionally, it requires the validation layers to load successfully.
     * \ingroup Core
     */
    struct VkDebugUtilsFunctions {
        PFN_vkSetDebugUtilsObjectNameEXT vkSetDebugUtilsObjectName{ nullptr };
        PFN_vkSetDebugUtilsObjectTagEXT vkSetDebugUtilsObjectTag{ nullptr };
        PFN_vkQueueBeginDebugUtilsLabelEXT vkQueueBeginDebugUtilsLabel{ nullptr };
        PFN_vkQueueEndDebugUtilsLabelEXT vkQueueEndDebugUtilsLabel{ nullptr };
        PFN_vkCmdBeginDebugUtilsLabelEXT vkCmdBeginDebugUtilsLabel{ nullptr };
        PFN_vkCmdEndDebugUtilsLabelEXT vkCmdEndDebugUtilsLabel{ nullptr };
        PFN_vkCmdInsertDebugUtilsLabelEXT vkCmdInsertDebugUtilsLabel{ nullptr };
        PFN_vkCreateDebugUtilsMessengerEXT vkCreateDebugUtilsMessenger{ nullptr };
        PFN_vkDestroyDebugUtilsMessengerEXT vkDestroyDebugUtilsMessenger{ nullptr };
        PFN_vkSubmitDebugUtilsMessageEXT vkSubmitDebugUtilsMessage{ nullptr };
    };

}

#endif //!VULKAN_DEBUG_UTILS_FN_POINTER_STRUCT_HPP
