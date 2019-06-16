#pragma once

#include <vulkan/vulkan.h>

#if defined(__GNUC__)
    #define EXPORT __attribute__((visibility("default")))
    #define IMPORT
#else
    #define EXPORT __declspec(dllexport)
    #define IMPORT __declspec(dllimport)
#endif

#ifdef VPR_DLL
#define VPR_API IMPORT
#elif defined(VPR_BUILD_DLL)
#define VPR_API EXPORT
#else
#define VPR_API
#endif

#if defined(VPR_BUILD_DLL) && defined(_MSC_VER)
// Warning about internal members not having a DLL interface: not needed and redundant.
// We're aware of structures not being exposed across the DLL, and don't want internal members available anyways
#pragma warning(disable : 4251)
#endif

#ifdef VPR_VERBOSE_LOGGING
namespace vpr {
    constexpr bool VERBOSE_LOGGING = true;
}
#else
namespace vpr {
    constexpr bool VERBOSE_LOGGING = false;
}
#endif

namespace vpr {
    // Note: this dummy forward declaration is required for the doxygen-mcss system to generate documentation for the namespace. That's all!
    /**@namespace vpr
     * @brief The VulpesRender namespace containing this libraries objects.
     */
}
