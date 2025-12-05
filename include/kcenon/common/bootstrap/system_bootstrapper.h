// BSD 3-Clause License
// Copyright (c) 2025, kcenon
// See the LICENSE file in the project root for full license information.

/**
 * @file system_bootstrapper.h
 * @brief SystemBootstrapper implementation for runtime binding.
 *
 * This header provides a fluent API for system initialization at the
 * application level. It integrates with GlobalLoggerRegistry to provide
 * centralized logger management and supports initialization/shutdown hooks
 * for lifecycle management.
 *
 * Thread Safety:
 * - SystemBootstrapper is NOT thread-safe for concurrent modification.
 * - All configuration methods should be called from a single thread.
 * - Once initialized, the registered loggers can be safely accessed
 *   from multiple threads through GlobalLoggerRegistry.
 *
 * RAII Support:
 * - Destructor automatically calls shutdown() if initialized.
 * - Prevents duplicate initialization and duplicate shutdown.
 *
 * Usage Example:
 * @code
 * #include <kcenon/common/bootstrap/system_bootstrapper.h>
 *
 * int main() {
 *     SystemBootstrapper bootstrapper;
 *     bootstrapper
 *         .with_default_logger([] { return create_console_logger(); })
 *         .with_logger("database", [] { return create_file_logger("db.log"); })
 *         .on_initialize([] { LOG_INFO("System started"); })
 *         .on_shutdown([] { LOG_INFO("System stopped"); });
 *
 *     auto result = bootstrapper.initialize();
 *     if (result.is_err()) {
 *         std::cerr << "Failed to initialize: " << result.error().message;
 *         return 1;
 *     }
 *
 *     // Application logic here...
 *
 *     // Shutdown is called automatically by destructor
 *     return 0;
 * }
 * @endcode
 *
 * @see Issue #176 for implementation requirements.
 * @see global_logger_registry.h for logger management.
 */

#pragma once

#include "../interfaces/global_logger_registry.h"
#include "../patterns/result.h"

#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

namespace kcenon::common::bootstrap {

/**
 * @class SystemBootstrapper
 * @brief Fluent API for system initialization and logger registration.
 *
 * SystemBootstrapper provides a centralized mechanism for:
 * - Registering default and named loggers using factory functions
 * - Defining initialization and shutdown callbacks
 * - Managing application lifecycle with RAII support
 *
 * Key Features:
 * - Fluent API with method chaining for expressive configuration
 * - Factory-based lazy initialization of loggers
 * - RAII support with automatic shutdown on destruction
 * - Prevention of duplicate initialization/shutdown
 * - Integration with GlobalLoggerRegistry for thread-safe logger access
 *
 * Design Notes:
 * - Configuration methods are not thread-safe; call from single thread
 * - Once initialize() succeeds, loggers are available globally
 * - Shutdown callbacks are called in reverse order of registration
 * - Initialize callbacks are called in order of registration
 */
class SystemBootstrapper {
public:
    /**
     * @brief Default constructor.
     *
     * Creates an uninitialized bootstrapper. Call configuration methods
     * and then initialize() to start the system.
     */
    SystemBootstrapper() = default;

    /**
     * @brief Destructor with automatic shutdown.
     *
     * Automatically calls shutdown() if the bootstrapper was initialized.
     * This ensures proper cleanup even if shutdown() is not called explicitly.
     */
    ~SystemBootstrapper();

    // Non-copyable
    SystemBootstrapper(const SystemBootstrapper&) = delete;
    SystemBootstrapper& operator=(const SystemBootstrapper&) = delete;

    // Movable
    SystemBootstrapper(SystemBootstrapper&& other) noexcept;
    SystemBootstrapper& operator=(SystemBootstrapper&& other) noexcept;

    // =========================================================================
    // Fluent Configuration API
    // =========================================================================

    /**
     * @brief Register a factory for the default logger.
     *
     * The factory function will be invoked during initialize() to create
     * the default logger. The logger will be registered with
     * GlobalLoggerRegistry and accessible via get_logger().
     *
     * @param factory Factory function that creates the default logger
     * @return Reference to this bootstrapper for method chaining
     *
     * @note If called multiple times, only the last factory is used.
     */
    SystemBootstrapper& with_default_logger(interfaces::LoggerFactory factory);

    /**
     * @brief Register a factory for a named logger.
     *
     * The factory function will be invoked during initialize() to create
     * the named logger. The logger will be registered with
     * GlobalLoggerRegistry and accessible via get_logger(name).
     *
     * @param name Logger name (case-sensitive)
     * @param factory Factory function that creates the logger
     * @return Reference to this bootstrapper for method chaining
     *
     * @note If the same name is registered multiple times, only the last
     *       factory is used.
     */
    SystemBootstrapper& with_logger(const std::string& name,
                                     interfaces::LoggerFactory factory);

