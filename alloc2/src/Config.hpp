#pragma once
#ifndef VPR_ALLOC2_CONFIG_HPP
#define VPR_ALLOC2_CONFIG_HPP

#ifndef VPR_ALLOC2_CONF_DEBUG_ALIGNMENT
constexpr static bool VPR_ALLOC2_DEBUG_ALIGNMENT_ENABLED{ false };
#endif

#ifndef VPR_ALLOC2_CONF_DEBUG_MARGINS
constexpr static bool VPR_ALLOC2_DEBUG_MARGINS_ENABLED{ false };
#endif 

#ifndef VPR_ALLOC2_CONF_DEBUG_INIT_ALLOCS
constexpr static bool VPR_ALLOC2_DEBUG_INIT_ALLOCS_ENABLED{ false };
#endif

#ifndef VPR_ALLOC2_CONF_DEBUG_DETECT_CORRUPTION
constexpr static bool VPR_ALLOC2_DEBUG_DETECT_CORRUPTION{ false };
#endif

#ifndef VPR_ALLOC2_CONF_DEBUG_GLOBAL_MUTEX
constexpr static bool VPR_ALLOC2_DEBUG_GLOBAL_MUTEX_ENABLED{ false };
#endif 

constexpr static unsigned int VPR_ALLOC2_DEBUG_CORRUPTION_DETECTION_BITS{ 0x7F84E666 };
constexpr static size_t VPR_ALLOC2_DEBUG_MARGINS_VALUE{ 0u };
constexpr static size_t VPR_ALLOC2_DEBUG_ALIGNMENT{ 1u };
constexpr static size_t VPR_ALLOC2_MIN_SUBALLOCATION_SIZE_TO_REGISTER{ 16u };

#endif //!VPR_ALLOC2_CONFIG_HPP
