// BSD 3-Clause License
// Copyright (c) 2025, kcenon
// See the LICENSE file in the project root for full license information.

/**
 * @file service.h
 * @brief C++20 concepts for dependency injection and service container.
 *
 * This header provides concepts for validating service interfaces,
 * implementations, and factories used in the dependency injection system.
 * These concepts enable compile-time validation with clear error messages.
 *
 * Requirements:
 * - C++20 compiler with concepts support
 * - GCC 10+, Clang 10+, MSVC 2022+
 *
 * Thread Safety:
 * - Concepts are evaluated at compile-time only.
 * - No runtime thread-safety considerations apply.
 *
 * @see service_container_interface.h for IServiceContainer definition
 * @see service_container.h for service_container implementation
 * @see https://en.cppreference.com/w/cpp/language/constraints
 */

#pragma once

#include <concepts>
#include <type_traits>
#include <memory>
#include <functional>

namespace kcenon::common {

// Forward declarations
namespace di {
class IServiceContainer;
}

namespace concepts {

/**
 * @concept ServiceInterface
 * @brief A type that can be used as a service interface.
 *
 * Service interfaces must be polymorphic (have virtual methods) and
 * have a virtual destructor to ensure proper cleanup through base pointers.
 *
 * Example usage:
 * @code
 * template<ServiceInterface T>
 * void register_service(IServiceContainer& container) {
 *     container.register_type<T, ConcreteT>();
 * }
 * @endcode
 */
template<typename T>
concept ServiceInterface = std::is_polymorphic_v<T> &&
    std::has_virtual_destructor_v<T>;

/**
 * @concept ServiceImplementation
 * @brief A type that implements a service interface.
 *
 * Service implementations must derive from the interface and be
 * default-constructible for automatic instantiation by the container.
 *
 * Example usage:
 * @code
 * template<ServiceInterface TInterface, ServiceImplementation<TInterface> TImpl>
 * void register(IServiceContainer& container) {
 *     container.register_type<TInterface, TImpl>();
 * }
 * @endcode
 */
template<typename TImpl, typename TInterface>
concept ServiceImplementation =
    ServiceInterface<TInterface> &&
    std::is_base_of_v<TInterface, TImpl> &&
    std::is_default_constructible_v<TImpl>;

/**
 * @concept ServiceFactory
 * @brief A callable that creates service instances.
 *
 * Service factories receive a reference to the container (for resolving
 * dependencies) and return a shared_ptr to the service instance.
 *
 * Example usage:
 * @code
 * template<ServiceInterface T, ServiceFactory<T> F>
 * void register_factory(IServiceContainer& container, F&& factory) {
 *     container.register_factory<T>(std::forward<F>(factory));
 * }
 * @endcode
 */
template<typename F, typename T>
concept ServiceFactory = std::invocable<F, di::IServiceContainer&> &&
    std::convertible_to<std::invoke_result_t<F, di::IServiceContainer&>,
                        std::shared_ptr<T>>;

/**
 * @concept SimpleServiceFactory
 * @brief A callable that creates service instances without container access.
 *
 * Simple factories don't need container access for dependency resolution.
 * They're useful for services without dependencies or with external dependencies.
 *
 * Example usage:
 * @code
 * template<ServiceInterface T, SimpleServiceFactory<T> F>
 * void register_simple(IServiceContainer& container, F&& factory) {
 *     container.register_simple_factory<T>(std::forward<F>(factory));
 * }
 * @endcode
 */
template<typename F, typename T>
concept SimpleServiceFactory = std::invocable<F> &&
    std::convertible_to<std::invoke_result_t<F>, std::shared_ptr<T>>;

/**
 * @concept ServiceContainerLike
 * @brief A type that satisfies service container interface requirements.
 *
 * Types satisfying this concept provide service registration and resolution
 * capabilities with scope management.
 *
 * Example usage:
 * @code
 * template<ServiceContainerLike C>
 * void setup_services(C& container) {
 *     container.register_type<ILogger, ConsoleLogger>();
 * }
 * @endcode
 */
template<typename T>
concept ServiceContainerLike = requires(T t) {
    { t.create_scope() };
    { t.registered_services() };
    { t.clear() } -> std::same_as<void>;
};

/**
 * @concept ServiceScopeLike
 * @brief A type that represents a service scope.
 *
 * Service scopes inherit from a parent container and maintain their own
 * instances for scoped services.
 *
 * Example usage:
 * @code
 * template<ServiceScopeLike S>
 * void use_scope(S& scope) {
 *     auto& parent = scope.parent();
 *     // Use scoped services
 * }
 * @endcode
 */
template<typename T>
concept ServiceScopeLike = ServiceContainerLike<T> && requires(T t) {
    { t.parent() };
};

/**
 * @concept InjectableService
 * @brief A service that can be automatically injected.
 *
 * Injectable services must be default-constructible or have a
 * constructor that accepts container-resolvable dependencies.
 *
 * Example usage:
 * @code
 * template<InjectableService T>
 * auto create_service() {
 *     return std::make_shared<T>();
 * }
 * @endcode
 */
template<typename T>
concept InjectableService = std::is_class_v<T> &&
    std::is_default_constructible_v<T>;

/**
 * @concept SharedService
 * @brief A type that can be shared via shared_ptr.
 *
 * This concept ensures the type is suitable for shared ownership,
 * which is required for service container storage.
 *
 * Example usage:
 * @code
 * template<SharedService T>
 * void store(std::shared_ptr<T> service) {
 *     services_.push_back(service);
 * }
 * @endcode
 */
template<typename T>
concept SharedService = std::is_class_v<T> &&
    requires { typename std::shared_ptr<T>; };

/**
 * @concept ConfigSection
 * @brief A serializable configuration section type.
 *
 * Configuration sections must be default and copy constructible
 * for storage and manipulation by configuration systems.
 *
 * Example usage:
 * @code
 * template<ConfigSection T>
 * T load_config(const std::string& section_name) {
 *     T config{};
 *     // Load configuration values
 *     return config;
 * }
 * @endcode
 */
template<typename T>
concept ConfigSection =
    std::is_default_constructible_v<T> &&
    std::is_copy_constructible_v<T>;

/**
 * @concept Validatable
 * @brief A type that can validate its own state.
 *
 * Validatable types provide a validate() method that checks internal
 * consistency and returns a Result indicating success or validation errors.
 *
 * Example usage:
 * @code
 * template<Validatable T>
 * bool is_valid(const T& obj) {
 *     auto result = obj.validate();
 *     return result.is_ok();
 * }
 * @endcode
 */
template<typename T>
concept Validatable = requires(const T t) {
    { t.validate() };
};

/**
 * @concept ServiceWithDependencies
 * @brief A service that declares its dependencies.
 *
 * Services satisfying this concept provide a static method or type
 * alias to declare their dependency types for automatic resolution.
 *
 * Example usage:
 * @code
 * template<ServiceWithDependencies T>
 * void register_with_deps(IServiceContainer& container) {
 *     // Automatically resolve and inject dependencies
 * }
 * @endcode
 */
template<typename T>
concept ServiceWithDependencies = requires {
    typename T::dependencies;
};

/**
 * @concept InitializableService
 * @brief A service that requires explicit initialization.
 *
 * Services satisfying this concept need an initialize() call after
 * construction to complete setup.
 *
 * Example usage:
 * @code
 * template<InitializableService T>
 * auto create_and_init() {
 *     auto service = std::make_shared<T>();
 *     auto result = service->initialize();
 *     if (result.is_err()) {
 *         throw std::runtime_error("Initialization failed");
 *     }
 *     return service;
 * }
 * @endcode
 */
template<typename T>
concept InitializableService = requires(T t) {
    { t.initialize() };
};

/**
 * @concept DisposableService
 * @brief A service that requires explicit cleanup.
 *
 * Services satisfying this concept need a dispose() call before
 * destruction to release resources properly.
 *
 * Example usage:
 * @code
 * template<DisposableService T>
 * void cleanup(T& service) {
 *     service.dispose();
 * }
 * @endcode
 */
template<typename T>
concept DisposableService = requires(T t) {
    { t.dispose() } -> std::same_as<void>;
};

/**
 * @concept ModuleRegistrar
 * @brief A class-based module registrar for ecosystem DI integration.
 *
 * Module registrars provide a standardized way for subsystem modules
 * (logger_system, monitoring_system, etc.) to register their services
 * with the service container.
 *
 * Registrars must provide:
 * - A static module_name() returning the module identifier
 * - A register_services() method that registers services with the container
 *
 * Example usage:
 * @code
 * class LoggerModule {
 * public:
 *     static constexpr std::string_view module_name() { return "logger"; }
 *
 *     VoidResult register_services(IServiceContainer& container) {
 *         return container.register_factory<ILogger>(
 *             [](IServiceContainer&) { return std::make_shared<ConsoleLogger>(); },
 *             service_lifetime::singleton
 *         );
 *     }
 * };
 *
 * static_assert(ModuleRegistrar<LoggerModule>);
 * @endcode
 *
 * @see unified_bootstrapper.h for registration with the bootstrapper
 */
template<typename T>
concept ModuleRegistrar = requires(T registrar, di::IServiceContainer& container) {
    { T::module_name() } -> std::convertible_to<std::string_view>;
    { registrar.register_services(container) };
};

} // namespace concepts
} // namespace kcenon::common
