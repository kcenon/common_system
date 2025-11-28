// BSD 3-Clause License
// Copyright (c) 2025, kcenon
// See the LICENSE file in the project root for full license information.

/**
 * @file service_container_interface.h
 * @brief Service container interfaces for dependency injection.
 *
 * This header defines the public interfaces for a dependency injection
 * container, enabling loosely coupled systems where services can be
 * registered, resolved, and managed with configurable lifetimes.
 *
 * Thread Safety:
 * - IServiceContainer implementations MUST be thread-safe for concurrent access.
 * - Service registration and resolution should use appropriate synchronization.
 * - Scoped containers inherit thread-safety requirements from parent.
 *
 * Circular Dependency Detection:
 * - Implementations MUST detect circular dependencies during resolution.
 * - When detected, resolve() should return an error with appropriate code.
 * - Detection should track resolution stack per-thread to avoid false positives.
 *
 * @see TICKET-101 for design rationale and requirements.
 */

#pragma once

#include <any>
#include <functional>
#include <memory>
#include <string>
#include <typeindex>
#include <vector>

#include "../patterns/result.h"

namespace kcenon::common {
namespace di {

/**
 * @enum service_lifetime
 * @brief Defines the lifetime policy for registered services.
 *
 * The lifetime determines how instances are created and cached:
 * - singleton: One instance shared across the entire application
 * - transient: New instance created for each resolution request
 * - scoped: One instance per scope (useful for request-scoped services)
 */
enum class service_lifetime {
    /**
     * @brief Single instance shared globally.
     *
     * The container creates one instance on first resolution and returns
     * the same instance for all subsequent requests. The instance lives
     * until the container is destroyed.
     *
     * Use for: Stateless services, expensive-to-create services, services
     * that maintain global state (loggers, configuration, etc.)
     */
    singleton,

    /**
     * @brief New instance created for each request.
     *
     * The container creates a new instance every time the service is resolved.
     * Each instance is independent and caller owns its lifetime.
     *
     * Use for: Stateful services where each consumer needs its own instance,
     * services with short lifespans, or services that cannot be shared safely.
     */
    transient,

    /**
     * @brief Single instance within a scope.
     *
     * Similar to singleton, but scoped to a particular IServiceScope.
     * Each scope gets its own instance, which is shared within that scope
     * but separate from instances in other scopes.
     *
     * Use for: Request-scoped services (web handlers), unit-of-work patterns,
     * services that should be isolated per logical operation.
     */
    scoped
};

/**
 * @brief Convert service_lifetime to string representation.
 * @param lifetime The lifetime value to convert
 * @return Human-readable string
 */
inline const char* to_string(service_lifetime lifetime) {
    switch (lifetime) {
        case service_lifetime::singleton: return "singleton";
        case service_lifetime::transient: return "transient";
        case service_lifetime::scoped: return "scoped";
        default: return "unknown";
    }
}

/**
 * @struct service_descriptor
 * @brief Metadata describing a registered service.
 *
 * Contains information about how a service is registered and can be resolved.
 * Used for introspection and debugging purposes.
 */
struct service_descriptor {
    /// Type index of the interface being registered
    std::type_index interface_type;

    /// Human-readable name of the interface type
    std::string interface_name;

    /// Lifetime policy for this service
    service_lifetime lifetime;

    /// Whether this service has been instantiated (for singletons)
    bool is_instantiated = false;

    /// Optional description or tags for the service
    std::string description;

    service_descriptor(std::type_index type, std::string name, service_lifetime lt)
        : interface_type(type)
        , interface_name(std::move(name))
        , lifetime(lt) {}
};

// Forward declaration
class IServiceScope;

/**
 * @interface IServiceContainer
 * @brief Abstract interface for dependency injection containers.
 *
 * IServiceContainer provides a type-safe mechanism for registering and resolving
 * services with configurable lifetimes. It supports:
 * - Type-based service registration and resolution
 * - Factory-based lazy instantiation
 * - Instance registration for pre-created objects
 * - Scoped containers for request-level isolation
 *
 * Thread Safety Requirements:
 * - All public methods MUST be thread-safe for concurrent access
 * - Implementations should use shared_mutex or similar for read/write locking
 * - Singleton instantiation must use double-checked locking or equivalent
 *
 * Error Handling:
 * - Registration failures return VoidResult with error details
 * - Resolution failures return Result<shared_ptr<T>> with error details
 * - Circular dependency detection returns error with cycle information
 *
 * Usage Example:
 * @code
 * auto& container = service_container::global();
 *
 * // Register services
 * container.register_factory<ILogger>(
 *     []() { return std::make_shared<ConsoleLogger>(); },
 *     service_lifetime::singleton
 * );
 *
 * container.register_type<IDatabase, PostgresDatabase>(
 *     service_lifetime::scoped
 * );
 *
 * // Resolve services
 * auto logger_result = container.resolve<ILogger>();
 * if (logger_result.is_ok()) {
 *     auto logger = logger_result.value();
 *     logger->info("Service resolved successfully");
 * }
 *
 * // Create scoped container
 * auto scope = container.create_scope();
 * auto db = scope->resolve<IDatabase>().value();
 * // db instance is unique to this scope
 * @endcode
 */
class IServiceContainer {
public:
    virtual ~IServiceContainer() = default;

