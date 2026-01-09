// BSD 3-Clause License
// Copyright (c) 2025, kcenon
// See the LICENSE file in the project root for full license information.

/**
 * @file health_monitor.h
 * @brief Health monitoring system with dependency management.
 *
 * This header defines the health_monitor class which provides a complete
 * health monitoring solution with check registration, dependency management,
 * and recovery handlers.
 */

#pragma once

#include <atomic>
#include <chrono>
#include <functional>
#include <memory>
#include <mutex>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

#include "health_check.h"
#include "health_dependency_graph.h"

namespace kcenon::common::interfaces {

/**
 * @struct health_monitor_stats
 * @brief Statistics for health monitoring
 */
struct health_monitor_stats {
    std::size_t total_checks{0};
    std::size_t healthy_count{0};
    std::size_t degraded_count{0};
    std::size_t unhealthy_count{0};
    std::size_t unknown_count{0};
    std::size_t check_executions{0};
    std::size_t recovery_attempts{0};
    std::size_t successful_recoveries{0};
    std::chrono::system_clock::time_point last_check_time;
    std::chrono::milliseconds last_check_duration{0};
};

/**
 * @brief Recovery handler function type
 *
 * A recovery handler is called when a health check fails.
 * It should attempt to recover the failed component.
 *
 * @return true if recovery was successful
 */
using recovery_handler = std::function<bool()>;

/**
 * @class health_monitor
 * @brief Central health monitoring system
 *
 * This class provides a complete health monitoring solution with:
 * - Health check registration and management
 * - Dependency tracking between checks
 * - Automatic health check execution
 * - Recovery handler support
 * - Statistics and reporting
 *
 * Example usage:
 * @code
 * health_monitor monitor;
 *
 * // Register health checks
 * monitor.register_check("database", db_check);
 * monitor.register_check("cache", cache_check);
 * monitor.register_check("api", api_check);
 *
 * // Define dependencies
 * monitor.add_dependency("api", "database");
 * monitor.add_dependency("api", "cache");
 *
 * // Register recovery handlers
 * monitor.register_recovery_handler("database", []() {
 *     // Attempt to reconnect
 *     return true;
 * });
 *
 * // Start monitoring
 * monitor.start();
 *
 * // Get health report
 * std::cout << monitor.get_health_report();
 * @endcode
 */
class health_monitor {
public:
    health_monitor() = default;

    ~health_monitor() { stop().value_or(std::monostate{}); }

    health_monitor(const health_monitor&) = delete;
    health_monitor& operator=(const health_monitor&) = delete;
    health_monitor(health_monitor&&) = delete;
    health_monitor& operator=(health_monitor&&) = delete;

    /**
     * @brief Register a health check
     * @param name Unique name for this check
     * @param check Health check implementation
     * @return Result indicating success or failure
     */
    Result<bool> register_check(const std::string& name, std::shared_ptr<health_check> check) {
        std::lock_guard<std::mutex> lock(mutex_);

        auto result = graph_.add_node(name, std::move(check));
        if (result.is_ok()) {
            ++stats_.total_checks;
        }
        return result;
    }

    /**
     * @brief Unregister a health check
     * @param name Name of the check to remove
     * @return Result indicating success or failure
     */
    Result<bool> unregister_check(const std::string& name) {
        std::lock_guard<std::mutex> lock(mutex_);

        auto result = graph_.remove_node(name);
        if (result.is_ok()) {
            --stats_.total_checks;
            recovery_handlers_.erase(name);
            last_results_.erase(name);
        }
        return result;
    }

    /**
     * @brief Execute a specific health check
     * @param name Name of the check to execute
     * @return Result containing health check result or error
     */
    Result<health_check_result> check(const std::string& name) {
        std::lock_guard<std::mutex> lock(mutex_);

        auto start_time = std::chrono::steady_clock::now();
        auto result = graph_.check_with_dependencies(name);
        auto end_time = std::chrono::steady_clock::now();

        if (result.is_ok()) {
            last_results_[name] = result.value();
            update_stats_after_check(result.value());

            stats_.last_check_time = std::chrono::system_clock::now();
            stats_.last_check_duration = std::chrono::duration_cast<std::chrono::milliseconds>(
                end_time - start_time);
            ++stats_.check_executions;

            // Trigger recovery if unhealthy
            if (result.value().status == health_status::unhealthy) {
                attempt_recovery(name);
            }
        }

        return result;
    }

    /**
     * @brief Add a dependency between health checks
     * @param dependent The check that depends on another
     * @param dependency The check being depended upon
     * @return Result indicating success or failure
     */
    Result<bool> add_dependency(const std::string& dependent, const std::string& dependency) {
        std::lock_guard<std::mutex> lock(mutex_);
        return graph_.add_dependency(dependent, dependency);
    }

    /**
     * @brief Start the health monitoring
     * @return Result indicating success or failure
     */
    VoidResult start() {
        if (running_.exchange(true)) {
            return {error_info{1, "Health monitor is already running", "health_monitor"}};
        }
        return ok(std::monostate{});
    }

    /**
     * @brief Stop the health monitoring
     * @return Result indicating success or failure
     */
    VoidResult stop() {
        if (!running_.exchange(false)) {
            return {error_info{1, "Health monitor is not running", "health_monitor"}};
        }
        return ok(std::monostate{});
    }

    /**
     * @brief Check if health monitoring is running
     * @return true if running
     */
    [[nodiscard]] bool is_running() const { return running_.load(); }

