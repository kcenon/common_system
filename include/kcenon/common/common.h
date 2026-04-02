// BSD 3-Clause License
// Copyright (c) 2025, 🍀☀🌕🌥 🌊
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
#include "interfaces/thread_pool_interface.h"
#include "interfaces/database_interface.h"
#include "interfaces/logger_interface.h"
#include "interfaces/global_logger_registry.h"

/// Unified logging (Issue #175)
#include "logging/log_functions.h"
#include "logging/log_macros.h"

/// Bootstrap (Issue #176)
#include "bootstrap/system_bootstrapper.h"

/// Common patterns
#include "patterns/result.h"
#include "patterns/event_bus.h"

/// Common utilities
#include "utils/object_pool.h"
#include "utils/circular_buffer.h"

// Note: Build-time configuration is handled by CMake. No CMake includes here.

namespace kcenon::common {

/**
 * @brief Version information for Common System
 *
 * This structure provides compile-time version information for the common_system
 * library. It follows Semantic Versioning (SemVer) 2.0.0 principles with an
 * additional tweak component for build identification.
 *
 * Version number format: MAJOR.MINOR.PATCH.TWEAK
 * - MAJOR: Incremented for incompatible API changes
 * - MINOR: Incremented for backward-compatible functionality additions
 * - PATCH: Incremented for backward-compatible bug fixes
 * - TWEAK: Build metadata for internal tracking (not part of SemVer 2.0.0)
 *
 * Note on Tweak Field:
 * The tweak field is an extension beyond SemVer 2.0.0 specification. It serves
 * as build metadata for:
 * - CI/CD pipeline build numbers
 * - Internal release candidates (e.g., 1.0.0.1, 1.0.0.2 for RC1, RC2)
 * - Distinguishing multiple builds of the same version
 *
 * For SemVer-compliant version comparison, only compare major.minor.patch.
 * The tweak field should not affect version precedence in dependency resolution.
 *
 * Usage:
 * @code
 * // Check version at compile time
 * static_assert(kcenon::common::version_info::major >= 1,
 *               "Requires common_system v1.0.0 or later");
 *
 * // Print version at runtime
 * std::cout << "common_system version: "
 *           << kcenon::common::version_info::string << std::endl;
 *
 * // SemVer-compliant comparison (ignore tweak)
 * bool is_newer = (v1.major > v2.major) ||
 *                 (v1.major == v2.major && v1.minor > v2.minor) ||
 *                 (v1.major == v2.major && v1.minor == v2.minor &&
 *                  v1.patch > v2.patch);
 * @endcode
 *
 * @see https://semver.org/ for Semantic Versioning specification
 */
struct version_info {
    /// Major version - incremented for breaking changes
    static constexpr int major = 0;
    /// Minor version - incremented for new features
    static constexpr int minor = 2;
    /// Patch version - incremented for bug fixes
    static constexpr int patch = 0;
    /**
     * @brief Tweak version - build metadata (not part of SemVer 2.0.0)
     *
     * This field extends SemVer for internal use cases:
     * - CI/CD build numbers
     * - Release candidate identification
     * - Multiple builds of the same semantic version
     *
     * @note Do not use for version precedence comparisons.
     *       SemVer 2.0.0 specifies only MAJOR.MINOR.PATCH for ordering.
     */
    static constexpr int tweak = 0;
    /// Version as human-readable string (MAJOR.MINOR.PATCH.TWEAK format)
    static constexpr const char* string = "0.2.0.0";
};

} // namespace kcenon::common
