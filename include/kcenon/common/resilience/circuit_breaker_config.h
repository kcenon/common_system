// BSD 3-Clause License
// Copyright (c) 2025, kcenon
// See the LICENSE file in the project root for full license information.

/**
 * @file circuit_breaker_config.h
 * @brief Configuration structure for circuit breaker behavior.
 *
 * Provides configurable thresholds and timeouts for circuit breaker operation.
 */

#pragma once

#include <chrono>
#include <cstddef>

namespace kcenon::common::resilience {

/**
 * @struct circuit_breaker_config
 * @brief Configuration parameters for circuit breaker.
 *
 * Thread Safety:
 * - This struct is intended to be read-only after construction.
 * - Safe to share across threads if not modified.
 */
struct circuit_breaker_config {
    /**
     * @brief Number of failures required to trip the circuit (CLOSED -> OPEN).
     * Default: 5 failures
     */
    std::size_t failure_threshold = 5;

    /**
     * @brief Number of successful requests required to close the circuit (HALF_OPEN -> CLOSED).
     * Default: 2 successes
     */
    std::size_t success_threshold = 2;

    /**
     * @brief Time window for tracking failures.
     * Failures older than this window are not counted.
     * Default: 60 seconds
     */
    std::chrono::milliseconds failure_window = std::chrono::seconds(60);

    /**
     * @brief Timeout before transitioning from OPEN to HALF_OPEN.
     * Default: 30 seconds
     */
    std::chrono::milliseconds timeout = std::chrono::seconds(30);

    /**
     * @brief Maximum number of requests allowed in HALF_OPEN state for testing.
     * Default: 3 requests
     */
    std::size_t half_open_max_requests = 3;
};

} // namespace kcenon::common::resilience
