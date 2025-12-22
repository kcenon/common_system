// BSD 3-Clause License
// Copyright (c) 2025, kcenon
// See the LICENSE file in the project root for full license information.

/**
 * @file feature_flags.h
 * @brief Unified feature flags header for common_system
 *
 * This is the main entry point for all feature detection and integration
 * flags in the common_system library. Include this header to get access
 * to all KCENON_* feature macros.
 *
 * Header organization:
 * - feature_flags_core.h    : Preprocessor helpers, compiler/platform detection
 * - feature_detection.h     : C++ standard library feature detection
 * - feature_system_deps.h   : System module integration flags
 * - feature_flags.h         : This file - aggregates all and provides legacy aliases
 *
 * Usage:
 * @code
 * #include <kcenon/common/config/feature_flags.h>
 *
 * #if KCENON_HAS_SOURCE_LOCATION
 *     #include <source_location>
 *     using location_type = std::source_location;
 * #else
 *     using location_type = kcenon::common::source_location;
 * #endif
 *
 * #if KCENON_HAS_JTHREAD
 *     std::jthread worker([](std::stop_token st) { ... });
 * #else
 *     std::thread worker([]{ ... });
 * #endif
 * @endcode
 *
 * @see feature_flags_core.h for preprocessor helpers
 * @see feature_detection.h for C++ feature detection
 * @see feature_system_deps.h for system integration flags
 */

#pragma once

// Include all feature detection headers
#include "feature_flags_core.h"
#include "feature_detection.h"
#include "feature_system_deps.h"

//==============================================================================
// Legacy Aliases
//==============================================================================

/**
 * @brief Legacy alias definitions for backward compatibility
 *
 * These aliases map old macro names to the new KCENON_* convention.
 * They are only defined if:
 * 1. KCENON_ENABLE_LEGACY_ALIASES is enabled (default: 1)
 * 2. The legacy macro is not already explicitly defined
 *
 * This ensures that explicit user definitions (including explicit 0 values)
 * are respected and not overwritten.
 *
 * @note Legacy aliases are planned for deprecation in v0.4.0 and
 *       removal in v1.0.0. Migrate to KCENON_* macros.
 */
#if KCENON_ENABLE_LEGACY_ALIASES

//------------------------------------------------------------------------------
// source_location aliases
//------------------------------------------------------------------------------

// COMMON_HAS_SOURCE_LOCATION -> KCENON_HAS_SOURCE_LOCATION
#ifndef COMMON_HAS_SOURCE_LOCATION
    #define COMMON_HAS_SOURCE_LOCATION KCENON_HAS_SOURCE_LOCATION
#endif

// KCENON_HAS_STD_SOURCE_LOCATION -> KCENON_HAS_SOURCE_LOCATION
// (This was the original name in source_location.h)
#ifndef KCENON_HAS_STD_SOURCE_LOCATION
    #define KCENON_HAS_STD_SOURCE_LOCATION KCENON_HAS_SOURCE_LOCATION
#endif

//------------------------------------------------------------------------------
// System integration aliases
//------------------------------------------------------------------------------

// USE_THREAD_SYSTEM -> KCENON_WITH_THREAD_SYSTEM
#ifndef USE_THREAD_SYSTEM
    #if KCENON_WITH_THREAD_SYSTEM
        #define USE_THREAD_SYSTEM 1
    #endif
#endif

// BUILD_WITH_THREAD_SYSTEM -> KCENON_WITH_THREAD_SYSTEM
#ifndef BUILD_WITH_THREAD_SYSTEM
    #if KCENON_WITH_THREAD_SYSTEM
        #define BUILD_WITH_THREAD_SYSTEM 1
    #endif
#endif

// USE_LOGGER_SYSTEM -> KCENON_WITH_LOGGER_SYSTEM
#ifndef USE_LOGGER_SYSTEM
    #if KCENON_WITH_LOGGER_SYSTEM
        #define USE_LOGGER_SYSTEM 1
    #endif
#endif

// BUILD_WITH_LOGGER -> KCENON_WITH_LOGGER_SYSTEM
#ifndef BUILD_WITH_LOGGER
    #if KCENON_WITH_LOGGER_SYSTEM
        #define BUILD_WITH_LOGGER 1
    #endif
#endif

