/**
 * @file common.h
 * @brief Main header file for common_system
 *
 * This header includes all common interfaces and patterns
 * provided by the common_system module.
 */

#pragma once

// Core interfaces
#include "interfaces/executor_interface.h"

// Common patterns
#include "patterns/result.h"
#include "patterns/event_bus.h"

// Configuration (build-time only)
// Note: CMake configuration is handled in CMake scripts, not headers.

namespace kcenon {
namespace common {

/**
 * @brief Version information for common_system
 */
struct version_info {
    static constexpr int major = 1;
    static constexpr int minor = 0;
    static constexpr int patch = 0;
    static constexpr const char* string = "1.0.0";
};

} // namespace common
} // namespace kcenon
