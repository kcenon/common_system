// BSD 3-Clause License
// Copyright (c) 2025, kcenon
// See the LICENSE file in the project root for full license information.

/**
 * @file global_logger_registry.h
 * @brief GlobalLoggerRegistry implementation for runtime binding.
 *
 * This header provides a centralized, thread-safe registry for managing logger
 * instances across all subsystems. It resolves the circular dependency between
 * thread_system and logger_system by providing a decoupled logging registry
 * that can be bound at runtime.
 *
 * Thread Safety:
 * - GlobalLoggerRegistry is thread-safe for all operations.
 * - Uses std::shared_mutex for read/write locking.
 * - Factory-based lazy initialization is protected against race conditions.
 * - NullLogger fallback ensures safe operation when no logger is registered.
 *
 * @see Issue #174 for implementation requirements.
 * @see logger_interface.h for the ILoggerRegistry interface definition.
 */

#pragma once

#include "logger_interface.h"

#include <functional>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <string>
#include <unordered_map>

namespace kcenon::common {
namespace interfaces {

/**
 * @class NullLogger
 * @brief A no-op logger implementation for fallback scenarios.
 *
 * NullLogger provides a safe default when no logger has been registered.
 * All logging operations are no-ops that return success without performing
 * any actual logging. This prevents null pointer dereferences and allows
 * code to function (silently) even when logging is not configured.
 *
 * @note Issue #177: Updated to support source_location-based logging.
 */
class NullLogger : public ILogger {
public:
    NullLogger() = default;
    ~NullLogger() override = default;

    VoidResult log(log_level /*level*/, const std::string& /*message*/) override {
        return VoidResult::ok({});
    }

    /**
     * @brief Log with source_location (no-op)
     * @note Issue #177: Preferred logging method with source_location.
     */
    VoidResult log(log_level /*level*/,
                   std::string_view /*message*/,
                   const source_location& /*loc*/ = source_location::current()) override {
        return VoidResult::ok({});
    }

// Suppress deprecation warning for implementing the deprecated interface method
#if defined(__GNUC__) || defined(__clang__)
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#elif defined(_MSC_VER)
    #pragma warning(push)
    #pragma warning(disable: 4996)
#endif

    VoidResult log(log_level /*level*/,
                   const std::string& /*message*/,
                   const std::string& /*file*/,
                   int /*line*/,
                   const std::string& /*function*/) override {
        return VoidResult::ok({});
    }

#if defined(__GNUC__) || defined(__clang__)
    #pragma GCC diagnostic pop
#elif defined(_MSC_VER)
    #pragma warning(pop)
#endif

    VoidResult log(const log_entry& /*entry*/) override {
        return VoidResult::ok({});
    }

    bool is_enabled(log_level /*level*/) const override {
        return false;
    }

    VoidResult set_level(log_level /*level*/) override {
        return VoidResult::ok({});
    }

    log_level get_level() const override {
        return log_level::off;
    }

    VoidResult flush() override {
        return VoidResult::ok({});
    }
};

/**
 * @class GlobalLoggerRegistry
 * @brief Thread-safe singleton registry for managing logger instances.
 *
 * GlobalLoggerRegistry implements the ILoggerRegistry interface and provides:
 * - Thread-safe registration and retrieval of named loggers
 * - Default logger management
 * - Factory-based lazy initialization for deferred logger creation
 * - NullLogger fallback for unregistered logger requests
 *
 * This class resolves the circular dependency between thread_system and
 * logger_system by providing a centralized, decoupled logging registry
 * that can be accessed from any subsystem without creating direct dependencies.
 *
 * Usage Example:
 * @code
 * // Get the global registry
 * auto& registry = GlobalLoggerRegistry::instance();
 *
 * // Register a default logger
 * auto logger = std::make_shared<MyLogger>();
 * registry.set_default_logger(logger);
 *
 * // Register a named logger
 * registry.register_logger("network", network_logger);
 *
 * // Retrieve loggers
 * auto default_log = registry.get_default_logger();
 * auto network_log = registry.get_logger("network");
 *
 * // Use factory for lazy initialization
 * registry.register_factory("database", []() {
 *     return std::make_shared<DatabaseLogger>();
 * });
 * @endcode
 */
class GlobalLoggerRegistry : public ILoggerRegistry {
public:
    /**
     * @brief Get the singleton instance of GlobalLoggerRegistry.
     *
     * Returns a reference to the application-wide singleton registry.
     * Thread-safe for concurrent access using Meyer's singleton pattern.
     *
     * @return Reference to the GlobalLoggerRegistry singleton
     */
    static GlobalLoggerRegistry& instance();