// USE_MONITORING_SYSTEM -> KCENON_WITH_MONITORING_SYSTEM
#ifndef USE_MONITORING_SYSTEM
    #if KCENON_WITH_MONITORING_SYSTEM
        #define USE_MONITORING_SYSTEM 1
    #endif
#endif

// BUILD_WITH_MONITORING -> KCENON_WITH_MONITORING_SYSTEM
#ifndef BUILD_WITH_MONITORING
    #if KCENON_WITH_MONITORING_SYSTEM
        #define BUILD_WITH_MONITORING 1
    #endif
#endif

// USE_CONTAINER_SYSTEM -> KCENON_WITH_CONTAINER_SYSTEM
#ifndef USE_CONTAINER_SYSTEM
    #if KCENON_WITH_CONTAINER_SYSTEM
        #define USE_CONTAINER_SYSTEM 1
    #endif
#endif

// BUILD_WITH_CONTAINER -> KCENON_WITH_CONTAINER_SYSTEM
#ifndef BUILD_WITH_CONTAINER
    #if KCENON_WITH_CONTAINER_SYSTEM
        #define BUILD_WITH_CONTAINER 1
    #endif
#endif

//------------------------------------------------------------------------------
// WITH_*_SYSTEM aliases (CMake-generated)
//------------------------------------------------------------------------------

// These may be set by CMake's configure_module_integration macro
#ifndef WITH_THREAD_SYSTEM
    #if KCENON_WITH_THREAD_SYSTEM
        #define WITH_THREAD_SYSTEM 1
    #endif
#endif

#ifndef WITH_LOGGER_SYSTEM
    #if KCENON_WITH_LOGGER_SYSTEM
        #define WITH_LOGGER_SYSTEM 1
    #endif
#endif

#ifndef WITH_MONITORING_SYSTEM
    #if KCENON_WITH_MONITORING_SYSTEM
        #define WITH_MONITORING_SYSTEM 1
    #endif
#endif

#ifndef WITH_CONTAINER_SYSTEM
    #if KCENON_WITH_CONTAINER_SYSTEM
        #define WITH_CONTAINER_SYSTEM 1
    #endif
#endif

#endif // KCENON_ENABLE_LEGACY_ALIASES

//==============================================================================
// Feature Summary (for debugging)
//==============================================================================

/**
 * @brief Print feature detection summary at compile time
 *
 * Enable by defining KCENON_PRINT_FEATURE_SUMMARY before including this header.
 * Useful for debugging feature detection issues.
 */
#ifdef KCENON_PRINT_FEATURE_SUMMARY

#pragma message("=== KCENON Feature Detection Summary ===")

// Compiler
#if KCENON_COMPILER_MSVC
    #pragma message("Compiler: MSVC")
#elif KCENON_COMPILER_CLANG
    #pragma message("Compiler: Clang")
#elif KCENON_COMPILER_GCC
    #pragma message("Compiler: GCC")
#else
    #pragma message("Compiler: Unknown")
#endif

// C++ Standard
#if KCENON_HAS_CPP23
    #pragma message("C++ Standard: C++23")
#elif KCENON_HAS_CPP20
    #pragma message("C++ Standard: C++20")
#elif KCENON_HAS_CPP17
    #pragma message("C++ Standard: C++17")
#else
    #pragma message("C++ Standard: Pre-C++17")
#endif

// Features
#if KCENON_HAS_SOURCE_LOCATION
    #pragma message("source_location: Available")
#else
    #pragma message("source_location: Unavailable (using fallback)")
#endif

#if KCENON_HAS_JTHREAD
    #pragma message("jthread: Available")
#else
    #pragma message("jthread: Unavailable")
#endif

#if KCENON_HAS_FORMAT
    #pragma message("std::format: Available")
#else
    #pragma message("std::format: Unavailable")
#endif

#if KCENON_HAS_CONCEPTS
    #pragma message("Concepts: Available")
#else
    #pragma message("Concepts: Unavailable")
#endif

#if KCENON_HAS_RANGES
    #pragma message("Ranges: Available")
#else
    #pragma message("Ranges: Unavailable")
#endif

#pragma message("=========================================")

#endif // KCENON_PRINT_FEATURE_SUMMARY

//==============================================================================
// Static Assertions for Minimum Requirements
//==============================================================================

// Ensure C++17 minimum
static_assert(KCENON_HAS_CPP17,
    "common_system requires C++17 or later. "
    "Please compile with -std=c++17 or higher.");
