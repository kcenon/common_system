// BSD 3-Clause License
// Copyright (c) 2025, kcenon
// See the LICENSE file in the project root for full license information.

/**
 * @file composite_health_check.h
 * @brief Composite health check that aggregates multiple health checks.
 *
 * This header defines the composite_health_check class which combines
 * multiple health checks into a single aggregated result.
 */

#pragma once

#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "health_check.h"

namespace kcenon::common::interfaces {

/**
 * @class composite_health_check
 * @brief Aggregates multiple health checks into a single check
 *
 * This class implements the Composite pattern for health checks,
 * allowing multiple checks to be grouped and executed together.
 * The overall health status is determined by the worst status
 * among all child checks.
 *
 * Example usage:
 * @code
 * auto composite = std::make_shared<composite_health_check>("system_health");
 * composite->add_check(database_check);
 * composite->add_check(cache_check);
 * composite->add_check(queue_check);
 *
 * auto result = composite->check();  // Runs all checks
 * @endcode
 */
class composite_health_check : public health_check {
public:
    /**
     * @brief Construct a composite health check
     * @param name Name for this composite check
     * @param type Health check type (default: custom)
     * @param critical Whether this composite is critical (default: true)
     */
    explicit composite_health_check(
        std::string name,
        health_check_type type = health_check_type::custom,
        bool critical = true)
        : name_(std::move(name)), type_(type), critical_(critical) {}

    [[nodiscard]] std::string get_name() const override { return name_; }

    [[nodiscard]] health_check_type get_type() const override { return type_; }

    [[nodiscard]] bool is_critical() const override { return critical_; }

    /**
     * @brief Execute all child health checks and aggregate results
     * @return Aggregated health check result
     *
     * The overall status is determined as follows:
     * - UNHEALTHY if any child is UNHEALTHY
     * - DEGRADED if any child is DEGRADED (and none are UNHEALTHY)
     * - UNKNOWN if any child is UNKNOWN (and none are worse)
     * - HEALTHY only if all children are HEALTHY
     */
    health_check_result check() override {
        std::lock_guard<std::mutex> lock(mutex_);

        health_check_result result;
        result.status = health_status::healthy;
        auto start_time = std::chrono::steady_clock::now();

        std::vector<std::string> messages;
        std::size_t healthy_count = 0;
        std::size_t degraded_count = 0;
        std::size_t unhealthy_count = 0;

        for (const auto& check : checks_) {
            auto child_result = check->check();

            switch (child_result.status) {
                case health_status::unhealthy:
                    ++unhealthy_count;
                    result.status = health_status::unhealthy;
                    messages.push_back(check->get_name() + ": " + child_result.message);
                    break;
                case health_status::degraded:
                    ++degraded_count;
                    if (result.status != health_status::unhealthy) {
                        result.status = health_status::degraded;
                    }
                    messages.push_back(check->get_name() + ": " + child_result.message);
                    break;
                case health_status::unknown:
                    if (result.status == health_status::healthy) {
                        result.status = health_status::unknown;
                    }
                    messages.push_back(check->get_name() + ": " + child_result.message);
                    break;
                case health_status::healthy:
                    ++healthy_count;
                    break;
            }
        }

        auto end_time = std::chrono::steady_clock::now();
        result.check_duration = std::chrono::duration_cast<std::chrono::milliseconds>(
            end_time - start_time);

        if (messages.empty()) {
            result.message = "All " + std::to_string(healthy_count) + " checks passed";
        } else {
            result.message = std::to_string(unhealthy_count) + " unhealthy, " +
                             std::to_string(degraded_count) + " degraded";
            for (const auto& msg : messages) {
                result.message += "; " + msg;
            }
        }

        result.metadata["total_checks"] = std::to_string(checks_.size());
        result.metadata["healthy_count"] = std::to_string(healthy_count);
        result.metadata["degraded_count"] = std::to_string(degraded_count);
        result.metadata["unhealthy_count"] = std::to_string(unhealthy_count);

        return result;
    }

    /**
     * @brief Add a child health check
     * @param check Health check to add
     */
    void add_check(std::shared_ptr<health_check> check) {
        if (!check) {
            return;
        }
        std::lock_guard<std::mutex> lock(mutex_);
        checks_.push_back(std::move(check));
    }

    /**
     * @brief Remove a child health check by name
     * @param name Name of the check to remove
     * @return true if check was found and removed
     */
    bool remove_check(const std::string& name) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = std::find_if(checks_.begin(), checks_.end(),
                               [&name](const std::shared_ptr<health_check>& check) {
                                   return check->get_name() == name;
                               });
        if (it != checks_.end()) {
            checks_.erase(it);
            return true;
        }
        return false;
    }

    /**
     * @brief Get the number of child checks
     * @return Number of child checks
     */
    [[nodiscard]] std::size_t size() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return checks_.size();
    }

    /**
     * @brief Check if this composite has no child checks
     * @return true if empty
     */
    [[nodiscard]] bool empty() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return checks_.empty();
    }

    /**
     * @brief Clear all child checks
     */
    void clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        checks_.clear();
    }

    /**
     * @brief Get all child check names
     * @return Vector of check names
     */
    [[nodiscard]] std::vector<std::string> get_check_names() const {
        std::lock_guard<std::mutex> lock(mutex_);
        std::vector<std::string> names;
        names.reserve(checks_.size());
        for (const auto& check : checks_) {
            names.push_back(check->get_name());
        }
        return names;
    }

private:
    std::string name_;
    health_check_type type_;
    bool critical_;
    std::vector<std::shared_ptr<health_check>> checks_;
    mutable std::mutex mutex_;
};

}  // namespace kcenon::common::interfaces