    // Delete copy and move operations
    GlobalLoggerRegistry(const GlobalLoggerRegistry&) = delete;
    GlobalLoggerRegistry& operator=(const GlobalLoggerRegistry&) = delete;
    GlobalLoggerRegistry(GlobalLoggerRegistry&&) = delete;
    GlobalLoggerRegistry& operator=(GlobalLoggerRegistry&&) = delete;

    // ===== ILoggerRegistry Implementation =====

    /**
     * @brief Register a logger with a name.
     *
     * Registers a logger instance under the specified name. If a logger
     * with the same name already exists, it will be replaced.
     *
     * @param name Logger name (case-sensitive)
     * @param logger Logger instance to register
     * @return VoidResult indicating success or error
     */
    VoidResult register_logger(const std::string& name,
                               std::shared_ptr<ILogger> logger) override;

    /**
     * @brief Get a logger by name.
     *
     * Retrieves a logger registered under the specified name. If no logger
     * is registered with that name but a factory exists, the factory will
     * be invoked to create the logger (lazy initialization).
     *
     * If no logger or factory is registered, returns a NullLogger instance.
     *
     * @param name Logger name (case-sensitive)
     * @return Logger instance or NullLogger if not found
     */
    std::shared_ptr<ILogger> get_logger(const std::string& name) override;

    /**
     * @brief Remove a logger by name.
     *
     * Unregisters a logger from the registry. Also removes any associated
     * factory if present.
     *
     * @param name Logger name
     * @return VoidResult indicating success (success even if not found)
     */
    VoidResult unregister_logger(const std::string& name) override;

    /**
     * @brief Get the default logger.
     *
     * Returns the default logger instance. If no default logger is set,
     * returns a NullLogger instance.
     *
     * @return Default logger instance or NullLogger
     */
    std::shared_ptr<ILogger> get_default_logger() override;

    /**
     * @brief Set the default logger.
     *
     * Sets the default logger instance. The default logger is used when
     * no specific logger name is provided.
     *
     * @param logger Logger instance to set as default
     * @return VoidResult indicating success or error
     */
    VoidResult set_default_logger(std::shared_ptr<ILogger> logger) override;

    // ===== Extended API for Factory Support =====

    /**
     * @brief Register a factory for lazy logger creation.
     *
     * Registers a factory function that will be invoked when a logger
     * with the specified name is first requested. This enables deferred
     * initialization of loggers.
     *
     * @param name Logger name (case-sensitive)
     * @param factory Factory function that creates the logger
     * @return VoidResult indicating success or error
     */
    VoidResult register_factory(const std::string& name, LoggerFactory factory);

    /**
     * @brief Set a factory for the default logger.
     *
     * Registers a factory function for creating the default logger.
     * The factory will be invoked when get_default_logger() is called
     * and no default logger has been set.
     *
     * @param factory Factory function that creates the default logger
     * @return VoidResult indicating success or error
     */
    VoidResult set_default_factory(LoggerFactory factory);

    /**
     * @brief Check if a logger is registered.
     *
     * Checks if a logger or factory is registered under the specified name.
     *
     * @param name Logger name to check
     * @return true if a logger or factory is registered
     */
    bool has_logger(const std::string& name) const;

    /**
     * @brief Check if a default logger is available.
     *
     * Checks if a default logger or factory is registered.
     *
     * @return true if a default logger or factory is available
     */
    bool has_default_logger() const;

    /**
     * @brief Clear all registered loggers and factories.
     *
     * Removes all registered loggers, factories, and the default logger.
     * Useful for testing or application shutdown.
     */
    void clear();

    /**
     * @brief Get the number of registered loggers.
     *
     * Returns the count of registered named loggers (not including the
     * default logger).
     *
     * @return Number of registered loggers
     */
    size_t size() const;

