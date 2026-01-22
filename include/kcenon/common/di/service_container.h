// BSD 3-Clause License
// Copyright (c) 2025, kcenon
// See the LICENSE file in the project root for full license information.

/**
 * @file service_container.h
 * @brief Implementation of the service container for dependency injection.
 *
 * This header provides the concrete implementation of IServiceContainer,
 * enabling type-safe dependency injection with configurable lifetimes.
 *
 * Thread Safety:
 * - service_container is thread-safe for concurrent registration and resolution.
 * - Uses std::shared_mutex for read/write locking.
 * - Singleton instantiation uses double-checked locking pattern.
 * - Circular dependency detection uses thread-local resolution stack.
 *
 * @see TICKET-102 for implementation requirements.
 * @see service_container_interface.h for the interface definition.
 */

#pragma once

#include "service_container_interface.h"
#include "../interfaces/registry_audit_log.h"

#include <atomic>
#include <mutex>
#include <shared_mutex>
#include <unordered_map>
#include <unordered_set>

namespace kcenon::common {
namespace di {

// Forward declaration
class service_scope;

/**
 * @class service_container
 * @brief Concrete implementation of IServiceContainer.
 *
 * Provides a thread-safe dependency injection container with support for:
 * - Singleton, transient, and scoped service lifetimes
 * - Factory-based lazy instantiation
 * - Circular dependency detection
 * - Scoped containers for request-level isolation
 *
 * Usage Example:
 * @code
 * auto& container = service_container::global();
 *
 * // Register a singleton logger
 * container.register_factory<ILogger>(
 *     [](IServiceContainer&) { return std::make_shared<ConsoleLogger>(); },
 *     service_lifetime::singleton
 * );
 *
 * // Register a transient service
 * container.register_type<IWorker, WorkerImpl>(service_lifetime::transient);
 *
 * // Resolve services
 * auto logger = container.resolve<ILogger>().value();
 * @endcode
 */
class service_container : public IServiceContainer {
public:
    /**
     * @brief Get the global service container instance.
     *
     * Returns a reference to the application-wide singleton container.
     * Thread-safe for concurrent access.
     *
     * @return Reference to the global service_container
     */
    static service_container& global();

    /**
     * @brief Default constructor.
     */
    service_container() = default;

    /**
     * @brief Destructor.
     */
    ~service_container() override = default;

    // Non-copyable, non-movable
    service_container(const service_container&) = delete;
    service_container& operator=(const service_container&) = delete;
    service_container(service_container&&) = delete;
    service_container& operator=(service_container&&) = delete;

    // ===== IServiceContainer Implementation =====

    /**
     * @brief Create a new service scope.
     * @return Unique pointer to the new scope
     */
    std::unique_ptr<IServiceScope> create_scope() override;

    /**
     * @brief Get list of all registered service descriptors.
     * @return Vector of service descriptors
     */
    std::vector<service_descriptor> registered_services() const override;

    /**
     * @brief Clear all registrations.
     */
    void clear() override;

    // ===== Security Controls =====

    /**
     * @brief Freeze the container to prevent further registrations.
     *
     * Once frozen, no new services can be registered or unregistered.
     * Existing services can still be resolved. This is a one-way operation
     * and cannot be undone.
     *
     * @note This should be called after system initialization to prevent
     *       unauthorized service replacement which could be used to inject
     *       malicious implementations.
     * @note This is a security feature to prevent service hijacking.
     *
     * @see Issue #206 for security requirements.
     */
    void freeze();

    /**
     * @brief Check if the container is frozen.
     *
     * @return true if the container is frozen and cannot be modified
     */
    bool is_frozen() const;

protected:
    // ===== Internal Implementation =====

    VoidResult register_factory_internal(
        std::type_index interface_type,
        const std::string& type_name,
        std::function<std::shared_ptr<void>(IServiceContainer&)> factory,
        service_lifetime lifetime) override;