    // ===== Service Registration (Type-safe templates) =====

    /**
     * @brief Register a service type with its implementation.
     *
     * Creates a factory that constructs TImpl instances when TInterface
     * is resolved. TImpl must be constructible with no arguments or
     * with dependencies that are also registered in the container.
     *
     * @tparam TInterface The interface type to register
     * @tparam TImpl The implementation type (must derive from TInterface)
     * @param lifetime Lifetime policy for the service
     * @return VoidResult indicating success or registration error
     *
     * @note TImpl must have a default constructor or use register_factory
     *       for types requiring constructor arguments.
     */
    template<typename TInterface, typename TImpl>
    VoidResult register_type(service_lifetime lifetime = service_lifetime::singleton) {
        static_assert(std::is_base_of_v<TInterface, TImpl>,
                      "TImpl must derive from TInterface");

        return register_factory_internal(
            std::type_index(typeid(TInterface)),
            typeid(TInterface).name(),
            [](IServiceContainer&) -> std::shared_ptr<void> {
                return std::make_shared<TImpl>();
            },
            lifetime
        );
    }

    /**
     * @brief Register a pre-existing instance as a singleton.
     *
     * The provided instance will be returned for all resolution requests.
     * The container takes shared ownership of the instance.
     *
     * @tparam TInterface The interface type to register
     * @param instance The instance to register
     * @return VoidResult indicating success or registration error
     */
    template<typename TInterface>
    VoidResult register_instance(std::shared_ptr<TInterface> instance) {
        if (!instance) {
            return make_error<std::monostate>(
                error_codes::INVALID_ARGUMENT,
                "Cannot register null instance",
                "di::IServiceContainer"
            );
        }

        return register_instance_internal(
            std::type_index(typeid(TInterface)),
            typeid(TInterface).name(),
            std::static_pointer_cast<void>(instance)
        );
    }

    /**
     * @brief Register a factory function for creating service instances.
     *
     * The factory is called each time the service needs to be instantiated
     * (once for singleton, each time for transient, once per scope for scoped).
     * The factory receives a reference to the container for resolving dependencies.
     *
     * @tparam TInterface The interface type to register
     * @tparam TFactory Callable returning shared_ptr<TInterface>
     * @param factory Factory function: (IServiceContainer&) -> shared_ptr<TInterface>
     * @param lifetime Lifetime policy for the service
     * @return VoidResult indicating success or registration error
     */
    template<typename TInterface, typename TFactory>
    VoidResult register_factory(TFactory&& factory,
                                service_lifetime lifetime = service_lifetime::singleton) {
        return register_factory_internal(
            std::type_index(typeid(TInterface)),
            typeid(TInterface).name(),
            [f = std::forward<TFactory>(factory)](IServiceContainer& c) -> std::shared_ptr<void> {
                return std::static_pointer_cast<void>(f(c));
            },
            lifetime
        );
    }

    /**
     * @brief Register a simple factory function without container access.
     *
     * Convenience overload for factories that don't need to resolve dependencies.
     *
     * @tparam TInterface The interface type to register
     * @tparam TFactory Callable returning shared_ptr<TInterface>
     * @param factory Factory function: () -> shared_ptr<TInterface>
     * @param lifetime Lifetime policy for the service
     * @return VoidResult indicating success or registration error
     */
    template<typename TInterface, typename TFactory>
    VoidResult register_simple_factory(TFactory&& factory,
                                       service_lifetime lifetime = service_lifetime::singleton) {
        return register_factory_internal(
            std::type_index(typeid(TInterface)),
            typeid(TInterface).name(),
            [f = std::forward<TFactory>(factory)](IServiceContainer&) -> std::shared_ptr<void> {
                return std::static_pointer_cast<void>(f());
            },
            lifetime
        );
    }

    // ===== Service Resolution =====

    /**
     * @brief Resolve a service by its interface type.
     *
     * Returns a shared pointer to the service instance. For singletons,
     * returns the cached instance. For transient, creates a new instance.
     * For scoped, returns the instance for the current scope.
     *
     * @tparam TInterface The interface type to resolve
     * @return Result containing the service or an error
     *
     * Possible errors:
     * - NOT_FOUND: Service not registered
     * - INTERNAL_ERROR: Circular dependency detected
     * - INTERNAL_ERROR: Factory threw an exception
     */
    template<typename TInterface>
    Result<std::shared_ptr<TInterface>> resolve() {
        auto result = resolve_internal(std::type_index(typeid(TInterface)));

        if (result.is_err()) {
            return make_error<std::shared_ptr<TInterface>>(result.error());
        }

        return Result<std::shared_ptr<TInterface>>::ok(
            std::static_pointer_cast<TInterface>(result.value())
        );
    }

