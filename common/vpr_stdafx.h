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


