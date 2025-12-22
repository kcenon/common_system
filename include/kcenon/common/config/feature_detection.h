// BSD 3-Clause License
// Copyright (c) 2025, kcenon
// See the LICENSE file in the project root for full license information.

/**
 * @file feature_detection.h
 * @brief Compiler and standard library feature detection
 *
 * This file detects C++17/20/23 standard library features and provides
 * unified KCENON_HAS_* macros for conditional compilation. All detection
 * follows the official feature test macros from the C++ standard.
 *
 * Detected features:
 * - std::source_location (C++20)
 * - std::jthread / std::stop_token (C++20)
 * - std::format (C++20)
 * - std::span (C++20)
 * - std::ranges (C++20)
 * - std::expected (C++23)
 * - std::stacktrace (C++23)
 *
 * @see https://en.cppreference.com/w/cpp/feature_test for feature test macros
 */

#pragma once

#include "feature_flags_core.h"

//==============================================================================
// Standard Library Version Detection
//==============================================================================

// Include version header if available (C++20)
#if KCENON_HAS_INCLUDE(<version>)
    #include <version>
#endif

//==============================================================================
// source_location Detection (C++20)
//==============================================================================

/**
 * @brief Detect std::source_location availability
 *
 * std::source_location provides compile-time capture of source code
 * location information (file, line, function).
 *
 * Detection criteria:
 * 1. C++20 or later
 * 2. <source_location> header available
 * 3. __cpp_lib_source_location feature macro >= 201907L
 */
#ifndef KCENON_HAS_SOURCE_LOCATION
    #if KCENON_HAS_CPP20 && KCENON_HAS_INCLUDE(<source_location>)
        #if defined(__cpp_lib_source_location) && __cpp_lib_source_location >= 201907L
            #define KCENON_HAS_SOURCE_LOCATION 1
        #endif
    #endif
    #ifndef KCENON_HAS_SOURCE_LOCATION
        #define KCENON_HAS_SOURCE_LOCATION 0
    #endif
#endif

//==============================================================================
// jthread / stop_token Detection (C++20)
//==============================================================================

/**
 * @brief Detect std::jthread and std::stop_token availability
 *
 * std::jthread is a cooperative thread with automatic joining and
 * stop_token support for cooperative cancellation.
 *
 * Detection criteria:
 * 1. C++20 or later
 * 2. <thread> or <stop_token> header available
 * 3. __cpp_lib_jthread >= 201911L OR __cpp_lib_stop_token >= 201907L
 *
 * @note Some implementations may have stop_token without jthread.
 *       We check both for maximum compatibility.
 */
#ifndef KCENON_HAS_JTHREAD
    #if KCENON_HAS_CPP20
        // Check jthread feature macro
        #if defined(__cpp_lib_jthread) && __cpp_lib_jthread >= 201911L
            #define KCENON_HAS_JTHREAD 1
        // Fallback: check stop_token which jthread depends on
        #elif defined(__cpp_lib_stop_token) && __cpp_lib_stop_token >= 201907L
            #define KCENON_HAS_JTHREAD 1
        #endif
    #endif
    #ifndef KCENON_HAS_JTHREAD
        #define KCENON_HAS_JTHREAD 0
    #endif
#endif

/**
 * @brief Detect std::stop_token availability separately
 */
#ifndef KCENON_HAS_STOP_TOKEN
    #if KCENON_HAS_CPP20
        #if defined(__cpp_lib_stop_token) && __cpp_lib_stop_token >= 201907L
            #define KCENON_HAS_STOP_TOKEN 1
        #endif
    #endif
    #ifndef KCENON_HAS_STOP_TOKEN
        #define KCENON_HAS_STOP_TOKEN 0
    #endif
#endif

//==============================================================================
// std::format Detection (C++20)
//==============================================================================

/**
 * @brief Detect std::format availability
 *
 * std::format provides type-safe text formatting similar to Python's
 * format strings and fmt library.
 */
#ifndef KCENON_HAS_FORMAT
    #if KCENON_HAS_CPP20 && KCENON_HAS_INCLUDE(<format>)
        #if defined(__cpp_lib_format) && __cpp_lib_format >= 201907L
            #define KCENON_HAS_FORMAT 1
        #endif
    #endif
    #ifndef KCENON_HAS_FORMAT
        #define KCENON_HAS_FORMAT 0
    #endif
#endif

//==============================================================================
// std::span Detection (C++20)
//==============================================================================

/**
 * @brief Detect std::span availability
 *
 * std::span provides a non-owning view over a contiguous sequence of objects.
 */
#ifndef KCENON_HAS_SPAN
    #if KCENON_HAS_CPP20 && KCENON_HAS_INCLUDE(<span>)
        #if defined(__cpp_lib_span) && __cpp_lib_span >= 202002L
            #define KCENON_HAS_SPAN 1
        #endif
    #endif
    #ifndef KCENON_HAS_SPAN
        #define KCENON_HAS_SPAN 0
    #endif
#endif

//==============================================================================
// std::ranges Detection (C++20)
//==============================================================================

/**
 * @brief Detect std::ranges availability
 *
 * C++20 Ranges library provides composable range adaptors and views.
 */
