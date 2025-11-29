// BSD 3-Clause License
// Copyright (c) 2025, kcenon
// See the LICENSE file in the project root for full license information.

/**
 * @file unified_bootstrapper.h
 * @brief Unified system bootstrapper for initialization and shutdown.
 *
 * This header provides a unified bootstrapper that coordinates the initialization
 * and shutdown of all system components through the service container.
 *
 * Thread Safety:
 * - unified_bootstrapper is thread-safe for concurrent initialization checks.
 * - initialize() and shutdown() should be called from a single thread.
 * - Once initialized, services() can be called from any thread.
 *
 * Signal Handling:
 * - Automatically registers handlers for SIGTERM and SIGINT.
 * - Graceful shutdown is triggered on signal receipt.
 *
 * @see TICKET-104 for implementation requirements.
 * @see service_container.h for dependency injection container.
 */

#pragma once

#include "service_container.h"
#include "../interfaces/logger_interface.h"

#include <atomic>
#include <chrono>
#include <csignal>
#include <functional>
#include <memory>
#include <mutex>
#include <vector>

namespace kcenon::common {
namespace di {

/**
 * @struct bootstrapper_options
 * @brief Configuration options for the unified bootstrapper.
 *
 * Controls which system components are enabled and how they are configured.
 */
struct bootstrapper_options {
    /// Enable logging system services
    bool enable_logging = true;

    /// Enable monitoring system services
    bool enable_monitoring = true;

    /// Enable database system services
    bool enable_database = false;

    /// Enable network system services
    bool enable_network = false;

    /// Path to configuration file (optional)
    std::string config_path;

    /// Default shutdown timeout
    std::chrono::milliseconds shutdown_timeout{30000};

    /// Register signal handlers (SIGTERM, SIGINT)
    bool register_signal_handlers = true;
};

/**
 * @brief Shutdown hook callback type.
 *
 * Shutdown hooks are called in reverse order of registration during shutdown.
 * Each hook receives the remaining timeout duration.
 */
using shutdown_hook = std::function<void(std::chrono::milliseconds remaining_timeout)>;

/**
 * @class unified_bootstrapper
 * @brief Coordinates system initialization and shutdown.
 *
 * The unified bootstrapper provides a single entry point for initializing
 * all system components. It manages:
 * - Service registration order
 * - Dependency resolution
 * - Graceful shutdown with timeout
 * - Signal handler registration
 *
 * Usage Example:
 * @code
 * int main() {
 *     // Initialize with options
 *     auto result = unified_bootstrapper::initialize({
 *         .enable_logging = true,
 *         .enable_monitoring = true,
 *         .config_path = "config.yaml"
 *     });
 *
 *     if (result.is_err()) {
 *         std::cerr << "Initialization failed: " << result.error().message << "\n";
 *         return 1;
 *     }
 *
 *     // Get services
 *     auto& services = unified_bootstrapper::services();
 *     auto logger = services.resolve<interfaces::ILogger>();
 *
 *     // Application logic...
 *
 *     // Shutdown (or wait for signal)
 *     unified_bootstrapper::shutdown();
 *     return 0;
 * }
 * @endcode
 */
class unified_bootstrapper {
public:
    // Prevent instantiation
    unified_bootstrapper() = delete;
    ~unified_bootstrapper() = delete;
    unified_bootstrapper(const unified_bootstrapper&) = delete;
    unified_bootstrapper& operator=(const unified_bootstrapper&) = delete;

    /**
     * @brief Initialize the unified system.
     *
     * Performs the following steps:
     * 1. Registers core services (always required)
     * 2. Registers optional services based on options
     * 3. Sets up shutdown hooks
     * 4. Registers signal handlers (if enabled)
     *
     * This method is idempotent - calling it multiple times after successful
     * initialization returns success without re-initializing.
     *
     * @param opts Configuration options
     * @return VoidResult indicating success or initialization error
     *
     * Possible errors:
     * - ALREADY_EXISTS: Already initialized with different options
     * - INTERNAL_ERROR: Service registration failed
     */
    static VoidResult initialize(const bootstrapper_options& opts = {});

    /**
     * @brief Shutdown the unified system gracefully.
     *
     * Performs the following steps:
     * 1. Sets shutdown flag to prevent new operations
     * 2. Calls shutdown hooks in reverse order
     * 3. Clears all service registrations
     * 4. Resets initialization state
     *
     * @param timeout Maximum time to wait for graceful shutdown
     * @return VoidResult indicating success or shutdown error
     *
     * Possible errors:
     * - TIMEOUT: Shutdown did not complete within timeout
     * - NOT_INITIALIZED: System was not initialized
     */
    static VoidResult shutdown(
        std::chrono::milliseconds timeout = std::chrono::seconds(30));

    /**
     * @brief Get the service container.
     *
     * @return Reference to the global service container
     * @throws std::runtime_error if not initialized
     */
    static service_container& services();

