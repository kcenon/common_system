/*
 * BSD 3-Clause License
 * Copyright (c) 2025, kcenon
 */

#pragma once

#include <atomic>
#include <memory>
#include <stdexcept>
#include <string>

/**
 * @file typed_adapter.h
 * @brief Legacy adapter classes (deprecated)
 *
 * @deprecated Use adapter.h instead:
 * - adapter_base -> kcenon::common::adapters::adapter_base (from adapter.h)
 * - typed_adapter<I, T> -> interface_adapter<I, T> (from adapter.h)
 * - safe_unwrap -> adapter_factory::try_unwrap (from adapter.h)
 * - is_adapter -> kcenon::common::adapters::is_adapter (from adapter.h)
 *
 * This file is maintained for backward compatibility and will be
 * removed in a future major version.
 */

namespace kcenon::common::adapters {

/**
 * @brief Base class for adapter interface to eliminate RTTI dependency
 * @note Consider using interface_adapter from adapter.h for new code
 */
class adapter_base {
public:
    virtual ~adapter_base() = default;

    /**
     * @brief Get the wrapper depth for this adapter
     * @return Number of adapter layers (0 for direct implementation)
     */
    virtual size_t get_adapter_depth() const = 0;

    /**
     * @brief Check if this is an adapter (always true for adapter_base)
     * @return true
     */
    virtual bool is_adapter() const { return true; }

    /**
     * @brief Get unique type ID for this adapter type
     * @return Type ID
     */
    virtual size_t get_type_id() const = 0;
};

/**
 * @brief Base adapter class with type safety and depth tracking
 *
 * This template provides:
 * - Type identification via type ID system (no RTTI)
 * - Wrapper depth tracking to prevent infinite chains
 * - Unwrap functionality to access underlying implementation
 * - Maximum depth limit (default: 2) to prevent performance issues
 *
 * @note Consider using interface_adapter from adapter.h for new code
 *
 * @tparam Interface The interface type being adapted to
 * @tparam Implementation The concrete implementation being wrapped
 */
template<typename Interface, typename Implementation>
class typed_adapter : public Interface, public adapter_base {
public:
    /**
     * @brief Construct adapter with existing implementation
     * @param impl Shared pointer to the implementation
     * @throws std::runtime_error if wrapper depth exceeds maximum
     */
    explicit typed_adapter(std::shared_ptr<Implementation> impl)
        : impl_(impl), wrapper_depth_(calculate_depth(impl)) {
        if (wrapper_depth_ > max_wrapper_depth_) {
            throw std::runtime_error(
                "Adapter chain too deep (" + std::to_string(wrapper_depth_) +
                " levels, max: " + std::to_string(max_wrapper_depth_) + ")");
        }
    }

    virtual ~typed_adapter() = default;

    /**
     * @brief Get the underlying implementation
     * @return Shared pointer to the wrapped implementation
     */
    std::shared_ptr<Implementation> unwrap() const {
        return impl_;
    }

    /**
     * @brief Check if this adapter wraps another adapter
     * @return true if wrapping another adapter, false otherwise
     */
    bool is_wrapped_adapter() const {
        return wrapper_depth_ > 0;
    }

    /**
     * @brief Get the current wrapper depth
     * @return Number of adapter layers (0 for direct implementation)
     */
    size_t get_wrapper_depth() const {
        return wrapper_depth_;
    }

    /**
     * @brief Get the adapter depth (implements adapter_base)
     * @return Wrapper depth
     */
    size_t get_adapter_depth() const override {
        return wrapper_depth_;
    }

    /**
     * @brief Get type name for debugging
     * @return Type name string
     */
    static std::string adapter_type_name() {
        return "typed_adapter<Interface, Implementation>";
    }

    /**
     * @brief Get the maximum allowed wrapper depth
     * @return Maximum depth value
     */
    static constexpr size_t max_depth() {
        return max_wrapper_depth_;
    }

    /**
     * @brief Get unique type ID for this adapter type
     * @return Type ID
     */
    size_t get_type_id() const override {
        return get_static_type_id();
    }

