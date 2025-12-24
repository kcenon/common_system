// BSD 3-Clause License
// Copyright (c) 2025, kcenon
// See the LICENSE file in the project root for full license information.

/**
 * @file feature_system_deps.h
 * @brief System module integration flags
 *
 * This file provides KCENON_* macros for detecting and controlling
 * integration with other system modules (thread_system, logger_system, etc.).
 * These flags are typically set via CMake and propagated as compile definitions.
 *
 * Integration flags:
 * - KCENON_WITH_THREAD_SYSTEM
 * - KCENON_WITH_LOGGER_SYSTEM
 * - KCENON_WITH_MONITORING_SYSTEM
 * - KCENON_WITH_CONTAINER_SYSTEM
 * - KCENON_WITH_NETWORK_SYSTEM
 * - KCENON_WITH_DATABASE_SYSTEM
 * - KCENON_WITH_MESSAGING_SYSTEM
 *
 * @note These macros are designed to be set via CMake's
 *       target_compile_definitions() or features.cmake configuration.
 */

#pragma once

#include "feature_flags_core.h"

//==============================================================================
// Common System Availability
//==============================================================================

/**
 * @brief Indicates that common_system types are available
 *
 * When feature_flags.h is included, this macro is automatically defined to 1.
 * Downstream projects (e.g., network_system) can use this to detect whether
 * common_system types like kcenon::common::Result<T> are available.
 *
 * This prevents ABI incompatibility issues where:
 * - Project A includes common_system headers and expects kcenon::common::Result<T>
 * - Project B is built without the flag and uses a local fallback type
 * - Linking causes symbol mismatches or heap corruption
 *
 * @note This flag is always set to 1 when feature_flags.h is included.
 *       It is not configurable since including this header implies
 *       common_system is available.
 *
 * @code
 * #ifdef KCENON_WITH_COMMON_SYSTEM
 *     using result_type = kcenon::common::Result<T>;
 * #else
 *     using result_type = local::Result<T>;  // fallback
 * #endif
 * @endcode
 */
#ifndef KCENON_WITH_COMMON_SYSTEM
    #define KCENON_WITH_COMMON_SYSTEM 1
#endif

//==============================================================================
// Thread System Integration
//==============================================================================

/**
 * @brief Enable integration with thread_system module
 *
 * When enabled, features like jthread wrappers, thread pools,
 * and cooperative cancellation are available.
 */
#ifndef KCENON_WITH_THREAD_SYSTEM
    #if defined(ENABLE_THREAD_INTEGRATION) || defined(WITH_THREAD_SYSTEM)
        #define KCENON_WITH_THREAD_SYSTEM 1
    #else
        #define KCENON_WITH_THREAD_SYSTEM 0
    #endif
#endif

//==============================================================================
// Logger System Integration
//==============================================================================

/**
 * @brief Enable integration with logger_system module
 *
 * When enabled, structured logging, log levels, and log sinks are available.
 */
#ifndef KCENON_WITH_LOGGER_SYSTEM
    #if defined(ENABLE_LOGGER_INTEGRATION) || defined(WITH_LOGGER_SYSTEM)
        #define KCENON_WITH_LOGGER_SYSTEM 1
    #else
        #define KCENON_WITH_LOGGER_SYSTEM 0
    #endif
#endif

//==============================================================================
// Monitoring System Integration
//==============================================================================

/**
 * @brief Enable integration with monitoring_system module
 *
 * When enabled, metrics collection, health checks, and alerting are available.
 */
#ifndef KCENON_WITH_MONITORING_SYSTEM
    #if defined(ENABLE_MONITORING_INTEGRATION) || defined(WITH_MONITORING_SYSTEM)
        #define KCENON_WITH_MONITORING_SYSTEM 1
    #else
        #define KCENON_WITH_MONITORING_SYSTEM 0
    #endif
#endif

//==============================================================================
// Container System Integration
//==============================================================================

/**
 * @brief Enable integration with container_system module
 *
 * When enabled, specialized containers like concurrent queues are available.
 */
#ifndef KCENON_WITH_CONTAINER_SYSTEM
    #if defined(ENABLE_CONTAINER_INTEGRATION) || defined(WITH_CONTAINER_SYSTEM)
        #define KCENON_WITH_CONTAINER_SYSTEM 1
    #else
        #define KCENON_WITH_CONTAINER_SYSTEM 0
    #endif
#endif

//==============================================================================
// Network System Integration
//==============================================================================

/**
 * @brief Enable integration with network_system module
 *
 * When enabled, network communication features are available.
 */
#ifndef KCENON_WITH_NETWORK_SYSTEM
    #if defined(ENABLE_NETWORK_INTEGRATION) || defined(WITH_NETWORK_SYSTEM)
        #define KCENON_WITH_NETWORK_SYSTEM 1
    #else
        #define KCENON_WITH_NETWORK_SYSTEM 0
    #endif
#endif

/**
 * @brief Enable SSL/TLS support in network_system
 */
