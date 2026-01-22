/*
 * BSD 3-Clause License
 * Copyright (c) 2025, kcenon
 */

#pragma once

/**
 * @file smart_adapter.h
 * @brief Smart adapter factory (legacy)
 *
 * @note Consider using adapter_factory from adapter.h for new code:
 * - smart_adapter_factory -> adapter_factory
 * - make_smart_adapter -> make_interface_adapter
 * - unwrap_adapter -> adapter_factory::try_unwrap
 *
 * This file is maintained for backward compatibility.
 */

#include <memory>
#include <type_traits>

#include "typed_adapter.h"

namespace kcenon::common::adapters {

/**
 * @brief Type trait to check if a type implements an interface (C++17 compatible)
 * @tparam T The type to check
 * @tparam Interface The interface to check against
 */
template<typename T, typename Interface>
inline constexpr bool implements_interface_v = std::is_base_of_v<Interface, T> ||
                                                std::is_convertible_v<T*, Interface*>;

/**
 * @brief Smart adapter factory that avoids unnecessary wrapping
 *
 * This factory uses compile-time checks to determine if adaptation is needed:
 * - If the implementation already implements the interface, direct cast (zero-cost)
 * - Otherwise, create a typed_adapter wrapper
 *
 * This provides zero-cost abstraction when possible.
 *
 * @note Consider using adapter_factory from adapter.h for new code
 */
class smart_adapter_factory {
public:
    /**
     * @brief Create an adapter with automatic optimization
     *
     * If Impl already implements Interface, returns a static_pointer_cast (zero-cost).
     * Otherwise, creates a typed_adapter wrapper.
     *
     * @tparam Interface The target interface type
     * @tparam Impl The implementation type
     * @param impl Shared pointer to the implementation
     * @return Shared pointer to Interface (either direct cast or adapter)
     */
    template<typename Interface, typename Impl>
    static std::shared_ptr<Interface> make_adapter(std::shared_ptr<Impl> impl) {
        if constexpr (implements_interface_v<Impl, Interface>) {
            // Zero-cost: Direct cast, no wrapper needed
            return std::static_pointer_cast<Interface>(impl);
        } else {
            // Create adapter wrapper
            return std::make_shared<typed_adapter<Interface, Impl>>(impl);
        }
    }

    /**
     * @brief Create an adapter with explicit adapter type
     *
     * Always creates the specified adapter type, even if zero-cost cast is possible.
     * Use this when you need specific adapter behavior beyond simple interface conversion.
     *
     * @tparam AdapterType The specific adapter class to use
     * @tparam Args Constructor argument types
     * @param args Arguments to forward to adapter constructor
     * @return Shared pointer to the created adapter
     */
    template<typename AdapterType, typename... Args>
    static std::shared_ptr<AdapterType> make_explicit_adapter(Args&&... args) {
        return std::make_shared<AdapterType>(std::forward<Args>(args)...);
    }

    /**
     * @brief Try to unwrap an interface to get underlying implementation
     *
     * If the interface is a typed_adapter, unwraps it.
     * Otherwise, returns nullptr (strict type safety without RTTI).
     *
     * @tparam T The expected underlying type
     * @tparam Interface The interface type
     * @param ptr Shared pointer to the interface
     * @return Shared pointer to T, or nullptr if conversion fails
     */
    template<typename T, typename Interface>
    static std::shared_ptr<T> try_unwrap(std::shared_ptr<Interface> ptr) {
        if (!ptr) {
            return nullptr;
        }

        // Try safe_unwrap (for typed_adapter)
        if (auto unwrapped = safe_unwrap<T>(ptr)) {
            return unwrapped;
        }

        // Not an adapter - return nullptr for strict type safety
        // (removed dynamic_pointer_cast to eliminate RTTI dependency)
        return nullptr;
    }

    /**
     * @brief Check if zero-cost adaptation is possible
     * @tparam Interface The target interface
     * @tparam Impl The implementation type
     * @return true if direct cast is possible (zero-cost)
     */
    template<typename Interface, typename Impl>
    static constexpr bool is_zero_cost() {
        return implements_interface_v<Impl, Interface>;
    }
};

/**
 * @brief Convenience function for creating smart adapters
 *
 * @tparam Interface The target interface type
 * @tparam Impl The implementation type (deduced)
 * @param impl Shared pointer to implementation
 * @return Shared pointer to Interface
 *
 * @code
 * auto executor = make_smart_adapter<IExecutor>(thread_pool);
 * // If thread_pool implements IExecutor: zero-cost cast
 * // Otherwise: creates typed_adapter<IExecutor, thread_pool>
 * @endcode
 */
template<typename Interface, typename Impl>
std::shared_ptr<Interface> make_smart_adapter(std::shared_ptr<Impl> impl) {
    return smart_adapter_factory::make_adapter<Interface, Impl>(impl);
}

/**
 * @brief Convenience function for unwrapping adapters
 *
 * @tparam T The expected underlying type
 * @tparam Interface The interface type (deduced)
 * @param ptr Shared pointer to interface
 * @return Shared pointer to underlying implementation
 *
 * @code
 * auto thread_pool = unwrap_adapter<thread_pool_type>(executor);
 * @endcode
 */
template<typename T, typename Interface>
std::shared_ptr<T> unwrap_adapter(std::shared_ptr<Interface> ptr) {
    return smart_adapter_factory::try_unwrap<T, Interface>(ptr);
}

} // namespace common::adapters
