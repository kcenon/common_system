// BSD 3-Clause License
// Copyright (c) 2025, kcenon
// See the LICENSE file in the project root for full license information.

/**
 * @file feature_flags_core.h
 * @brief Core KCENON_* macro definitions for feature detection
 *
 * This file provides the foundational macros used across all system modules
 * for detecting compiler and standard library features. It is designed to
 * be included first before any other feature detection headers.
 *
 * @note All macros follow the KCENON_ prefix convention for consistency.
 */

#pragma once

//==============================================================================
// Preprocessor Helpers
//==============================================================================

/**
 * @brief Helper macro to check if __has_include is available
 *
 * Some older compilers may not support __has_include. This provides a
 * safe fallback mechanism.
 */
#ifdef __has_include
    #define KCENON_HAS_INCLUDE(x) __has_include(x)
#else
    #define KCENON_HAS_INCLUDE(x) 0
#endif

/**
 * @brief Helper macro to check if __has_cpp_attribute is available
 */
#ifdef __has_cpp_attribute
    #define KCENON_HAS_CPP_ATTRIBUTE(x) __has_cpp_attribute(x)
#else
    #define KCENON_HAS_CPP_ATTRIBUTE(x) 0
#endif

/**
 * @brief Helper macro to check if __has_feature is available (Clang)
 */
#ifdef __has_feature
    #define KCENON_HAS_FEATURE(x) __has_feature(x)
#else
    #define KCENON_HAS_FEATURE(x) 0
#endif

/**
 * @brief Helper macro to check if __has_builtin is available
 */
#ifdef __has_builtin
    #define KCENON_HAS_BUILTIN(x) __has_builtin(x)
#else
    #define KCENON_HAS_BUILTIN(x) 0
#endif

//==============================================================================
// Compiler Detection
//==============================================================================

// Detect compilers
#if defined(_MSC_VER)
    #define KCENON_COMPILER_MSVC 1
    #define KCENON_COMPILER_VERSION _MSC_VER
#else
    #define KCENON_COMPILER_MSVC 0
#endif

#if defined(__clang__)
    #define KCENON_COMPILER_CLANG 1
    #define KCENON_CLANG_VERSION \
        (__clang_major__ * 10000 + __clang_minor__ * 100 + __clang_patchlevel__)
#else
    #define KCENON_COMPILER_CLANG 0
    #define KCENON_CLANG_VERSION 0
#endif

#if defined(__GNUC__) && !defined(__clang__)
    #define KCENON_COMPILER_GCC 1
    #define KCENON_GCC_VERSION \
        (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__)
#else
    #define KCENON_COMPILER_GCC 0
    #define KCENON_GCC_VERSION 0
#endif

//==============================================================================
// C++ Standard Detection
//==============================================================================

#if defined(_MSVC_LANG)
    #define KCENON_CPLUSPLUS _MSVC_LANG
#else
    #define KCENON_CPLUSPLUS __cplusplus
#endif

#define KCENON_HAS_CPP17 (KCENON_CPLUSPLUS >= 201703L)
#define KCENON_HAS_CPP20 (KCENON_CPLUSPLUS >= 202002L)
#define KCENON_HAS_CPP23 (KCENON_CPLUSPLUS >= 202302L)

//==============================================================================
// Platform Detection
//==============================================================================

#if defined(_WIN32) || defined(_WIN64)
    #define KCENON_PLATFORM_WINDOWS 1
#else
    #define KCENON_PLATFORM_WINDOWS 0
#endif

#if defined(__linux__)
    #define KCENON_PLATFORM_LINUX 1
#else
    #define KCENON_PLATFORM_LINUX 0
#endif

#if defined(__APPLE__) && defined(__MACH__)
    #define KCENON_PLATFORM_MACOS 1
#else
    #define KCENON_PLATFORM_MACOS 0
#endif

#define KCENON_PLATFORM_UNIX \
    (KCENON_PLATFORM_LINUX || KCENON_PLATFORM_MACOS)

//==============================================================================
// Legacy Alias Configuration
//==============================================================================

/**
 * @brief Enable legacy macro aliases for backward compatibility
 *
 * When defined and set to 1, legacy aliases (COMMON_*, USE_*, etc.) will
 * be available alongside the new KCENON_* macros. This provides a
 * migration path for existing code.
 *
 * @note Legacy aliases will not override explicit 0 values to ensure
 *       intentional disabling is respected.
 */
#ifndef KCENON_ENABLE_LEGACY_ALIASES
    #define KCENON_ENABLE_LEGACY_ALIASES 1
#endif

/**
 * @brief Helper macro for defining legacy aliases
 *
 * Only defines the legacy alias if:
 * 1. Legacy aliases are enabled
 * 2. The legacy macro is not already defined
 */
#if KCENON_ENABLE_LEGACY_ALIASES
    #define KCENON_DEFINE_LEGACY_ALIAS(legacy_name, new_name) \
        do { } while(0)
    // Legacy alias definitions are handled in feature_flags.h
    // after all KCENON_* macros are established
#endif