    VoidResult register_instance_internal(
        std::type_index interface_type,
        const std::string& type_name,
        std::shared_ptr<void> instance) override;

    Result<std::shared_ptr<void>> resolve_internal(
        std::type_index interface_type) override;

    bool is_registered_internal(std::type_index interface_type) const override;

    VoidResult unregister_internal(std::type_index interface_type) override;

private:
    friend class service_scope;

    /**
     * @brief Internal service registration entry.
     */
    struct service_entry {
        std::type_index interface_type;
        std::string type_name;
        std::function<std::shared_ptr<void>(IServiceContainer&)> factory;
        service_lifetime lifetime;
        std::shared_ptr<void> singleton_instance;
        bool is_instantiated = false;

        service_entry(std::type_index type, std::string name,
                      std::function<std::shared_ptr<void>(IServiceContainer&)> f,
                      service_lifetime lt)
            : interface_type(type)
            , type_name(std::move(name))
            , factory(std::move(f))
            , lifetime(lt) {}
    };

    /**
     * @brief Resolve a service with circular dependency detection.
     *
     * @param interface_type The type to resolve
     * @param scoped_instances Optional map for scoped instances (from scope)
     * @return Result containing the service or an error
     */
    Result<std::shared_ptr<void>> resolve_with_detection(
        std::type_index interface_type,
        std::unordered_map<std::type_index, std::shared_ptr<void>>* scoped_instances = nullptr);

    /**
     * @brief Check if currently resolving this type (circular dependency check).
     */
    bool is_resolving(std::type_index interface_type) const;

    /**
     * @brief Push type onto resolution stack.
     */
    void push_resolution(std::type_index interface_type);

    /**
     * @brief Pop type from resolution stack.
     */
    void pop_resolution(std::type_index interface_type);

    /**
     * @brief Get current resolution stack as string for error messages.
     */
    std::string get_resolution_stack_string() const;

    // ===== Helper Methods for Reducing Code Duplication =====

    /**
     * @brief Check if container is frozen and log audit event if so.
     *
     * @param type_name The type name for audit logging
     * @param action The registry action being performed
     * @param error_message The error message to use if frozen
     * @return VoidResult - error if frozen, ok otherwise
     */
    VoidResult check_frozen_for_registration(
        const std::string& type_name,
        interfaces::registry_action action,
        const std::string& error_message) const;

    /**
     * @brief Check if a service is already registered and log audit event if so.
     *
     * @note Caller must hold the mutex lock before calling this method.
     *
     * @param interface_type The type to check
     * @param type_name The type name for audit logging
     * @return VoidResult - error if already registered, ok otherwise
     */
    VoidResult check_already_registered(
        std::type_index interface_type,
        const std::string& type_name) const;

    /**
     * @brief Safely invoke a factory function with exception handling.
     *
     * @param factory The factory function to invoke
     * @return Result containing the created instance or a factory_error
     */
    Result<std::shared_ptr<void>> invoke_factory_safe(
        const std::function<std::shared_ptr<void>(IServiceContainer&)>& factory);

    // Service registry
    mutable std::shared_mutex mutex_;
    std::unordered_map<std::type_index, service_entry> services_;

    // Security controls
    std::atomic<bool> frozen_{false};

    // Thread-local resolution stack for circular dependency detection
    static thread_local std::unordered_set<std::type_index> resolution_stack_;
    static thread_local std::vector<std::type_index> resolution_order_;
};

/**
 * @class service_scope
 * @brief Scoped service container implementation.
 *
 * A service scope shares singleton registrations with its parent container
 * but maintains its own instances for scoped services. When the scope is
 * destroyed, all scoped instances are disposed.
 *
 * Thread Safety:
 * - service_scope is thread-safe for concurrent resolution.
 * - Multiple threads can safely resolve services from the same scope.
 * - Uses std::shared_mutex for read/write locking of scoped instances.
 * - Recommended usage pattern: one scope per request/thread for best performance.
 *
 * @note While thread-safe, creating separate scopes per thread is recommended
 *       for optimal performance and natural isolation of scoped instances.
 */
class service_scope : public IServiceScope {
public:
    /**
     * @brief Construct a scope with a parent container.
     * @param parent Reference to the parent container
     */
    explicit service_scope(service_container& parent);