    /**
     * @brief Register an initialization callback.
     *
     * The callback will be invoked during initialize() after all loggers
     * are registered. Multiple callbacks can be registered and will be
     * called in the order of registration.
     *
     * @param callback Function to call during initialization
     * @return Reference to this bootstrapper for method chaining
     */
    SystemBootstrapper& on_initialize(std::function<void()> callback);

    /**
     * @brief Register a shutdown callback.
     *
     * The callback will be invoked during shutdown() before loggers are
     * cleared. Multiple callbacks can be registered and will be called
     * in reverse order of registration (LIFO).
     *
     * @param callback Function to call during shutdown
     * @return Reference to this bootstrapper for method chaining
     */
    SystemBootstrapper& on_shutdown(std::function<void()> callback);

    // =========================================================================
    // Lifecycle Management
    // =========================================================================

    /**
     * @brief Initialize the system.
     *
     * Performs the following steps in order:
     * 1. Validates that initialization hasn't already occurred
     * 2. Creates and registers the default logger (if configured)
     * 3. Creates and registers all named loggers
     * 4. Calls all initialization callbacks in registration order
     * 5. Marks the bootstrapper as initialized
     *
     * If any step fails, the operation is rolled back and an error is returned.
     *
     * @return VoidResult indicating success or error
     *
     * @note This method should only be called once. Subsequent calls return
     *       an error.
     */
    VoidResult initialize();

    /**
     * @brief Shutdown the system.
     *
     * Performs the following steps in order:
     * 1. Validates that the system is initialized
     * 2. Calls all shutdown callbacks in reverse registration order
     * 3. Clears all loggers from GlobalLoggerRegistry
     * 4. Marks the bootstrapper as not initialized
     *
     * @note This method is idempotent; calling it multiple times after
     *       the first call has no effect.
     * @note The destructor calls this automatically if needed.
     */
    void shutdown();

    /**
     * @brief Check if the system is initialized.
     *
     * @return true if initialize() has been called successfully
     */
    bool is_initialized() const noexcept;

    /**
     * @brief Reset the bootstrapper to initial state.
     *
     * Clears all registered factories, callbacks, and resets the
     * initialized state. If currently initialized, shutdown() is
     * called first.
     *
     * Useful for testing or reconfiguration scenarios.
     */
    void reset();

private:
    /**
     * @brief Register all loggers with GlobalLoggerRegistry.
     *
     * @return VoidResult indicating success or error
     */
    VoidResult register_loggers();

    /**
     * @brief Execute all initialization callbacks.
     */
    void execute_init_callbacks();

    /**
     * @brief Execute all shutdown callbacks in reverse order.
     */
    void execute_shutdown_callbacks();

    /**
     * @brief Clear all loggers from GlobalLoggerRegistry.
     */
    void clear_loggers();

    // Configuration state
    interfaces::LoggerFactory default_logger_factory_;
    std::vector<std::pair<std::string, interfaces::LoggerFactory>> named_logger_factories_;
    std::vector<std::function<void()>> init_callbacks_;
    std::vector<std::function<void()>> shutdown_callbacks_;