    /**
     * @brief Try to resolve a service, returning nullptr if not found.
     *
     * Unlike resolve(), this method does not return an error for unregistered
     * services. Use when the service is optional.
     *
     * @tparam TInterface The interface type to resolve
     * @return Shared pointer to service or nullptr if not registered
     */
    template<typename TInterface>
    std::shared_ptr<TInterface> resolve_or_null() {
        auto result = resolve<TInterface>();
        if (result.is_ok()) {
            return result.value();
        }
        return nullptr;
    }

    // ===== Scope Management =====

    /**
     * @brief Create a new service scope.
     *
     * Returns a scoped container that shares singleton registrations with
     * the parent but maintains its own instances for scoped services.
     *
     * @return Unique pointer to the new scope
     */
    virtual std::unique_ptr<IServiceScope> create_scope() = 0;

    // ===== Introspection =====

    /**
     * @brief Check if a service type is registered.
     *
     * @tparam TInterface The interface type to check
     * @return true if registered, false otherwise
     */
    template<typename TInterface>
    bool is_registered() const {
        return is_registered_internal(std::type_index(typeid(TInterface)));
    }

    /**
     * @brief Get list of all registered service descriptors.
     *
     * @return Vector of service descriptors
     */
    virtual std::vector<service_descriptor> registered_services() const = 0;

    /**
     * @brief Unregister a service type.
     *
     * Removes the registration for the specified interface. Existing resolved
     * instances remain valid but no new instances can be resolved.
     *
     * @tparam TInterface The interface type to unregister
     * @return VoidResult indicating success or error
     */
    template<typename TInterface>
    VoidResult unregister() {
        return unregister_internal(std::type_index(typeid(TInterface)));
    }

    /**
     * @brief Clear all registrations.
     *
     * Removes all service registrations. Existing resolved instances remain
     * valid but no services can be resolved after this call.
     */
    virtual void clear() = 0;

protected:
    // Internal type-erased methods for implementation

    /**
     * @brief Internal factory registration (type-erased).
     */
    virtual VoidResult register_factory_internal(
        std::type_index interface_type,
        const std::string& type_name,
        std::function<std::shared_ptr<void>(IServiceContainer&)> factory,
        service_lifetime lifetime) = 0;

    /**
     * @brief Internal instance registration (type-erased).
     */
    virtual VoidResult register_instance_internal(
        std::type_index interface_type,
        const std::string& type_name,
        std::shared_ptr<void> instance) = 0;

    /**
     * @brief Internal service resolution (type-erased).
     */
    virtual Result<std::shared_ptr<void>> resolve_internal(
        std::type_index interface_type) = 0;

    /**
     * @brief Internal registration check (type-erased).
     */
    virtual bool is_registered_internal(std::type_index interface_type) const = 0;

    /**
     * @brief Internal unregistration (type-erased).
     */
    virtual VoidResult unregister_internal(std::type_index interface_type) = 0;
};

/**
 * @interface IServiceScope
 * @brief Scoped service container for request-level isolation.
 *
 * A service scope inherits all registrations from its parent container but
 * maintains its own instances for scoped services. When the scope is destroyed,
 * all scoped instances are disposed.
 *
 * Thread Safety:
 * - IServiceScope implementations should be thread-safe
 * - Typically used within a single request/thread context
 *
 * Usage:
 * @code
 * void handle_request(IServiceContainer& container) {
 *     auto scope = container.create_scope();
 *
 *     // Each request gets its own database connection
 *     auto db = scope->resolve<IDatabase>().value();
 *     auto db2 = scope->resolve<IDatabase>().value();
 *     // db == db2 (same scoped instance)
 *
 *     // Process request...
 * } // scope destroyed, scoped instances disposed
 * @endcode
 */
class IServiceScope : public IServiceContainer {
public:
    ~IServiceScope() override = default;

    /**
     * @brief Get the parent container.
     * @return Reference to the parent container
     */
    virtual IServiceContainer& parent() = 0;

    /**
     * @brief Get the parent container (const).
     * @return Const reference to the parent container
     */
    virtual const IServiceContainer& parent() const = 0;
};

/**
 * @brief Error codes specific to dependency injection.
 */
namespace di_error_codes {
    /// Service not registered in container
    constexpr int service_not_registered = -100;

    /// Circular dependency detected during resolution
    constexpr int circular_dependency = -101;

    /// Service already registered (duplicate registration attempt)
    constexpr int already_registered = -102;

    /// Factory threw an exception during instantiation
    constexpr int factory_error = -103;

    /// Invalid service lifetime configuration
    constexpr int invalid_lifetime = -104;

    /// Scoped service resolved from root container
    constexpr int scoped_from_root = -105;
}

} // namespace di
} // namespace kcenon::common
