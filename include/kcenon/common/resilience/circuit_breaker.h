// BSD 3-Clause License
// Copyright (c) 2025, kcenon
// See the LICENSE file in the project root for full license information.

/**
 * @file circuit_breaker.h
 * @brief Circuit breaker pattern for fault tolerance and resilience.
 *
 * Implements the Circuit Breaker pattern to prevent cascading failures
 * in distributed systems by temporarily blocking requests to failing services.
 *
 * State Machine:
 * - CLOSED: Normal operation, requests pass through
 * - OPEN: Failure threshold exceeded, requests immediately fail
 * - HALF_OPEN: Testing recovery, limited requests allowed
 *
 * This implementation consolidates circuit_breaker from thread_system,
 * monitoring_system, and network_system into a single, reusable component.
 */

#pragma once

#include "circuit_breaker_config.h"
#include "circuit_state.h"
#include "failure_window.h"
#include "../interfaces/stats.h"

#include <atomic>
#include <chrono>
#include <exception>
#include <memory>
#include <mutex>

namespace kcenon::common::resilience {

/**
 * @class circuit_breaker
 * @brief Thread-safe circuit breaker for fault tolerance.
 *
 * Usage Example:
 * @code
 * circuit_breaker_config config{
 *     .failure_threshold = 5,
 *     .timeout = std::chrono::seconds(30)
 * };
 * circuit_breaker breaker(config);
 *
 * // Check before making request
 * if (!breaker.allow_request()) {
 *     // Circuit is open, handle gracefully
 *     return error("Service unavailable");
 * }
 *
 * // Use RAII guard for automatic recording
 * {
 *     auto guard = breaker.make_guard();
 *     // Make risky operation
 *     auto result = risky_operation();
 *     guard.record_success(); // Mark as success if no exception
 * } // Automatically records failure if exception thrown
 * @endcode
 *
 * Thread Safety:
 * - All public methods are thread-safe.
 * - Safe for concurrent access from multiple threads.
 * - State transitions are protected by internal synchronization.
 */
class circuit_breaker : public interfaces::IStats {
public:
    using clock_type = std::chrono::steady_clock;
    using time_point = clock_type::time_point;

    /**
     * @brief Construct circuit breaker with specified configuration.
     * @param config Configuration parameters (thresholds, timeouts)
     */
    explicit circuit_breaker(circuit_breaker_config config = {})
        : config_(std::move(config))
        , state_(circuit_state::CLOSED)
        , failure_window_(config_.failure_window)
        , consecutive_successes_(0)
        , half_open_requests_(0)
        , last_state_change_(clock_type::now())
    {
    }

    /**
     * @brief Check if request should be allowed through the circuit.
     * @return true if request is allowed, false if circuit is open
     */
    [[nodiscard]] auto allow_request() -> bool
    {
        std::lock_guard<std::mutex> lock(mutex_);

        switch (state_) {
        case circuit_state::CLOSED:
            return true;

        case circuit_state::OPEN:
            // Check if timeout has elapsed to attempt recovery
            if (should_attempt_reset()) {
                transition_to_half_open();
                // First request in HALF_OPEN counts toward the limit
                ++half_open_requests_;
                return true;
            }
            return false;

        case circuit_state::HALF_OPEN:
            // Allow limited requests for testing
            if (half_open_requests_ < config_.half_open_max_requests) {
                ++half_open_requests_;
                return true;
            }
            return false;
        }

        return false; // Unreachable, but satisfies compiler
    }

    /**
     * @brief Record a successful operation.
     * May trigger state transition from HALF_OPEN to CLOSED.
     */
    auto record_success() -> void
    {
        std::lock_guard<std::mutex> lock(mutex_);

        if (state_ == circuit_state::HALF_OPEN) {
            ++consecutive_successes_;
            if (consecutive_successes_ >= config_.success_threshold) {
                transition_to_closed();
            }
        }
    }

    /**
     * @brief Record a failed operation.
     * May trigger state transition to OPEN or back to OPEN from HALF_OPEN.
     * @param e Optional exception pointer for logging/metrics
     */
    auto record_failure(const std::exception* e = nullptr) -> void
    {
        (void)e; // Reserved for future metrics/logging integration

        std::lock_guard<std::mutex> lock(mutex_);

        failure_window_.record_failure();

        if (state_ == circuit_state::HALF_OPEN) {
            // Any failure in half-open immediately reopens circuit
            transition_to_open();
        } else if (state_ == circuit_state::CLOSED) {
            // Check if failure threshold exceeded
            if (failure_window_.get_failure_count() >= config_.failure_threshold) {
                transition_to_open();
            }
        }
    }

