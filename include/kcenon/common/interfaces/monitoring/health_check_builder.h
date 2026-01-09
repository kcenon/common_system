// BSD 3-Clause License
// Copyright (c) 2025, kcenon
// See the LICENSE file in the project root for full license information.

/**
 * @file health_check_builder.h
 * @brief Builder pattern for creating health checks.
 *
 * This header defines the health_check_builder class which provides
 * a fluent API for constructing health checks.
 */

#pragma once

#include <chrono>
#include <functional>
#include <memory>
#include <string>

#include "health_check.h"

namespace kcenon::common::interfaces {

/**
 * @class health_check_builder
 * @brief Fluent builder for creating health checks
 *
 * This class provides a builder pattern for constructing health checks
 * with a fluent API. It allows setting various properties before
 * creating the final health check object.
 *
 * Example usage:
 * @code
 * auto check = health_check_builder()
 *     .name("database")
 *     .type(health_check_type::dependency)
 *     .timeout(std::chrono::seconds{10})
 *     .critical(true)
 *     .with_check([]() {
 *         health_check_result result;
 *         result.status = health_status::healthy;
 *         result.message = "Database OK";
 *         return result;
 *     })
 *     .build();
 * @endcode
 */
class health_check_builder {
public:
    using check_fn_type = std::function<health_check_result()>;

    health_check_builder() = default;

    /**
     * @brief Set the health check name
     * @param value Health check name
     * @return Reference to this builder
     */
    health_check_builder& name(std::string value) {
        name_ = std::move(value);
        return *this;
    }

    /**
     * @brief Set the health check type
     * @param value Health check type
     * @return Reference to this builder
     */
    health_check_builder& type(health_check_type value) {
        type_ = value;
        return *this;
    }

    /**
     * @brief Set the check function
     * @param fn Function to execute for health check
     * @return Reference to this builder
     */
    health_check_builder& with_check(check_fn_type fn) {
        check_fn_ = std::move(fn);
        return *this;
    }

    /**
     * @brief Set whether this check is critical
     * @param value true if critical
     * @return Reference to this builder
     */
    health_check_builder& critical(bool value) {
        critical_ = value;
        return *this;
    }

    /**
     * @brief Set the check timeout
     * @param value Timeout duration
     * @return Reference to this builder
     */
    health_check_builder& timeout(std::chrono::milliseconds value) {
        timeout_ = value;
        return *this;
    }

    /**
     * @brief Build the health check
     * @return Result containing the health check or error
     *
     * Validates that all required fields are set before building.
     */
    [[nodiscard]] Result<std::shared_ptr<health_check>> build() const {
        if (name_.empty()) {
            return {error_info{1, "Health check name is required", "health_check_builder"}};
        }

        if (!check_fn_) {
            return {error_info{2, "Check function is required", "health_check_builder"}};
        }

        std::shared_ptr<health_check> check = std::make_shared<lambda_health_check>(
            name_, type_, check_fn_, critical_, timeout_);
        return ok(std::move(check));
    }

    /**
     * @brief Build the health check without validation
     * @return Shared pointer to the health check (may be invalid)
     *
     * Use this method only when you're sure all required fields are set.
     * For safer construction, use build() instead.
     */
    [[nodiscard]] std::shared_ptr<health_check> build_unsafe() const {
        return std::make_shared<lambda_health_check>(
            name_, type_, check_fn_, critical_, timeout_);
    }

    /**
     * @brief Reset the builder to initial state
     * @return Reference to this builder
     */
    health_check_builder& reset() {
        name_.clear();
        type_ = health_check_type::custom;
        check_fn_ = nullptr;
        critical_ = true;
        timeout_ = std::chrono::milliseconds{5000};
        return *this;
    }

private:
    std::string name_;
    health_check_type type_ = health_check_type::custom;
    check_fn_type check_fn_;
    bool critical_ = true;
    std::chrono::milliseconds timeout_{5000};
};

}  // namespace kcenon::common::interfaces