#ifndef KCENON_HAS_RANGES
    #if KCENON_HAS_CPP20 && KCENON_HAS_INCLUDE(<ranges>)
        #if defined(__cpp_lib_ranges) && __cpp_lib_ranges >= 201911L
            #define KCENON_HAS_RANGES 1
        #endif
    #endif
    #ifndef KCENON_HAS_RANGES
        #define KCENON_HAS_RANGES 0
    #endif
#endif

//==============================================================================
// std::expected Detection (C++23)
//==============================================================================

/**
 * @brief Detect std::expected availability
 *
 * std::expected provides a value-or-error wrapper similar to Result types
 * in Rust and other languages.
 */
#ifndef KCENON_HAS_EXPECTED
    #if KCENON_HAS_CPP23 && KCENON_HAS_INCLUDE(<expected>)
        #if defined(__cpp_lib_expected) && __cpp_lib_expected >= 202202L
            #define KCENON_HAS_EXPECTED 1
        #endif
    #endif
    #ifndef KCENON_HAS_EXPECTED
        #define KCENON_HAS_EXPECTED 0
    #endif
#endif

//==============================================================================
// std::stacktrace Detection (C++23)
//==============================================================================

/**
 * @brief Detect std::stacktrace availability
 *
 * std::stacktrace provides portable stack trace information.
 */
#ifndef KCENON_HAS_STACKTRACE
    #if KCENON_HAS_CPP23 && KCENON_HAS_INCLUDE(<stacktrace>)
        #if defined(__cpp_lib_stacktrace) && __cpp_lib_stacktrace >= 202011L
            #define KCENON_HAS_STACKTRACE 1
        #endif
    #endif
    #ifndef KCENON_HAS_STACKTRACE
        #define KCENON_HAS_STACKTRACE 0
    #endif
#endif

//==============================================================================
// Concepts Detection (C++20)
//==============================================================================

/**
 * @brief Detect C++20 concepts availability
 */
#ifndef KCENON_HAS_CONCEPTS
    #if KCENON_HAS_CPP20 && KCENON_HAS_INCLUDE(<concepts>)
        #if defined(__cpp_concepts) && __cpp_concepts >= 201907L
            #define KCENON_HAS_CONCEPTS 1
        #endif
    #endif
    #ifndef KCENON_HAS_CONCEPTS
        #define KCENON_HAS_CONCEPTS 0
    #endif
#endif

//==============================================================================
// Coroutines Detection (C++20)
//==============================================================================

/**
 * @brief Detect C++20 coroutines availability
 */
#ifndef KCENON_HAS_COROUTINES
    #if KCENON_HAS_CPP20 && KCENON_HAS_INCLUDE(<coroutine>)
        #if defined(__cpp_impl_coroutine) && __cpp_impl_coroutine >= 201902L
            #define KCENON_HAS_COROUTINES 1
        #endif
    #endif
    #ifndef KCENON_HAS_COROUTINES
        #define KCENON_HAS_COROUTINES 0
    #endif
#endif

//==============================================================================
// Three-way Comparison Detection (C++20)
//==============================================================================

/**
 * @brief Detect C++20 three-way comparison (spaceship operator)
 */
#ifndef KCENON_HAS_THREE_WAY_COMPARISON
    #if KCENON_HAS_CPP20 && KCENON_HAS_INCLUDE(<compare>)
        #if defined(__cpp_impl_three_way_comparison) && \
            __cpp_impl_three_way_comparison >= 201907L
            #define KCENON_HAS_THREE_WAY_COMPARISON 1
        #endif
    #endif
    #ifndef KCENON_HAS_THREE_WAY_COMPARISON
        #define KCENON_HAS_THREE_WAY_COMPARISON 0
    #endif
#endif

//==============================================================================
// std::optional enhancements Detection
//==============================================================================

/**
 * @brief Detect std::optional monadic operations (C++23)
 */
#ifndef KCENON_HAS_OPTIONAL_MONADIC
    #if KCENON_HAS_CPP23
        #if defined(__cpp_lib_optional) && __cpp_lib_optional >= 202110L
            #define KCENON_HAS_OPTIONAL_MONADIC 1
        #endif
    #endif
    #ifndef KCENON_HAS_OPTIONAL_MONADIC
        #define KCENON_HAS_OPTIONAL_MONADIC 0
    #endif
#endif

//==============================================================================
// Constexpr Enhancements Detection
//==============================================================================

/**
 * @brief Detect constexpr std::vector (C++20)
 */
#ifndef KCENON_HAS_CONSTEXPR_VECTOR
    #if KCENON_HAS_CPP20
        #if defined(__cpp_lib_constexpr_vector) && \
            __cpp_lib_constexpr_vector >= 201907L
            #define KCENON_HAS_CONSTEXPR_VECTOR 1
        #endif
    #endif
    #ifndef KCENON_HAS_CONSTEXPR_VECTOR
        #define KCENON_HAS_CONSTEXPR_VECTOR 0
    #endif
#endif

/**
 * @brief Detect constexpr std::string (C++20)
 */
#ifndef KCENON_HAS_CONSTEXPR_STRING
    #if KCENON_HAS_CPP20
        #if defined(__cpp_lib_constexpr_string) && \
            __cpp_lib_constexpr_string >= 201907L
            #define KCENON_HAS_CONSTEXPR_STRING 1
        #endif
    #endif
    #ifndef KCENON_HAS_CONSTEXPR_STRING
        #define KCENON_HAS_CONSTEXPR_STRING 0
    #endif
#endif