    /**
     * @brief Get current circuit state.
     * @return Current state (CLOSED, OPEN, or HALF_OPEN)
     */
    [[nodiscard]] auto get_state() const -> circuit_state
    {
        return state_.load(std::memory_order_acquire);
    }

    /**
     * @brief RAII guard for automatic success/failure recording.
     *
     * Automatically records failure if destroyed without explicit success call.
     * Helps prevent forgetting to record operation results.
     */
    class guard {
    public:
        explicit guard(circuit_breaker& breaker)
            : breaker_(breaker)
            , committed_(false)
        {
        }

        ~guard()
        {
            if (!committed_) {
                breaker_.record_failure();
            }
        }

        // Non-copyable, non-movable
        guard(const guard&) = delete;
        guard& operator=(const guard&) = delete;
        guard(guard&&) = delete;
        guard& operator=(guard&&) = delete;

        /**
         * @brief Explicitly mark operation as successful.
         * Prevents automatic failure recording on destruction.
         */
        auto record_success() -> void
        {
            committed_ = true;
            breaker_.record_success();
        }

    private:
        circuit_breaker& breaker_;
        bool committed_;
    };

    /**
     * @brief Create RAII guard for automatic recording.
     * @return Guard object that records failure unless success is called
     */
    [[nodiscard]] auto make_guard() -> guard { return guard(*this); }

    // IStats interface implementation

    /**
     * @brief Get current statistics as key-value pairs.
     * @return Map of metric names to values (state, counts, rates)
     */
    [[nodiscard]] auto get_stats() const -> std::unordered_map<std::string, interfaces::stats_value> override
    {
        std::lock_guard<std::mutex> lock(mutex_);

        const auto current_state = state_.load(std::memory_order_acquire);
        const auto failure_count = const_cast<failure_window&>(failure_window_).get_failure_count();

        return {
            {"current_state", to_string(current_state)},
            {"failure_count", static_cast<std::int64_t>(failure_count)},
            {"consecutive_successes", static_cast<std::int64_t>(consecutive_successes_)},
            {"half_open_requests", static_cast<std::int64_t>(half_open_requests_)},
            {"failure_threshold", static_cast<std::int64_t>(config_.failure_threshold)},
            {"is_open", current_state == circuit_state::OPEN}
        };
    }

    /**
     * @brief Get statistics as JSON string.
     * @return JSON representation of current statistics
     */
    [[nodiscard]] auto to_json() const -> std::string override
    {
        const auto snapshot = get_snapshot();
        return snapshot.to_json();
    }

    /**
     * @brief Get component name for identification.
     * @return Component name "circuit_breaker"
     */
    [[nodiscard]] auto name() const -> std::string_view override
    {
        return "circuit_breaker";
    }

private:
    /**
     * @brief Check if circuit should attempt reset to HALF_OPEN.
     * Must be called with mutex locked.
     */
    [[nodiscard]] auto should_attempt_reset() const -> bool
    {
        const auto now = clock_type::now();
        const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            now - last_state_change_);
        return elapsed >= config_.timeout;
    }

    /**
     * @brief Transition to CLOSED state.
     * Must be called with mutex locked.
     */
    auto transition_to_closed() -> void
    {
        state_.store(circuit_state::CLOSED, std::memory_order_release);
        failure_window_.reset();
        consecutive_successes_ = 0;
        half_open_requests_ = 0;
        last_state_change_ = clock_type::now();
    }

    /**
     * @brief Transition to OPEN state.
     * Must be called with mutex locked.
     */
    auto transition_to_open() -> void
    {
        state_.store(circuit_state::OPEN, std::memory_order_release);
        consecutive_successes_ = 0;
        half_open_requests_ = 0;
        last_state_change_ = clock_type::now();
    }

    /**
     * @brief Transition to HALF_OPEN state.
     * Must be called with mutex locked.
     */
    auto transition_to_half_open() -> void
    {
        state_.store(circuit_state::HALF_OPEN, std::memory_order_release);
        consecutive_successes_ = 0;
        half_open_requests_ = 0;
        last_state_change_ = clock_type::now();
    }

    circuit_breaker_config config_;
    std::atomic<circuit_state> state_;
    failure_window failure_window_;
    std::size_t consecutive_successes_;
    std::size_t half_open_requests_;
    time_point last_state_change_;
    mutable std::mutex mutex_;
};

} // namespace kcenon::common::resilience
