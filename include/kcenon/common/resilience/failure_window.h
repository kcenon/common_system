// BSD 3-Clause License
// Copyright (c) 2025, kcenon
// See the LICENSE file in the project root for full license information.

/**
 * @file failure_window.h
 * @brief Sliding time window for tracking failures.
 *
 * Maintains a time-based window of failure timestamps,
 * automatically expiring old failures outside the window.
 */

#pragma once

#include <algorithm>
#include <chrono>
#include <deque>
#include <mutex>

namespace kcenon::common::resilience {

/**
 * @class failure_window
 * @brief Thread-safe sliding window for failure tracking.
 *
 * Maintains a deque of failure timestamps within a configured window duration.
 * Automatically removes expired failures when queried.
 *
 * Thread Safety:
 * - All public methods are thread-safe via internal mutex.
 * - Safe for concurrent access from multiple threads.
 */
class failure_window {
public:
    using clock_type = std::chrono::steady_clock;
    using time_point = clock_type::time_point;
    using duration = std::chrono::milliseconds;

    /**
     * @brief Construct a failure window with specified duration.
     * @param window_duration Time window for tracking failures
     */
    explicit failure_window(duration window_duration)
        : window_duration_(window_duration)
    {
    }

    /**
     * @brief Record a new failure at current time.
     */
    auto record_failure() -> void
    {
        std::lock_guard<std::mutex> lock(mutex_);
        failures_.push_back(clock_type::now());
    }

    /**
     * @brief Get current failure count within window.
     * Removes expired failures before counting.
     * @return Number of failures within the time window
     */
    [[nodiscard]] auto get_failure_count() -> std::size_t
    {
        std::lock_guard<std::mutex> lock(mutex_);
        cleanup_expired_failures();
        return failures_.size();
    }

    /**
     * @brief Clear all recorded failures.
     */
    auto reset() -> void
    {
        std::lock_guard<std::mutex> lock(mutex_);
        failures_.clear();
    }

    /**
     * @brief Check if window is empty (no recent failures).
     * @return true if no failures within window, false otherwise
     */
    [[nodiscard]] auto is_empty() -> bool
    {
        std::lock_guard<std::mutex> lock(mutex_);
        cleanup_expired_failures();
        return failures_.empty();
    }

private:
    /**
     * @brief Remove failures outside the time window.
     * Must be called with mutex locked.
     */
    auto cleanup_expired_failures() -> void
    {
        const auto now = clock_type::now();
        const auto cutoff = now - window_duration_;

        // Remove all failures older than cutoff
        failures_.erase(
            std::remove_if(failures_.begin(), failures_.end(),
                [cutoff](const time_point& t) { return t < cutoff; }),
            failures_.end());
    }

    duration window_duration_;
    std::deque<time_point> failures_;
    std::mutex mutex_;
};

} // namespace kcenon::common::resilience
