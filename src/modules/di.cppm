// BSD 3-Clause License
// Copyright (c) 2025, kcenon
// See the LICENSE file in the project root for full license information.

/**
 * @file di.cppm
 * @brief C++20 module partition for dependency injection.
 *
 * This module partition exports DI-related types:
 * - IServiceContainer: Service container interface
 * - ServiceContainer: Simple implementation
 *
 * Part of the kcenon.common module.
 */

module;

#include <any>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <typeindex>
#include <typeinfo>
#include <unordered_map>

export module kcenon.common:di;

export namespace kcenon::common::di {

// ============================================================================
// Service Container Interface
// ============================================================================

/**
 * @class IServiceContainer
 * @brief Interface for dependency injection containers.
 */
class IServiceContainer {
public:
    virtual ~IServiceContainer() = default;

    /**
     * @brief Register a service with a factory function.
     */
    template<typename Interface, typename Factory>
    void register_factory(Factory&& factory);

    /**
     * @brief Register a singleton instance.
     */
    template<typename Interface>
    void register_singleton(std::shared_ptr<Interface> instance);

    /**
     * @brief Resolve a service by interface type.
     */
    template<typename Interface>
    std::shared_ptr<Interface> resolve();

    /**
     * @brief Check if a service is registered.
     */
    template<typename Interface>
    bool has() const;

protected:
    virtual void register_impl(std::type_index type, std::any factory) = 0;
    virtual void register_singleton_impl(std::type_index type, std::any instance) = 0;
    virtual std::any resolve_impl(std::type_index type) = 0;
    virtual bool has_impl(std::type_index type) const = 0;
};

template<typename Interface, typename Factory>
void IServiceContainer::register_factory(Factory&& factory) {
    register_impl(std::type_index(typeid(Interface)),
                  std::function<std::shared_ptr<Interface>()>(std::forward<Factory>(factory)));
}

template<typename Interface>
void IServiceContainer::register_singleton(std::shared_ptr<Interface> instance) {
    register_singleton_impl(std::type_index(typeid(Interface)), std::any(instance));
}

template<typename Interface>
std::shared_ptr<Interface> IServiceContainer::resolve() {
    auto result = resolve_impl(std::type_index(typeid(Interface)));
    if (result.has_value()) {
        try {
            // Try singleton first
            return std::any_cast<std::shared_ptr<Interface>>(result);
        } catch (const std::bad_any_cast&) {
            // Try factory
            auto factory = std::any_cast<std::function<std::shared_ptr<Interface>()>>(result);
            return factory();
        }
    }
    return nullptr;
}

template<typename Interface>
bool IServiceContainer::has() const {
    return has_impl(std::type_index(typeid(Interface)));
}

// ============================================================================
// Service Container Implementation
// ============================================================================

/**
 * @class ServiceContainer
 * @brief Simple thread-safe service container implementation.
 */
class ServiceContainer : public IServiceContainer {
public:
    ServiceContainer() = default;

protected:
    void register_impl(std::type_index type, std::any factory) override {
        std::lock_guard<std::mutex> lock(mutex_);
        factories_[type] = std::move(factory);
    }

    void register_singleton_impl(std::type_index type, std::any instance) override {
        std::lock_guard<std::mutex> lock(mutex_);
        singletons_[type] = std::move(instance);
    }

    std::any resolve_impl(std::type_index type) override {
        std::lock_guard<std::mutex> lock(mutex_);

        // Check singletons first
        auto singleton_it = singletons_.find(type);
        if (singleton_it != singletons_.end()) {
            return singleton_it->second;
        }

        // Check factories
        auto factory_it = factories_.find(type);
        if (factory_it != factories_.end()) {
            return factory_it->second;
        }

        return std::any{};
    }

    bool has_impl(std::type_index type) const override {
        std::lock_guard<std::mutex> lock(mutex_);
        return singletons_.count(type) > 0 || factories_.count(type) > 0;
    }

private:
    mutable std::mutex mutex_;
    std::unordered_map<std::type_index, std::any> factories_;
    std::unordered_map<std::type_index, std::any> singletons_;
};

// ============================================================================
// Bootstrapper Interface
// ============================================================================

/**
 * @class IBootstrapper
 * @brief Interface for system bootstrapping.
 */
class IBootstrapper {
public:
    virtual ~IBootstrapper() = default;

    /**
     * @brief Initialize the system.
     * @return true if successful
     */
    virtual bool initialize() = 0;

    /**
     * @brief Shutdown the system.
     */
    virtual void shutdown() = 0;

    /**
     * @brief Check if the system is initialized.
     */
    virtual bool is_initialized() const = 0;
};

} // namespace kcenon::common::di