#ifndef KCENON_NETWORK_SSL
    #if defined(ENABLE_NETWORK_SSL)
        #define KCENON_NETWORK_SSL 1
    #else
        #define KCENON_NETWORK_SSL 0
    #endif
#endif

/**
 * @brief Enable compression support in network_system
 */
#ifndef KCENON_NETWORK_COMPRESSION
    #if defined(ENABLE_NETWORK_COMPRESSION)
        #define KCENON_NETWORK_COMPRESSION 1
    #else
        #define KCENON_NETWORK_COMPRESSION 0
    #endif
#endif

//==============================================================================
// Database System Integration
//==============================================================================

/**
 * @brief Enable integration with database_system module
 */
#ifndef KCENON_WITH_DATABASE_SYSTEM
    #if defined(ENABLE_DATABASE_INTEGRATION) || defined(WITH_DATABASE_SYSTEM)
        #define KCENON_WITH_DATABASE_SYSTEM 1
    #else
        #define KCENON_WITH_DATABASE_SYSTEM 0
    #endif
#endif

/**
 * @brief Database backend support flags
 */
#ifndef KCENON_DATABASE_MYSQL
    #if defined(ENABLE_DATABASE_MYSQL)
        #define KCENON_DATABASE_MYSQL 1
    #else
        #define KCENON_DATABASE_MYSQL 0
    #endif
#endif

#ifndef KCENON_DATABASE_POSTGRESQL
    #if defined(ENABLE_DATABASE_POSTGRESQL)
        #define KCENON_DATABASE_POSTGRESQL 1
    #else
        #define KCENON_DATABASE_POSTGRESQL 0
    #endif
#endif

#ifndef KCENON_DATABASE_SQLITE
    #if defined(ENABLE_DATABASE_SQLITE)
        #define KCENON_DATABASE_SQLITE 1
    #else
        #define KCENON_DATABASE_SQLITE 0
    #endif
#endif

#ifndef KCENON_DATABASE_MONGODB
    #if defined(ENABLE_DATABASE_MONGODB)
        #define KCENON_DATABASE_MONGODB 1
    #else
        #define KCENON_DATABASE_MONGODB 0
    #endif
#endif

#ifndef KCENON_DATABASE_REDIS
    #if defined(ENABLE_DATABASE_REDIS)
        #define KCENON_DATABASE_REDIS 1
    #else
        #define KCENON_DATABASE_REDIS 0
    #endif
#endif

//==============================================================================
// Messaging System Integration
//==============================================================================

/**
 * @brief Enable integration with messaging_system module
 *
 * When enabled, message queuing and pub/sub features are available.
 */
#ifndef KCENON_WITH_MESSAGING_SYSTEM
    #if defined(ENABLE_MESSAGING_INTEGRATION) || defined(WITH_MESSAGING_SYSTEM)
        #define KCENON_WITH_MESSAGING_SYSTEM 1
    #else
        #define KCENON_WITH_MESSAGING_SYSTEM 0
    #endif
#endif

//==============================================================================
// Build Configuration Flags
//==============================================================================

/**
 * @brief Enable unit tests
 */
#ifndef KCENON_ENABLE_TESTING
    #if defined(ENABLE_TESTING)
        #define KCENON_ENABLE_TESTING 1
    #else
        #define KCENON_ENABLE_TESTING 0
    #endif
#endif

/**
 * @brief Enable benchmarks
 */
#ifndef KCENON_ENABLE_BENCHMARKS
    #if defined(ENABLE_BENCHMARKS)
        #define KCENON_ENABLE_BENCHMARKS 1
    #else
        #define KCENON_ENABLE_BENCHMARKS 0
    #endif
#endif

/**
 * @brief Enable example applications
 */
#ifndef KCENON_ENABLE_EXAMPLES
    #if defined(ENABLE_EXAMPLES)
        #define KCENON_ENABLE_EXAMPLES 1
    #else
        #define KCENON_ENABLE_EXAMPLES 0
    #endif
#endif

//==============================================================================
// Sanitizer Flags
//==============================================================================

/**
 * @brief Address Sanitizer enabled
 */
#ifndef KCENON_ENABLE_ASAN
    #if defined(ENABLE_ASAN) || defined(__SANITIZE_ADDRESS__)
        #define KCENON_ENABLE_ASAN 1
    #else
        #define KCENON_ENABLE_ASAN 0
    #endif
#endif

/**
 * @brief Thread Sanitizer enabled
 */
#ifndef KCENON_ENABLE_TSAN
    #if defined(ENABLE_TSAN) || defined(__SANITIZE_THREAD__)
        #define KCENON_ENABLE_TSAN 1
    #else
        #define KCENON_ENABLE_TSAN 0
    #endif
#endif

/**
 * @brief Undefined Behavior Sanitizer enabled
 */
#ifndef KCENON_ENABLE_UBSAN
    #if defined(ENABLE_UBSAN)
        #define KCENON_ENABLE_UBSAN 1
    #else
        #define KCENON_ENABLE_UBSAN 0
    #endif
#endif