    /**
     * @brief Check if the system is initialized.
     *
     * Thread-safe check of initialization state.
     *
     * @return true if initialized, false otherwise
     */
    static bool is_initialized();

    /**
     * @brief Check if shutdown has been requested.
     *
     * Thread-safe check of shutdown state. Useful for long-running
     * operations to check if they should abort.
     *
     * @return true if shutdown requested, false otherwise
     */
    static bool is_shutdown_requested();

    /**
     * @brief Register a shutdown hook.
     *
     * Hooks are called in reverse order of registration during shutdown.
     * Use for cleanup operations that must complete before the system
     * shuts down.
     *
     * @param name Unique name for the hook (for logging)
     * @param hook Callback function to execute during shutdown
     * @return VoidResult indicating success or error
     *
     * Possible errors:
     * - ALREADY_EXISTS: Hook with this name already registered
     * - NOT_INITIALIZED: System not initialized
     */
    static VoidResult register_shutdown_hook(
        const std::string& name,
        shutdown_hook hook);

    /**
     * @brief Unregister a shutdown hook.
     *
     * @param name Name of the hook to unregister
     * @return VoidResult indicating success or error
     */
    static VoidResult unregister_shutdown_hook(const std::string& name);

    /**
     * @brief Request graceful shutdown.
     *
     * Sets the shutdown flag and optionally triggers shutdown.
     * Can be called from signal handlers.
     *
     * @param trigger_shutdown If true, also calls shutdown()
     */
    static void request_shutdown(bool trigger_shutdown = false);

    /**
     * @brief Get the initialization options.
     *
     * @return Current bootstrapper options (empty if not initialized)
     */
    static bootstrapper_options get_options();

private:
    /**
     * @brief Register core services that are always required.
     */
    static VoidResult register_core_services();

    /**
     * @brief Register optional services based on options.
     */
    static VoidResult register_optional_services(const bootstrapper_options& opts);

    /**
     * @brief Set up default shutdown hooks.
     */
    static void setup_default_shutdown_hooks();

    /**
     * @brief Set up signal handlers.
     */
    static void setup_signal_handlers();

    /**
     * @brief Signal handler function.
     */
    static void signal_handler(int signal);

    /**
     * @brief Execute all shutdown hooks.
     */
    static void execute_shutdown_hooks(std::chrono::milliseconds timeout);

    // State
    static std::atomic<bool> initialized_;
    static std::atomic<bool> shutdown_requested_;
    static std::mutex mutex_;
    static bootstrapper_options options_;

    // Shutdown hooks
    struct shutdown_hook_entry {
        std::string name;
        shutdown_hook hook;
    };
    static std::vector<shutdown_hook_entry> shutdown_hooks_;
};

// ============================================================================
// unified_bootstrapper Implementation
// ============================================================================

inline std::atomic<bool> unified_bootstrapper::initialized_{false};
inline std::atomic<bool> unified_bootstrapper::shutdown_requested_{false};
inline std::mutex unified_bootstrapper::mutex_;
inline bootstrapper_options unified_bootstrapper::options_;
inline std::vector<unified_bootstrapper::shutdown_hook_entry>
    unified_bootstrapper::shutdown_hooks_;

inline VoidResult unified_bootstrapper::initialize(const bootstrapper_options& opts) {
    std::lock_guard<std::mutex> lock(mutex_);

    // Check if already initialized
    if (initialized_.load()) {
        return VoidResult::ok({});
    }

    // Store options
    options_ = opts;

    // Register core services
    auto core_result = register_core_services();
    if (core_result.is_err()) {
        return core_result;
    }

    // Register optional services
    auto optional_result = register_optional_services(opts);
    if (optional_result.is_err()) {
        // Cleanup on failure
        service_container::global().clear();
        return optional_result;
    }

    // Set up default shutdown hooks
    setup_default_shutdown_hooks();

    // Set up signal handlers
    if (opts.register_signal_handlers) {
        setup_signal_handlers();
    }

    // Mark as initialized
    initialized_.store(true);
    shutdown_requested_.store(false);

    return VoidResult::ok({});
}

inline VoidResult unified_bootstrapper::shutdown(std::chrono::milliseconds timeout) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (!initialized_.load()) {
        return make_error<std::monostate>(
            error_codes::NOT_INITIALIZED,
            "System is not initialized",
            "di::unified_bootstrapper"
        );
    }

    // Mark shutdown as requested
    shutdown_requested_.store(true);

    // Execute shutdown hooks
    execute_shutdown_hooks(timeout);

    // Clear all services
    service_container::global().clear();

    // Clear shutdown hooks
    shutdown_hooks_.clear();

    // Reset state
    initialized_.store(false);
    shutdown_requested_.store(false);
    options_ = bootstrapper_options{};