    /**
     * @brief Destructor - disposes scoped instances.
     */
    ~service_scope() override = default;

    // Non-copyable, non-movable
    service_scope(const service_scope&) = delete;
    service_scope& operator=(const service_scope&) = delete;
    service_scope(service_scope&&) = delete;
    service_scope& operator=(service_scope&&) = delete;

    // ===== IServiceScope Implementation =====

    /**
     * @brief Get the parent container.
     * @return Reference to the parent container
     */
    IServiceContainer& parent() override;

    /**
     * @brief Get the parent container (const).
     * @return Const reference to the parent container
     */
    const IServiceContainer& parent() const override;

    // ===== IServiceContainer Implementation =====

    std::unique_ptr<IServiceScope> create_scope() override;
    std::vector<service_descriptor> registered_services() const override;
    void clear() override;

protected:
    VoidResult register_factory_internal(
        std::type_index interface_type,
        const std::string& type_name,
        std::function<std::shared_ptr<void>(IServiceContainer&)> factory,
        service_lifetime lifetime) override;

    VoidResult register_instance_internal(
        std::type_index interface_type,
        const std::string& type_name,
        std::shared_ptr<void> instance) override;

    Result<std::shared_ptr<void>> resolve_internal(
        std::type_index interface_type) override;

    bool is_registered_internal(std::type_index interface_type) const override;

