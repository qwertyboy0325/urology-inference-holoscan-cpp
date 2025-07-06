/**
 * @file holoscan_fix.hpp
 * @brief Compatibility header for Holoscan SDK 3.3.0
 * 
 * This header provides compatibility fixes for Holoscan SDK 3.3.0.
 * Note: In Holoscan 3.3.0, logging macros are properly prefixed with HOLOSCAN_LOG_
 * so there are no conflicts with custom logging systems.
 */

#ifndef HOLOSCAN_FIX_HPP
#define HOLOSCAN_FIX_HPP

// Include Holoscan headers
#include <holoscan/holoscan.hpp>

// Note: No macro conflicts in Holoscan 3.3.0 as all logging macros are prefixed
// with HOLOSCAN_LOG_ (e.g., HOLOSCAN_LOG_TRACE, HOLOSCAN_LOG_DEBUG, etc.)

#endif // HOLOSCAN_FIX_HPP 