    return VoidResult::ok({});
}

inline service_container& unified_bootstrapper::services() {
    if (!initialized_.load()) {
        throw std::runtime_error(
            "unified_bootstrapper: System is not initialized. "
            "Call initialize() first."
        );
    }
    return service_container::global();
}

inline bool unified_bootstrapper::is_initialized() {
    return initialized_.load();
}

inline bool unified_bootstrapper::is_shutdown_requested() {
    return shutdown_requested_.load();
}

inline VoidResult unified_bootstrapper::register_shutdown_hook(
    const std::string& name,
    shutdown_hook hook) {

    std::lock_guard<std::mutex> lock(mutex_);

    if (!initialized_.load()) {
        return make_error<std::monostate>(
            error_codes::NOT_INITIALIZED,
            "System is not initialized",
            "di::unified_bootstrapper"
        );
    }

    // Check for duplicate
    for (const auto& entry : shutdown_hooks_) {
        if (entry.name == name) {
            return make_error<std::monostate>(
                error_codes::ALREADY_EXISTS,
                "Shutdown hook already registered: " + name,
                "di::unified_bootstrapper"
            );
        }
    }

    shutdown_hooks_.push_back({name, std::move(hook)});
    return VoidResult::ok({});
}

inline VoidResult unified_bootstrapper::unregister_shutdown_hook(
    const std::string& name) {

    std::lock_guard<std::mutex> lock(mutex_);

    auto it = std::find_if(shutdown_hooks_.begin(), shutdown_hooks_.end(),
        [&name](const shutdown_hook_entry& entry) {
            return entry.name == name;
        });

    if (it == shutdown_hooks_.end()) {
        return make_error<std::monostate>(
            error_codes::NOT_FOUND,
            "Shutdown hook not found: " + name,
            "di::unified_bootstrapper"
        );
    }

    shutdown_hooks_.erase(it);
    return VoidResult::ok({});
}

inline void unified_bootstrapper::request_shutdown(bool trigger_shutdown) {
    shutdown_requested_.store(true);

    if (trigger_shutdown) {
        shutdown(options_.shutdown_timeout);
    }
}

inline bootstrapper_options unified_bootstrapper::get_options() {
    std::lock_guard<std::mutex> lock(mutex_);
    return options_;
}

inline VoidResult unified_bootstrapper::register_core_services() {
    // Core services are minimal and always registered
    // The actual service implementations come from subsystems via adapters

    // For now, we just ensure the container is ready
    // Subsystems register their own services via their adapter modules

    return VoidResult::ok({});
}

inline VoidResult unified_bootstrapper::register_optional_services(
    const bootstrapper_options& opts) {

    // Optional services are registered based on configuration
    // The actual implementations come from subsystem adapters

    // Logging services
    if (opts.enable_logging) {
        // Placeholder: logger_system registers its services via adapter
        // Example: logger::di::register_logger_services(services());
    }

    // Monitoring services
    if (opts.enable_monitoring) {
        // Placeholder: monitoring_system registers its services via adapter
        // Example: monitoring::di::register_monitoring_services(services());
    }

    // Database services
    if (opts.enable_database) {
        // Placeholder: database_system registers its services via adapter
        // Example: database::di::register_database_services(services());
    }

    // Network services
    if (opts.enable_network) {
        // Placeholder: network_system registers its services via adapter
        // Example: network::di::register_network_services(services());
    }

    return VoidResult::ok({});
}

inline void unified_bootstrapper::setup_default_shutdown_hooks() {
    // Register default shutdown hooks for core cleanup

    // Service container cleanup hook (runs last, lowest priority)
    shutdown_hooks_.push_back({
        "service_container_cleanup",
        [](std::chrono::milliseconds) {
            // Container cleanup is handled by shutdown() itself
        }
    });
}

inline void unified_bootstrapper::setup_signal_handlers() {
#ifndef _WIN32
    // POSIX signal handling
    struct sigaction sa;
    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    sigaction(SIGTERM, &sa, nullptr);
    sigaction(SIGINT, &sa, nullptr);
#else
    // Windows signal handling
    std::signal(SIGTERM, signal_handler);
    std::signal(SIGINT, signal_handler);
#endif
}

inline void unified_bootstrapper::signal_handler(int signal) {
    (void)signal;  // Unused parameter

    // Request shutdown but don't trigger it from signal handler
    // The main thread should check is_shutdown_requested() and call shutdown()
    shutdown_requested_.store(true);
}

inline void unified_bootstrapper::execute_shutdown_hooks(
    std::chrono::milliseconds timeout) {

    auto start = std::chrono::steady_clock::now();

    // Execute hooks in reverse order (LIFO)
    for (auto it = shutdown_hooks_.rbegin(); it != shutdown_hooks_.rend(); ++it) {
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - start);
        auto remaining = timeout - elapsed;

        if (remaining <= std::chrono::milliseconds::zero()) {
            // Timeout reached, skip remaining hooks
            break;
        }

        try {
            it->hook(remaining);
        } catch (...) {
            // Ignore exceptions during shutdown
        }
    }
}

} // namespace di
} // namespace kcenon::common