    VoidResult unregister_internal(std::type_index interface_type) override;

private:
    service_container& parent_;
    mutable std::shared_mutex mutex_;
    std::unordered_map<std::type_index, std::shared_ptr<void>> scoped_instances_;
};

// ============================================================================
// service_container Implementation
// ============================================================================

// Thread-local storage for circular dependency detection
inline thread_local std::unordered_set<std::type_index> service_container::resolution_stack_;
inline thread_local std::vector<std::type_index> service_container::resolution_order_;

inline service_container& service_container::global() {
    static service_container instance;
    return instance;
}

inline std::unique_ptr<IServiceScope> service_container::create_scope() {
    return std::make_unique<service_scope>(*this);
}

inline std::vector<service_descriptor> service_container::registered_services() const {
    std::shared_lock lock(mutex_);

    std::vector<service_descriptor> result;
    result.reserve(services_.size());

    for (const auto& [type_index, entry] : services_) {
        service_descriptor desc(entry.interface_type, entry.type_name, entry.lifetime);
        desc.is_instantiated = entry.is_instantiated;
        result.push_back(std::move(desc));
    }

    return result;
}

inline void service_container::clear() {
    if (is_frozen()) {
        interfaces::RegistryAuditLog::log_event(interfaces::registry_event(
            interfaces::registry_action::clear_services, "",
            interfaces::source_location::current(), false,
            "Container is frozen"));
        // Silently ignore clear when frozen to maintain API compatibility
        return;
    }

    std::unique_lock lock(mutex_);
    services_.clear();

    interfaces::RegistryAuditLog::log_event(interfaces::registry_event(
        interfaces::registry_action::clear_services, "",
        interfaces::source_location::current(), true));
}

inline void service_container::freeze() {
    frozen_.store(true, std::memory_order_release);

    interfaces::RegistryAuditLog::log_event(interfaces::registry_event(
        interfaces::registry_action::freeze_service_container, "",
        interfaces::source_location::current(), true));
}

inline bool service_container::is_frozen() const {
    return frozen_.load(std::memory_order_acquire);
}

// ============================================================================
// Helper Methods Implementation
// ============================================================================

inline VoidResult service_container::check_frozen_for_registration(
    const std::string& type_name,
    interfaces::registry_action action,
    const std::string& error_message) const {

    if (is_frozen()) {
        interfaces::RegistryAuditLog::log_event(interfaces::registry_event(
            action, type_name,
            interfaces::source_location::current(), false,
            "Container is frozen"));
        return make_error<std::monostate>(
            error_codes::REGISTRY_FROZEN,
            error_message,
            "di::service_container"
        );
    }

    return VoidResult::ok({});
}

inline VoidResult service_container::check_already_registered(
    std::type_index interface_type,
    const std::string& type_name) const {

    if (services_.find(interface_type) != services_.end()) {
        interfaces::RegistryAuditLog::log_event(interfaces::registry_event(
            interfaces::registry_action::register_service, type_name,
            interfaces::source_location::current(), false,
            "Service already registered"));
        return make_error<std::monostate>(
            di_error_codes::already_registered,
            "Service already registered: " + type_name,
            "di::service_container"
        );
    }

    return VoidResult::ok({});
}

inline Result<std::shared_ptr<void>> service_container::invoke_factory_safe(
    const std::function<std::shared_ptr<void>(IServiceContainer&)>& factory) {

    try {
        return Result<std::shared_ptr<void>>::ok(factory(*this));
    } catch (const std::exception& e) {
        return make_error<std::shared_ptr<void>>(
            di_error_codes::factory_error,
            std::string("Factory threw exception: ") + e.what(),
            "di::service_container"
        );
    }
}

inline VoidResult service_container::register_factory_internal(
    std::type_index interface_type,
    const std::string& type_name,
    std::function<std::shared_ptr<void>(IServiceContainer&)> factory,
    service_lifetime lifetime) {

    // Check frozen state before acquiring lock
    auto frozen_check = check_frozen_for_registration(
        type_name,
        interfaces::registry_action::register_service,
        "Cannot register service: container is frozen");
    if (!frozen_check.is_ok()) {
        return frozen_check;
    }

    std::unique_lock lock(mutex_);

    // Check if already registered (must hold lock)
    auto registered_check = check_already_registered(interface_type, type_name);
    if (!registered_check.is_ok()) {
        return registered_check;
    }

    services_.emplace(
        interface_type,
        service_entry(interface_type, type_name, std::move(factory), lifetime)
    );

    interfaces::RegistryAuditLog::log_event(interfaces::registry_event(
        interfaces::registry_action::register_service, type_name,
        interfaces::source_location::current(), true));

    return VoidResult::ok({});
}

inline VoidResult service_container::register_instance_internal(
    std::type_index interface_type,
    const std::string& type_name,
    std::shared_ptr<void> instance) {

    // Check frozen state before acquiring lock
    auto frozen_check = check_frozen_for_registration(
        type_name,
        interfaces::registry_action::register_service,
        "Cannot register instance: container is frozen");
    if (!frozen_check.is_ok()) {
        return frozen_check;
    }

    std::unique_lock lock(mutex_);

    // Check if already registered (must hold lock)
    auto registered_check = check_already_registered(interface_type, type_name);
    if (!registered_check.is_ok()) {
        return registered_check;
    }

    // Create entry with instance as singleton
    service_entry entry(
        interface_type,
        type_name,
        [](IServiceContainer&) -> std::shared_ptr<void> { return nullptr; },
        service_lifetime::singleton
    );
    entry.singleton_instance = std::move(instance);
    entry.is_instantiated = true;

    services_.emplace(interface_type, std::move(entry));

    interfaces::RegistryAuditLog::log_event(interfaces::registry_event(
        interfaces::registry_action::register_service, type_name,
        interfaces::source_location::current(), true));

    return VoidResult::ok({});
}

inline Result<std::shared_ptr<void>> service_container::resolve_internal(
    std::type_index interface_type) {
    return resolve_with_detection(interface_type, nullptr);
}

inline Result<std::shared_ptr<void>> service_container::resolve_with_detection(
    std::type_index interface_type,
    std::unordered_map<std::type_index, std::shared_ptr<void>>* scoped_instances) {

    // Check for circular dependency
    if (is_resolving(interface_type)) {
        std::string cycle_info = get_resolution_stack_string();
        return make_error<std::shared_ptr<void>>(
            di_error_codes::circular_dependency,
            "Circular dependency detected: " + cycle_info,
            "di::service_container"
        );
    }

    // Check if service is registered
    {
        std::shared_lock lock(mutex_);
        auto it = services_.find(interface_type);
        if (it == services_.end()) {
            return make_error<std::shared_ptr<void>>(
                di_error_codes::service_not_registered,
                "Service not registered: " + std::string(interface_type.name()),
                "di::service_container"
            );
        }
    }

    // Push onto resolution stack
    push_resolution(interface_type);

    // RAII guard to pop from resolution stack
    struct resolution_guard {
        service_container* container;
        std::type_index type;
        ~resolution_guard() { container->pop_resolution(type); }
    } guard{this, interface_type};

    // Resolve based on lifetime
    std::shared_lock read_lock(mutex_);
    auto it = services_.find(interface_type);
    auto& entry = it->second;

    // Copy factory and lifetime before releasing lock to avoid deadlock
    // when factory calls resolve() recursively
    auto factory_copy = entry.factory;
    auto lifetime = entry.lifetime;

    switch (lifetime) {
        case service_lifetime::singleton: {
            // Double-checked locking for singleton
            if (entry.is_instantiated) {
                return Result<std::shared_ptr<void>>::ok(entry.singleton_instance);
            }

            read_lock.unlock();

            // Create instance outside of lock to avoid deadlock
            auto factory_result = invoke_factory_safe(factory_copy);
            if (!factory_result.is_ok()) {
                return factory_result;
            }

            // Now acquire write lock to store the instance
            std::unique_lock write_lock(mutex_);

            // Re-check after acquiring write lock (another thread may have created it)
            auto it2 = services_.find(interface_type);
            auto& entry2 = it2->second;
            if (entry2.is_instantiated) {
                // Another thread created it, return that instance
                return Result<std::shared_ptr<void>>::ok(entry2.singleton_instance);
            }

            entry2.singleton_instance = std::move(factory_result.value());
            entry2.is_instantiated = true;
            return Result<std::shared_ptr<void>>::ok(entry2.singleton_instance);
        }

        case service_lifetime::transient: {
            // Create new instance each time - release lock before calling factory
            read_lock.unlock();
            return invoke_factory_safe(factory_copy);
        }

        case service_lifetime::scoped: {
            // Scoped services should be resolved from a scope
            if (scoped_instances == nullptr) {
                return make_error<std::shared_ptr<void>>(
                    di_error_codes::scoped_from_root,
                    "Cannot resolve scoped service from root container. Use create_scope().",
                    "di::service_container"
                );
            }

            // Check if already instantiated in scope
            auto scoped_it = scoped_instances->find(interface_type);
            if (scoped_it != scoped_instances->end()) {
                return Result<std::shared_ptr<void>>::ok(scoped_it->second);
            }

            // Create new instance for scope - release lock before calling factory
            read_lock.unlock();
            auto factory_result = invoke_factory_safe(factory_copy);
            if (!factory_result.is_ok()) {
                return factory_result;
            }

            auto instance = std::move(factory_result.value());
            (*scoped_instances)[interface_type] = instance;
            return Result<std::shared_ptr<void>>::ok(std::move(instance));
        }

        default:
            return make_error<std::shared_ptr<void>>(
                di_error_codes::invalid_lifetime,
                "Unknown service lifetime",
                "di::service_container"
            );
    }
}

inline bool service_container::is_registered_internal(std::type_index interface_type) const {
    std::shared_lock lock(mutex_);
    return services_.find(interface_type) != services_.end();
}

inline VoidResult service_container::unregister_internal(std::type_index interface_type) {
    std::string type_name = interface_type.name();

    if (is_frozen()) {
        interfaces::RegistryAuditLog::log_event(interfaces::registry_event(
            interfaces::registry_action::unregister_service, type_name,
            interfaces::source_location::current(), false,
            "Container is frozen"));
        return make_error<std::monostate>(
            error_codes::REGISTRY_FROZEN,
            "Cannot unregister service: container is frozen",
            "di::service_container"
        );
    }

    std::unique_lock lock(mutex_);

    auto it = services_.find(interface_type);
    if (it == services_.end()) {
        interfaces::RegistryAuditLog::log_event(interfaces::registry_event(
            interfaces::registry_action::unregister_service, type_name,
            interfaces::source_location::current(), false,
            "Service not registered"));
        return make_error<std::monostate>(
            di_error_codes::service_not_registered,
            "Service not registered",
            "di::service_container"
        );
    }

    services_.erase(it);

    interfaces::RegistryAuditLog::log_event(interfaces::registry_event(
        interfaces::registry_action::unregister_service, type_name,
        interfaces::source_location::current(), true));

    return VoidResult::ok({});
}

inline bool service_container::is_resolving(std::type_index interface_type) const {
    return resolution_stack_.find(interface_type) != resolution_stack_.end();
}

inline void service_container::push_resolution(std::type_index interface_type) {
    resolution_stack_.insert(interface_type);
    resolution_order_.push_back(interface_type);
}

inline void service_container::pop_resolution(std::type_index interface_type) {
    resolution_stack_.erase(interface_type);
    if (!resolution_order_.empty() && resolution_order_.back() == interface_type) {
        resolution_order_.pop_back();
    }
}

inline std::string service_container::get_resolution_stack_string() const {
    std::string result;
    for (size_t i = 0; i < resolution_order_.size(); ++i) {
        if (i > 0) {
            result += " -> ";
        }
        result += resolution_order_[i].name();
    }
    return result;
}

// ============================================================================
// service_scope Implementation
// ============================================================================

inline service_scope::service_scope(service_container& parent)
    : parent_(parent) {}

inline IServiceContainer& service_scope::parent() {
    return parent_;
}

inline const IServiceContainer& service_scope::parent() const {
    return parent_;
}

inline std::unique_ptr<IServiceScope> service_scope::create_scope() {
    // Nested scopes share the same root parent
    return std::make_unique<service_scope>(parent_);
}

inline std::vector<service_descriptor> service_scope::registered_services() const {
    // Return parent's registered services
    return parent_.registered_services();
}

inline void service_scope::clear() {
    std::unique_lock lock(mutex_);
    scoped_instances_.clear();
}

inline VoidResult service_scope::register_factory_internal(
    std::type_index interface_type,
    const std::string& type_name,
    std::function<std::shared_ptr<void>(IServiceContainer&)> factory,
    service_lifetime lifetime) {
    // Delegate to parent
    return parent_.register_factory_internal(interface_type, type_name, std::move(factory), lifetime);
}

inline VoidResult service_scope::register_instance_internal(
    std::type_index interface_type,
    const std::string& type_name,
    std::shared_ptr<void> instance) {
    // Delegate to parent
    return parent_.register_instance_internal(interface_type, type_name, std::move(instance));
}

inline Result<std::shared_ptr<void>> service_scope::resolve_internal(
    std::type_index interface_type) {
    // Protect scoped_instances_ for thread-safe concurrent resolution.
    // The mutex is held during the entire resolution to prevent data races
    // when multiple threads resolve the same scoped service simultaneously.
    std::unique_lock lock(mutex_);
    return parent_.resolve_with_detection(interface_type, &scoped_instances_);
}

inline bool service_scope::is_registered_internal(std::type_index interface_type) const {
    return parent_.is_registered_internal(interface_type);
}

inline VoidResult service_scope::unregister_internal(std::type_index interface_type) {
    // Delegate to parent
    return parent_.unregister_internal(interface_type);
}

} // namespace di
} // namespace kcenon::common