    /**
     * @brief Refresh all health checks
     *
     * Executes all registered health checks and updates statistics.
     */
    void refresh() {
        std::lock_guard<std::mutex> lock(mutex_);

        auto start_time = std::chrono::steady_clock::now();

        // Reset counts
        stats_.healthy_count = 0;
        stats_.degraded_count = 0;
        stats_.unhealthy_count = 0;
        stats_.unknown_count = 0;

        auto nodes = graph_.get_all_nodes();
        for (const auto& name : nodes) {
            auto result = graph_.check_with_dependencies(name);
            if (result.is_ok()) {
                last_results_[name] = result.value();
                update_stats_after_check(result.value());

                if (result.value().status == health_status::unhealthy) {
                    attempt_recovery(name);
                }
            }
        }

        auto end_time = std::chrono::steady_clock::now();
        stats_.last_check_time = std::chrono::system_clock::now();
        stats_.last_check_duration = std::chrono::duration_cast<std::chrono::milliseconds>(
            end_time - start_time);
        ++stats_.check_executions;
    }

    /**
     * @brief Register a recovery handler for a health check
     * @param name Name of the health check
     * @param handler Recovery function to execute on failure
     */
    void register_recovery_handler(const std::string& name, recovery_handler handler) {
        std::lock_guard<std::mutex> lock(mutex_);
        recovery_handlers_[name] = std::move(handler);
    }

    /**
     * @brief Get monitoring statistics
     * @return Current statistics
     */
    [[nodiscard]] health_monitor_stats get_stats() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return stats_;
    }

    /**
     * @brief Get a formatted health report
     * @return Human-readable health report
     */
    [[nodiscard]] std::string get_health_report() const {
        std::lock_guard<std::mutex> lock(mutex_);

        std::ostringstream report;
        report << "=== Health Report ===\n";
        report << "Status: " << get_overall_status_string() << "\n";
        report << "Total Checks: " << stats_.total_checks << "\n";
        report << "Healthy: " << stats_.healthy_count << "\n";
        report << "Degraded: " << stats_.degraded_count << "\n";
        report << "Unhealthy: " << stats_.unhealthy_count << "\n";
        report << "Unknown: " << stats_.unknown_count << "\n";
        report << "\n--- Individual Checks ---\n";

        for (const auto& [name, result] : last_results_) {
            report << name << ": " << to_string(result.status);
            if (!result.message.empty()) {
                report << " - " << result.message;
            }
            report << "\n";
        }

        return report.str();
    }

    /**
     * @brief Get the overall health status
     * @return Overall health status based on all checks
     */
    [[nodiscard]] health_status get_overall_status() const {
        std::lock_guard<std::mutex> lock(mutex_);

        if (stats_.unhealthy_count > 0) {
            return health_status::unhealthy;
        }
        if (stats_.degraded_count > 0) {
            return health_status::degraded;
        }
        if (stats_.unknown_count > 0) {
            return health_status::unknown;
        }
        if (stats_.healthy_count > 0) {
            return health_status::healthy;
        }
        return health_status::unknown;
    }

    /**
     * @brief Check if a health check is registered
     * @param name Name of the health check
     * @return true if registered
     */
    [[nodiscard]] bool has_check(const std::string& name) const {
        std::lock_guard<std::mutex> lock(mutex_);
        return graph_.has_node(name);
    }

    /**
     * @brief Get all registered check names
     * @return Vector of check names
     */
    [[nodiscard]] std::vector<std::string> get_check_names() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return graph_.get_all_nodes();
    }

private:
    void update_stats_after_check(const health_check_result& result) {
        switch (result.status) {
            case health_status::healthy:
                ++stats_.healthy_count;
                break;
            case health_status::degraded:
                ++stats_.degraded_count;
                break;
            case health_status::unhealthy:
                ++stats_.unhealthy_count;
                break;
            case health_status::unknown:
                ++stats_.unknown_count;
                break;
        }
    }

    void attempt_recovery(const std::string& name) {
        auto it = recovery_handlers_.find(name);
        if (it == recovery_handlers_.end()) {
            return;
        }

        ++stats_.recovery_attempts;
        if (it->second()) {
            ++stats_.successful_recoveries;
        }
    }

    [[nodiscard]] std::string get_overall_status_string() const {
        return to_string(get_overall_status_internal());
    }

    [[nodiscard]] health_status get_overall_status_internal() const {
        if (stats_.unhealthy_count > 0) {
            return health_status::unhealthy;
        }
        if (stats_.degraded_count > 0) {
            return health_status::degraded;
        }
        if (stats_.unknown_count > 0) {
            return health_status::unknown;
        }
        if (stats_.healthy_count > 0) {
            return health_status::healthy;
        }
        return health_status::unknown;
    }

    health_dependency_graph graph_;
    std::unordered_map<std::string, recovery_handler> recovery_handlers_;
    std::unordered_map<std::string, health_check_result> last_results_;
    health_monitor_stats stats_;
    std::atomic<bool> running_{false};
    mutable std::mutex mutex_;
};

/**
 * @brief Get the global health monitor instance
 * @return Reference to the global health monitor singleton
 *
 * This function provides access to a global health monitor instance
 * for application-wide health monitoring.
 *
 * Example usage:
 * @code
 * auto& monitor = global_health_monitor();
 * monitor.register_check("database", db_check);
 * @endcode
 */
inline health_monitor& global_health_monitor() {
    static health_monitor instance;
    return instance;
}

}  // namespace kcenon::common::interfaces
