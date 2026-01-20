// BSD 3-Clause License
// Copyright (c) 2025, kcenon
// See the LICENSE file in the project root for full license information.

/**
 * @file health_check.h
 * @brief Base classes and types for health checking functionality.
 *
 * This header defines the fundamental health check abstractions including
 * the health_check base class and health_check_type enumeration.
 */

#pragma once

#include <array>
#include <chrono>
#include <functional>
#include <string>
#include <string_view>
#include <utility>

#include "../monitoring_interface.h"
#include "../../utils/enum_serialization.h"

namespace kcenon::common::interfaces {

/**
 * @enum health_check_type
 * @brief Types of health checks supported by the system
 */
enum class health_check_type {
    liveness,   // Basic alive check - is the service running?
    readiness,  // Ready to accept traffic?
    startup,    // Has the service completed initialization?
    dependency, // External dependency check (database, cache, etc.)
    custom      // User-defined health check type
};

}  // namespace kcenon::common::interfaces

namespace kcenon::common {

/**
 * @brief Specialization of enum_traits for health_check_type
 */
template<>
struct enum_traits<interfaces::health_check_type> {
    static constexpr auto values = std::array{
        std::pair{interfaces::health_check_type::liveness, std::string_view{"LIVENESS"}},
        std::pair{interfaces::health_check_type::readiness, std::string_view{"READINESS"}},
        std::pair{interfaces::health_check_type::startup, std::string_view{"STARTUP"}},
        std::pair{interfaces::health_check_type::dependency, std::string_view{"DEPENDENCY"}},
        std::pair{interfaces::health_check_type::custom, std::string_view{"CUSTOM"}},
    };
    static constexpr std::string_view module_name = "health_check";
};

}  // namespace kcenon::common

namespace kcenon::common::interfaces {

/**
 * @brief Convert health check type to string
 */
inline std::string to_string(health_check_type type) {
    return enum_to_string(type);
}

/**
 * @brief Convert string to health check type
 */
inline Result<health_check_type> health_check_type_from_string(const std::string& str) {
    return enum_from_string<health_check_type>(str);
}

/**
 * @interface health_check
 * @brief Abstract base class for health checks
 *
 * This class defines the contract for health check implementations.
 * Subclasses should implement the check() method to perform their
 * specific health verification logic.
 *
 * Example usage:
 * @code
 * class database_health_check : public health_check {
 * public:
 *     std::string get_name() const override { return "database"; }
 *     health_check_type get_type() const override { return health_check_type::dependency; }
 *     health_check_result check() override {
 *         // Check database connectivity
 *         health_check_result result;
 *         if (db_connection_->is_connected()) {
 *             result.status = health_status::healthy;
 *             result.message = "Database connection OK";
 *         } else {
 *             result.status = health_status::unhealthy;
 *             result.message = "Database connection failed";
 *         }
 *         return result;
 *     }
 * };
 * @endcode
 */
class health_check {
public:
    health_check() = default;
    virtual ~health_check() = default;

    health_check(const health_check&) = delete;
    health_check& operator=(const health_check&) = delete;
    health_check(health_check&&) = delete;
    health_check& operator=(health_check&&) = delete;

    /**
     * @brief Get the unique name of this health check
     * @return Health check name
     */
    [[nodiscard]] virtual std::string get_name() const = 0;

    /**
     * @brief Get the type of this health check
     * @return Health check type
     */
    [[nodiscard]] virtual health_check_type get_type() const = 0;

    /**
     * @brief Perform the health check
     * @return Health check result containing status and details
     */
    virtual health_check_result check() = 0;

    /**
     * @brief Get timeout duration for this health check
     * @return Timeout duration (default: 5 seconds)
     */
    [[nodiscard]] virtual std::chrono::milliseconds get_timeout() const {
        return std::chrono::milliseconds{5000};
    }

    /**
     * @brief Check if this health check is critical
     *
     * Critical health checks affect the overall system health status.
     * Non-critical checks are reported but don't impact system health.
     *
     * @return true if this is a critical health check
     */
    [[nodiscard]] virtual bool is_critical() const { return true; }
};

/**
 * @class lambda_health_check
 * @brief Health check implementation using a lambda function
 *
 * This class allows creating health checks from lambda functions,
 * useful for simple checks that don't require a full class hierarchy.
 *
 * Example usage:
 * @code
 * auto check = std::make_shared<lambda_health_check>(
 *     "memory_check",
 *     health_check_type::liveness,
 *     []() -> health_check_result {
 *         health_check_result result;
 *         result.status = health_status::healthy;
 *         result.message = "Memory usage OK";
 *         return result;
 *     }
 * );
 * @endcode
 */
class lambda_health_check : public health_check {
public:
    using check_function = std::function<health_check_result()>;

    /**
     * @brief Construct a lambda health check
     * @param name Health check name
     * @param type Health check type
     * @param check_fn Function to execute for health check
     * @param critical Whether this check is critical (default: true)
     * @param timeout Check timeout (default: 5 seconds)
     */
    lambda_health_check(std::string name,
                        health_check_type type,
                        check_function check_fn,
                        bool critical = true,
                        std::chrono::milliseconds timeout = std::chrono::milliseconds{5000})
        : name_(std::move(name))
        , type_(type)
        , check_fn_(std::move(check_fn))
        , critical_(critical)
        , timeout_(timeout) {}

    [[nodiscard]] std::string get_name() const override { return name_; }

    [[nodiscard]] health_check_type get_type() const override { return type_; }

    health_check_result check() override {
        if (!check_fn_) {
            health_check_result result;
            result.status = health_status::unknown;
            result.message = "No check function configured";
            return result;
        }
        return check_fn_();
    }

    [[nodiscard]] std::chrono::milliseconds get_timeout() const override { return timeout_; }

    [[nodiscard]] bool is_critical() const override { return critical_; }

private:
    std::string name_;
    health_check_type type_;
    check_function check_fn_;
    bool critical_;
    std::chrono::milliseconds timeout_;
};

}  // namespace kcenon::common::interfaces