    // Lifecycle state
    bool initialized_ = false;
    mutable std::mutex state_mutex_;
};

// =============================================================================
// SystemBootstrapper Implementation
// =============================================================================

inline SystemBootstrapper::~SystemBootstrapper() {
    if (initialized_) {
        shutdown();
    }
}

inline SystemBootstrapper::SystemBootstrapper(SystemBootstrapper&& other) noexcept
    : default_logger_factory_(std::move(other.default_logger_factory_))
    , named_logger_factories_(std::move(other.named_logger_factories_))
    , init_callbacks_(std::move(other.init_callbacks_))
    , shutdown_callbacks_(std::move(other.shutdown_callbacks_))
    , initialized_(other.initialized_) {
    other.initialized_ = false;  // Prevent double shutdown
}

inline SystemBootstrapper& SystemBootstrapper::operator=(
    SystemBootstrapper&& other) noexcept {
    if (this != &other) {
        // Shutdown current state if initialized
        if (initialized_) {
            shutdown();
        }

        default_logger_factory_ = std::move(other.default_logger_factory_);
        named_logger_factories_ = std::move(other.named_logger_factories_);
        init_callbacks_ = std::move(other.init_callbacks_);
        shutdown_callbacks_ = std::move(other.shutdown_callbacks_);
        initialized_ = other.initialized_;
        other.initialized_ = false;  // Prevent double shutdown
    }
    return *this;
}

inline SystemBootstrapper& SystemBootstrapper::with_default_logger(
    interfaces::LoggerFactory factory) {
    default_logger_factory_ = std::move(factory);
    return *this;
}

inline SystemBootstrapper& SystemBootstrapper::with_logger(
    const std::string& name,
    interfaces::LoggerFactory factory) {
    // Check if name already exists and update, otherwise add
    for (auto& entry : named_logger_factories_) {
        if (entry.first == name) {
            entry.second = std::move(factory);
            return *this;
        }
    }
    named_logger_factories_.emplace_back(name, std::move(factory));
    return *this;
}

inline SystemBootstrapper& SystemBootstrapper::on_initialize(
    std::function<void()> callback) {
    if (callback) {
        init_callbacks_.push_back(std::move(callback));
    }
    return *this;
}

inline SystemBootstrapper& SystemBootstrapper::on_shutdown(
    std::function<void()> callback) {
    if (callback) {
        shutdown_callbacks_.push_back(std::move(callback));
    }
    return *this;
}

inline VoidResult SystemBootstrapper::initialize() {
    std::lock_guard<std::mutex> lock(state_mutex_);

    if (initialized_) {
        return make_error<std::monostate>(
            error_codes::ALREADY_EXISTS,
            "SystemBootstrapper already initialized",
            "bootstrap::SystemBootstrapper"
        );
    }

    // Step 1: Register all loggers
    auto result = register_loggers();
    if (result.is_err()) {
        return result;
    }

    // Step 2: Execute initialization callbacks
    execute_init_callbacks();

    // Step 3: Mark as initialized
    initialized_ = true;

    return VoidResult::ok({});
}

inline void SystemBootstrapper::shutdown() {
    std::lock_guard<std::mutex> lock(state_mutex_);

    if (!initialized_) {
        return;  // Already shutdown or never initialized
    }

    // Step 1: Execute shutdown callbacks in reverse order
    execute_shutdown_callbacks();

    // Step 2: Clear loggers from registry
    clear_loggers();

    // Step 3: Mark as not initialized
    initialized_ = false;
}

inline bool SystemBootstrapper::is_initialized() const noexcept {
    std::lock_guard<std::mutex> lock(state_mutex_);
    return initialized_;
}

inline void SystemBootstrapper::reset() {
    // Shutdown if initialized
    if (is_initialized()) {
        shutdown();
    }

    std::lock_guard<std::mutex> lock(state_mutex_);

    // Clear all configuration
    default_logger_factory_ = nullptr;
    named_logger_factories_.clear();
    init_callbacks_.clear();
    shutdown_callbacks_.clear();
}

inline VoidResult SystemBootstrapper::register_loggers() {
    auto& registry = interfaces::GlobalLoggerRegistry::instance();

    // Register default logger
    if (default_logger_factory_) {
        auto logger = default_logger_factory_();
        if (!logger) {
            return make_error<std::monostate>(
                error_codes::INTERNAL_ERROR,
                "Default logger factory returned null",
                "bootstrap::SystemBootstrapper"
            );
        }
        auto result = registry.set_default_logger(std::move(logger));
        if (result.is_err()) {
            return result;
        }
    }

    // Register named loggers
    for (const auto& [name, factory] : named_logger_factories_) {
        if (!factory) {
            continue;  // Skip null factories
        }
        auto logger = factory();
        if (!logger) {
            return make_error<std::monostate>(
                error_codes::INTERNAL_ERROR,
                "Logger factory for '" + name + "' returned null",
                "bootstrap::SystemBootstrapper"
            );
        }
        auto result = registry.register_logger(name, std::move(logger));
        if (result.is_err()) {
            // Clean up already registered loggers
            clear_loggers();
            return result;
        }
    }

    return VoidResult::ok({});
}

inline void SystemBootstrapper::execute_init_callbacks() {
    for (const auto& callback : init_callbacks_) {
        if (callback) {
            callback();
        }
    }
}

inline void SystemBootstrapper::execute_shutdown_callbacks() {
    // Execute in reverse order (LIFO)
    for (auto it = shutdown_callbacks_.rbegin();
         it != shutdown_callbacks_.rend();
         ++it) {
        if (*it) {
            (*it)();
        }
    }
}

inline void SystemBootstrapper::clear_loggers() {
    interfaces::GlobalLoggerRegistry::instance().clear();
}

} // namespace kcenon::common::bootstrap
