// BSD 3-Clause License
// Copyright (c) 2025, kcenon
// See the LICENSE file in the project root for full license information.

/**
 * @file circuit_state.h
 * @brief Circuit breaker state machine states.
 *
 * Defines the three states of a circuit breaker:
 * - CLOSED: Normal operation, requests are allowed
 * - OPEN: Failure threshold exceeded, requests are blocked
 * - HALF_OPEN: Testing recovery, limited requests allowed
 */

#pragma once

#include <string>

namespace kcenon::common::resilience {

/**
 * @enum circuit_state
 * @brief Represents the current state of a circuit breaker.
 *
 * State Transitions:
 * CLOSED -> OPEN: When failure threshold is exceeded
 * OPEN -> HALF_OPEN: After timeout period expires
 * HALF_OPEN -> CLOSED: When success threshold is met
 * HALF_OPEN -> OPEN: When any failure occurs during testing
 */
enum class circuit_state {
    /**
     * @brief Normal operation state.
     * Requests are allowed and failures are tracked.
     */
    CLOSED,

    /**
     * @brief Failure state.
     * Requests are immediately rejected without execution.
     * Transitions to HALF_OPEN after timeout.
     */
    OPEN,

    /**
     * @brief Recovery testing state.
     * Limited requests are allowed to test if service has recovered.
     */
    HALF_OPEN
};

/**
 * @brief Convert circuit state to human-readable string.
 * @param state Circuit state to convert
 * @return String representation of the state
 */
inline auto to_string(circuit_state state) -> std::string
{
    switch (state) {
    case circuit_state::CLOSED:
        return "CLOSED";
    case circuit_state::OPEN:
        return "OPEN";
    case circuit_state::HALF_OPEN:
        return "HALF_OPEN";
    default:
        return "UNKNOWN";
    }
}

} // namespace kcenon::common::resilience
