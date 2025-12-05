/*****************************************************************************
BSD 3-Clause License

Copyright (c) 2025, 🍀☀🌕🌥 🌊
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its
   contributors may be used to endorse or promote products derived from
   this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*****************************************************************************/

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
 * library. It follows Semantic Versioning (SemVer) 2.0.0 principles.
 *
 * Version number format: MAJOR.MINOR.PATCH
 * - MAJOR: Incremented for incompatible API changes
 * - MINOR: Incremented for backward-compatible functionality additions
 * - PATCH: Incremented for backward-compatible bug fixes
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
 * // Version-specific code
 * #if (kcenon::common::version_info::major >= 2)
 *     // Use v2.x API
 * #else
 *     // Fallback to v1.x API
 * #endif
 * @endcode
 *
 * @see https://semver.org/ for Semantic Versioning specification
 */
struct version_info {
    /// Major version - incremented for breaking changes
    static constexpr int major = 1;
    /// Minor version - incremented for new features
    static constexpr int minor = 0;
    /// Patch version - incremented for bug fixes
    static constexpr int patch = 0;
    /// Version as human-readable string
    static constexpr const char* string = "1.0.0";
};

} // namespace kcenon::common