    /**
     * @brief Get static type ID for this adapter type
     * @return Type ID
     */
    static size_t get_static_type_id() {
        static const size_t id = generate_type_id();
        return id;
    }

protected:
    std::shared_ptr<Implementation> impl_;

private:
    size_t wrapper_depth_;
    static constexpr size_t max_wrapper_depth_ = 2;

    /**
     * @brief Generate a unique type ID
     * @return Unique ID
     */
    static size_t generate_type_id() {
        static std::atomic<size_t> counter{0};
        return ++counter;
    }

    /**
     * @brief Calculate the depth of adapter wrapping
     * @param impl The implementation to check
     * @return Depth level (0 for direct implementation, 1+ for wrapped)
     *
     * @note max_wrapper_depth_ = 2 is enforced to prevent performance degradation
     *       from excessive adapter layering. Deep nesting can cause cache misses
     *       and vtable indirection overhead.
     */
    static size_t calculate_depth(std::shared_ptr<Implementation> impl) {
        if (!impl) {
            return 0;
        }

        // Check if implementation inherits from adapter_base using compile-time check
        if constexpr (std::is_base_of_v<adapter_base, Implementation>) {
            // Use dynamic_cast for runtime safety when RTTI is available
            #ifdef __cpp_rtti
            if (auto* base_adapter = dynamic_cast<adapter_base*>(impl.get())) {
                return 1 + base_adapter->get_adapter_depth();
            }
            // If dynamic_cast fails, fall back to 0
            return 0;
            #else
            // When RTTI is disabled, use static_cast with additional validation
            auto* base_adapter = static_cast<adapter_base*>(impl.get());
            // Validate the cast by checking the type ID
            if (base_adapter && base_adapter->get_type_id() != 0) {
                return 1 + base_adapter->get_adapter_depth();
            }
            return 0;
            #endif
        }

        return 0;
    }
};

/**
 * @brief Helper function to safely unwrap an adapter
 * @tparam T The expected underlying type
 * @tparam Interface The interface type
 * @param ptr Shared pointer to the interface
 * @return Shared pointer to unwrapped implementation, or nullptr if not an adapter
 */
template<typename T, typename Interface>
std::shared_ptr<T> safe_unwrap(std::shared_ptr<Interface> ptr) {
    if (!ptr) {
        return nullptr;
    }

    // Check if Interface is derived from adapter_base using compile-time check
    if constexpr (std::is_base_of_v<adapter_base, Interface>) {
        #ifdef __cpp_rtti
        // Use dynamic_cast for runtime safety when RTTI is available
        if (auto* adapter = dynamic_cast<typed_adapter<Interface, T>*>(ptr.get())) {
            return adapter->unwrap();
        }
        #else
        // When RTTI is disabled, use static_cast with type ID validation
        auto* base = static_cast<adapter_base*>(ptr.get());

        // Check if this is the correct adapter type by comparing type IDs
        if (base && base->get_type_id() == typed_adapter<Interface, T>::get_static_type_id()) {
            // Safe static_cast to typed_adapter after validation
            auto* adapter = static_cast<typed_adapter<Interface, T>*>(ptr.get());
            return adapter->unwrap();
        }
        #endif
    }

    // Not an adapter or wrong type
    return nullptr;
}

/**
 * @brief Check if an interface pointer is actually an adapter
 * @tparam Interface The interface type
 * @param ptr Shared pointer to check
 * @return true if ptr is a typed_adapter, false otherwise
 */
template<typename Interface>
bool is_adapter(std::shared_ptr<Interface> ptr) {
    if (!ptr) {
        return false;
    }

    // Compile-time check if Interface inherits from adapter_base
    if constexpr (std::is_base_of_v<adapter_base, Interface>) {
        // Safe static_cast (no RTTI needed)
        auto* base = static_cast<adapter_base*>(ptr.get());
        return base->is_adapter();
    }

    // Not derived from adapter_base, so not an adapter
    return false;
}

} // namespace common::adapters
