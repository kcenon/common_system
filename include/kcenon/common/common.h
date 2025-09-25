// BSD 3-Clause License
// Copyright (c) 2025, kcenon
// See the LICENSE file in the project root for full license information.

/**
 * @file common.h
 * @brief Aggregated public header for Common System.
 *
 * This header exposes the main public interfaces and patterns of the
 * Common System module. Include this file to access the common
 * abstractions without worrying about internal structure.
 */

#pragma once

/// Core interfaces
#include "interfaces/executor_interface.h"

/// Common patterns
#include "patterns/result.h"
#include "patterns/event_bus.h"

// Note: Build-time configuration is handled by CMake. No CMake includes here.

namespace kcenon {
namespace common {

/**
 * @brief Version information for Common System.
 */
struct version_info {
    /// Major version
    static constexpr int major = 1;
    /// Minor version
    static constexpr int minor = 0;
    /// Patch version
    static constexpr int patch = 0;
    /// Version as human-readable string
    static constexpr const char* string = "1.0.0";
};

} // namespace common
} // namespace kcenon