    /**
     * @brief Get the shared NullLogger instance.
     *
     * Returns the singleton NullLogger instance used as a fallback
     * when no logger is registered.
     *
     * @return Shared pointer to NullLogger
     */
    static std::shared_ptr<ILogger> null_logger();

private:
    GlobalLoggerRegistry() = default;
    ~GlobalLoggerRegistry() = default;

    /**
     * @brief Create logger from factory if available.
     *
     * Internal helper that creates a logger using a registered factory
     * and stores it in the logger map.
     *
     * @param name Logger name
     * @param lock Reference to held read lock (will be upgraded if needed)
     * @return Logger instance or nullptr if no factory
     */
    std::shared_ptr<ILogger> create_from_factory(const std::string& name);

    /**
     * @brief Create default logger from factory if available.
     *
     * @return Logger instance or nullptr if no factory
     */
    std::shared_ptr<ILogger> create_default_from_factory();

    mutable std::shared_mutex mutex_;
    std::unordered_map<std::string, std::shared_ptr<ILogger>> loggers_;
    std::unordered_map<std::string, LoggerFactory> factories_;
    std::shared_ptr<ILogger> default_logger_;
    LoggerFactory default_factory_;
};

// ============================================================================
// GlobalLoggerRegistry Implementation
// ============================================================================

inline GlobalLoggerRegistry& GlobalLoggerRegistry::instance() {
    static GlobalLoggerRegistry instance;
    return instance;
}

inline std::shared_ptr<ILogger> GlobalLoggerRegistry::null_logger() {
    static auto null_logger_instance = std::make_shared<NullLogger>();
    return null_logger_instance;
}

inline VoidResult GlobalLoggerRegistry::register_logger(
    const std::string& name,
    std::shared_ptr<ILogger> logger) {

    if (name.empty()) {
        return make_error<std::monostate>(
            error_codes::INVALID_ARGUMENT,
            "Logger name cannot be empty",
            "interfaces::GlobalLoggerRegistry"
        );
    }

    if (!logger) {
        return make_error<std::monostate>(
            error_codes::INVALID_ARGUMENT,
            "Logger instance cannot be null",
            "interfaces::GlobalLoggerRegistry"
        );
    }

    std::unique_lock lock(mutex_);
    loggers_[name] = std::move(logger);
    // Remove factory if one exists for this name (logger takes precedence)
    factories_.erase(name);

    return VoidResult::ok({});
}

inline std::shared_ptr<ILogger> GlobalLoggerRegistry::get_logger(const std::string& name) {
    // First, try to get existing logger with read lock
    {
        std::shared_lock lock(mutex_);
        auto it = loggers_.find(name);
        if (it != loggers_.end()) {
            return it->second;
        }
    }

    // Try to create from factory
    auto logger = create_from_factory(name);
    if (logger) {
        return logger;
    }

    // Return NullLogger as fallback
    return null_logger();
}

inline VoidResult GlobalLoggerRegistry::unregister_logger(const std::string& name) {
    std::unique_lock lock(mutex_);
    loggers_.erase(name);
    factories_.erase(name);
    return VoidResult::ok({});
}

inline std::shared_ptr<ILogger> GlobalLoggerRegistry::get_default_logger() {
    // First, try to get existing default logger with read lock
    {
        std::shared_lock lock(mutex_);
        if (default_logger_) {
            return default_logger_;
        }
    }

    // Try to create from factory
    auto logger = create_default_from_factory();
    if (logger) {
        return logger;
    }

    // Return NullLogger as fallback
    return null_logger();
}

inline VoidResult GlobalLoggerRegistry::set_default_logger(std::shared_ptr<ILogger> logger) {
    if (!logger) {
        return make_error<std::monostate>(
            error_codes::INVALID_ARGUMENT,
            "Default logger instance cannot be null",
            "interfaces::GlobalLoggerRegistry"
        );
    }

    std::unique_lock lock(mutex_);
    default_logger_ = std::move(logger);
    // Clear factory since we have a concrete instance
    default_factory_ = nullptr;

    return VoidResult::ok({});
}

inline VoidResult GlobalLoggerRegistry::register_factory(
    const std::string& name,
    LoggerFactory factory) {

    if (name.empty()) {
        return make_error<std::monostate>(
            error_codes::INVALID_ARGUMENT,
            "Logger name cannot be empty",
            "interfaces::GlobalLoggerRegistry"
        );
    }

    if (!factory) {
        return make_error<std::monostate>(
            error_codes::INVALID_ARGUMENT,
            "Factory function cannot be null",
            "interfaces::GlobalLoggerRegistry"
        );
    }

    std::unique_lock lock(mutex_);

    // Only register factory if no logger already exists
    if (loggers_.find(name) != loggers_.end()) {
        return make_error<std::monostate>(
            error_codes::ALREADY_EXISTS,
            "Logger already registered with name: " + name,
            "interfaces::GlobalLoggerRegistry"
        );
    }

    factories_[name] = std::move(factory);
    return VoidResult::ok({});
}

inline VoidResult GlobalLoggerRegistry::set_default_factory(LoggerFactory factory) {
    if (!factory) {
        return make_error<std::monostate>(
            error_codes::INVALID_ARGUMENT,
            "Factory function cannot be null",
            "interfaces::GlobalLoggerRegistry"
        );
    }

    std::unique_lock lock(mutex_);

    // Only set factory if no default logger exists
    if (default_logger_) {
        return make_error<std::monostate>(
            error_codes::ALREADY_EXISTS,
            "Default logger already registered",
            "interfaces::GlobalLoggerRegistry"
        );
    }

    default_factory_ = std::move(factory);
    return VoidResult::ok({});
}

inline bool GlobalLoggerRegistry::has_logger(const std::string& name) const {
    std::shared_lock lock(mutex_);
    return loggers_.find(name) != loggers_.end() ||
           factories_.find(name) != factories_.end();
}

inline bool GlobalLoggerRegistry::has_default_logger() const {
    std::shared_lock lock(mutex_);
    return default_logger_ != nullptr || default_factory_ != nullptr;
}

inline void GlobalLoggerRegistry::clear() {
    std::unique_lock lock(mutex_);
    loggers_.clear();
    factories_.clear();
    default_logger_.reset();
    default_factory_ = nullptr;
}

inline size_t GlobalLoggerRegistry::size() const {
    std::shared_lock lock(mutex_);
    return loggers_.size() + factories_.size();
}

inline std::shared_ptr<ILogger> GlobalLoggerRegistry::create_from_factory(
    const std::string& name) {

    std::unique_lock lock(mutex_);

    // Check again under write lock
    auto logger_it = loggers_.find(name);
    if (logger_it != loggers_.end()) {
        return logger_it->second;
    }

    auto factory_it = factories_.find(name);
    if (factory_it == factories_.end()) {
        return nullptr;
    }

    // Create logger from factory
    auto logger = factory_it->second();
    if (logger) {
        loggers_[name] = logger;
        factories_.erase(factory_it);
    }

    return logger;
}

inline std::shared_ptr<ILogger> GlobalLoggerRegistry::create_default_from_factory() {
    std::unique_lock lock(mutex_);

    // Check again under write lock
    if (default_logger_) {
        return default_logger_;
    }

    if (!default_factory_) {
        return nullptr;
    }

    // Create logger from factory
    auto logger = default_factory_();
    if (logger) {
        default_logger_ = logger;
        default_factory_ = nullptr;
    }

    return logger;
}

// ============================================================================
// Convenience Functions
// ============================================================================

/**
 * @brief Get the global logger registry instance.
 *
 * Convenience function to access the GlobalLoggerRegistry singleton.
 *
 * @return Reference to the GlobalLoggerRegistry
 */
inline GlobalLoggerRegistry& get_registry() {
    return GlobalLoggerRegistry::instance();
}

/**
 * @brief Get the default logger from the global registry.
 *
 * Convenience function to get the default logger. Returns NullLogger
 * if no default logger is registered.
 *
 * @return Default logger or NullLogger
 */
inline std::shared_ptr<ILogger> get_logger() {
    return GlobalLoggerRegistry::instance().get_default_logger();
}

/**
 * @brief Get a named logger from the global registry.
 *
 * Convenience function to get a named logger. Returns NullLogger
 * if the specified logger is not registered.
 *
 * @param name Logger name
 * @return Named logger or NullLogger
 */
inline std::shared_ptr<ILogger> get_logger(const std::string& name) {
    return GlobalLoggerRegistry::instance().get_logger(name);
}

} // namespace interfaces
} // namespace kcenon::common
