/*
 * BSD 3-Clause License
 * Copyright (c) 2025, kcenon
 */

#pragma once

#include <atomic>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <type_traits>

namespace kcenon::common::adapters {

// Forward declarations
class adapter_base;

template<typename T>
struct adapter_traits;

/**
 * @brief Primary adapter_traits template for value types
 * @tparam T The type being wrapped
 */
template<typename T>
struct adapter_traits {
    using value_type = T;
    using pointer_type = T*;
    using reference_type = T&;
    using const_reference_type = const T&;
    static constexpr bool is_smart_pointer = false;
    static constexpr bool supports_weak = false;
};

/**
 * @brief Specialization for std::shared_ptr
 */
template<typename T>
struct adapter_traits<std::shared_ptr<T>> {
    using value_type = T;
    using pointer_type = std::shared_ptr<T>;
    using reference_type = T&;
    using const_reference_type = const T&;
    using weak_type = std::weak_ptr<T>;
    static constexpr bool is_smart_pointer = true;
    static constexpr bool supports_weak = true;
};

/**
 * @brief Specialization for std::unique_ptr
 */
template<typename T, typename Deleter>
struct adapter_traits<std::unique_ptr<T, Deleter>> {
    using value_type = T;
    using pointer_type = std::unique_ptr<T, Deleter>;
    using reference_type = T&;
    using const_reference_type = const T&;
    static constexpr bool is_smart_pointer = true;
    static constexpr bool supports_weak = false;
};

/**
 * @brief Base class for adapter interface to eliminate RTTI dependency
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
 * @brief Unified adapter template for wrapping values or smart pointers
 *
 * This adapter provides a consistent interface regardless of whether
 * the wrapped type is a value, shared_ptr, or unique_ptr.
 *
 * @tparam T The type to wrap (can be value, shared_ptr<U>, or unique_ptr<U>)
 * @tparam Traits Adapter traits (automatically deduced)
 *
 * @code
 * // Value type
 * auto a1 = adapter<int>(42);
 *
 * // Shared pointer
 * auto a2 = adapter(std::make_shared<MyClass>());
 *
 * // Factory functions
 * auto a3 = make_adapter<MyClass>(args...);
 * auto a4 = make_shared_adapter<MyClass>(args...);
 * @endcode
 */
template<typename T, typename Traits = adapter_traits<T>>
class adapter {
public:
    using value_type = typename Traits::value_type;
    using pointer_type = typename Traits::pointer_type;
    using reference_type = typename Traits::reference_type;
    using const_reference_type = typename Traits::const_reference_type;

    /**
     * @brief Construct adapter from value
     * @param value The value to wrap
     */
    explicit adapter(T value) : value_(std::move(value)) {}

    /**
     * @brief Default constructor (for default-constructible types)
     */
    adapter() requires std::is_default_constructible_v<T> : value_() {}

    /**
     * @brief Access raw pointer to the stored value
     * @return Pointer to the value
     */
    auto get() const noexcept {
        if constexpr (Traits::is_smart_pointer) {
            return value_.get();
        } else {
            return std::addressof(value_);
        }
    }

    /**
     * @brief Dereference operator
     * @return Reference to the stored value
     */
    decltype(auto) operator*() const {
        if constexpr (Traits::is_smart_pointer) {
            return *value_;
        } else {
            return value_;
        }
    }

    /**
     * @brief Mutable dereference
     */
    decltype(auto) operator*() {
        if constexpr (Traits::is_smart_pointer) {
            return *value_;
        } else {
            return value_;
        }
    }

    /**
     * @brief Arrow operator
     * @return Pointer to the value for member access
     */
    auto operator->() const noexcept { return get(); }

    /**
     * @brief Arrow operator (mutable)
     */
    auto operator->() noexcept { return get(); }

    /**
     * @brief Get weak reference (only for shared_ptr)
     * @return weak_ptr to the stored value
     */
    auto weak() const requires(Traits::supports_weak) {
        return typename Traits::weak_type(value_);
    }

    /**
     * @brief Check if adapter holds a valid value
     * @return true if valid, false otherwise
     */
    explicit operator bool() const noexcept {
        if constexpr (Traits::is_smart_pointer) {
            return static_cast<bool>(value_);
        } else {
            return true; // Value types are always valid
        }
    }

    /**
     * @brief Get the underlying storage
     * @return Reference to the stored value
     */
    const T& value() const& noexcept { return value_; }

    /**
     * @brief Get the underlying storage (mutable)
     */
    T& value() & noexcept { return value_; }

    /**
     * @brief Move out the underlying storage
     */
    T value() && noexcept { return std::move(value_); }

    /**
     * @brief Release ownership and return the underlying value
     * @return The stored value (moved)
     */
    T release() noexcept { return std::move(value_); }

    /**
     * @brief Check if adapter is holding a smart pointer type
     * @return true if smart pointer, false if value type
     */
    static constexpr bool is_smart_pointer() noexcept {
        return Traits::is_smart_pointer;
    }

    /**
     * @brief Check if weak references are supported
     * @return true if supports weak references
     */
    static constexpr bool supports_weak() noexcept {
        return Traits::supports_weak;
    }

private:
    T value_;
};

// Deduction guides
template<typename T>
adapter(T) -> adapter<T>;

template<typename T>
adapter(std::shared_ptr<T>) -> adapter<std::shared_ptr<T>>;

template<typename T, typename D>
adapter(std::unique_ptr<T, D>) -> adapter<std::unique_ptr<T, D>>;

/**
 * @brief Factory function to create adapter with in-place construction
 *
 * @tparam T The type to construct
 * @tparam Args Constructor argument types
 * @param args Arguments to forward to T's constructor
 * @return adapter<T>
 */
template<typename T, typename... Args>
auto make_adapter(Args&&... args) {
    return adapter<T>(T(std::forward<Args>(args)...));
}

/**
 * @brief Factory function to create adapter wrapping shared_ptr
 *
 * @tparam T The type to manage
 * @tparam Args Constructor argument types
 * @param args Arguments to forward to T's constructor
 * @return adapter<std::shared_ptr<T>>
 */
template<typename T, typename... Args>
auto make_shared_adapter(Args&&... args) {
    return adapter(std::make_shared<T>(std::forward<Args>(args)...));
}

/**
 * @brief Factory function to create adapter wrapping unique_ptr
 *
 * @tparam T The type to manage
 * @tparam Args Constructor argument types
 * @param args Arguments to forward to T's constructor
 * @return adapter<std::unique_ptr<T>>
 */
template<typename T, typename... Args>
auto make_unique_adapter(Args&&... args) {
    return adapter(std::make_unique<T>(std::forward<Args>(args)...));
}

/**
 * @brief Interface adapter with type safety and depth tracking
 *
 * This template provides:
 * - Type identification via type ID system (no RTTI)
 * - Wrapper depth tracking to prevent infinite chains
 * - Unwrap functionality to access underlying implementation
 * - Maximum depth limit (default: 2) to prevent performance issues
 *
 * @tparam Interface The interface type being adapted to
 * @tparam Implementation The concrete implementation being wrapped
 */
template<typename Interface, typename Implementation>
class interface_adapter : public Interface, public adapter_base {
public:
    /**
     * @brief Construct adapter with existing implementation
     * @param impl Shared pointer to the implementation
     * @throws std::runtime_error if wrapper depth exceeds maximum
     */
    explicit interface_adapter(std::shared_ptr<Implementation> impl)
        : impl_(impl), wrapper_depth_(calculate_depth(impl)) {
        if (wrapper_depth_ > max_wrapper_depth_) {
            throw std::runtime_error(
                "Adapter chain too deep (" + std::to_string(wrapper_depth_) +
                " levels, max: " + std::to_string(max_wrapper_depth_) + ")");
        }
    }

    virtual ~interface_adapter() = default;

    /**
     * @brief Get the underlying implementation
     * @return Shared pointer to the wrapped implementation
     */
    std::shared_ptr<Implementation> unwrap() const { return impl_; }

    /**
     * @brief Check if this adapter wraps another adapter
     * @return true if wrapping another adapter, false otherwise
     */
    bool is_wrapped_adapter() const { return wrapper_depth_ > 0; }

    /**
     * @brief Get the current wrapper depth
     * @return Number of adapter layers (0 for direct implementation)
     */
    size_t get_wrapper_depth() const { return wrapper_depth_; }

    /**
     * @brief Get the adapter depth (implements adapter_base)
     * @return Wrapper depth
     */
    size_t get_adapter_depth() const override { return wrapper_depth_; }

    /**
     * @brief Get type name for debugging
     * @return Type name string
     */
    static std::string adapter_type_name() {
        return "interface_adapter<Interface, Implementation>";
    }

    /**
     * @brief Get the maximum allowed wrapper depth
     * @return Maximum depth value
     */
    static constexpr size_t max_depth() { return max_wrapper_depth_; }

    /**
     * @brief Get unique type ID for this adapter type
     * @return Type ID
     */
    size_t get_type_id() const override { return get_static_type_id(); }

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
     */
    static size_t calculate_depth(std::shared_ptr<Implementation> impl) {
        if (!impl) {
            return 0;
        }

        if constexpr (std::is_base_of_v<adapter_base, Implementation>) {
#ifdef __cpp_rtti
            if (auto* base_adapter =
                    dynamic_cast<adapter_base*>(impl.get())) {
                return 1 + base_adapter->get_adapter_depth();
            }
            return 0;
#else
            auto* base_adapter = static_cast<adapter_base*>(impl.get());
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
 * @brief Type trait to check if a type implements an interface
 * @tparam T The type to check
 * @tparam Interface The interface to check against
 */
template<typename T, typename Interface>
inline constexpr bool implements_interface_v =
    std::is_base_of_v<Interface, T> || std::is_convertible_v<T*, Interface*>;

/**
 * @brief Smart adapter factory that avoids unnecessary wrapping
 *
 * This factory uses compile-time checks to determine if adaptation is needed:
 * - If the implementation already implements the interface, direct cast
 * (zero-cost)
 * - Otherwise, create an interface_adapter wrapper
 */
class adapter_factory {
public:
    /**
     * @brief Create an adapter with automatic optimization
     *
     * If Impl already implements Interface, returns a static_pointer_cast
     * (zero-cost). Otherwise, creates an interface_adapter wrapper.
     *
     * @tparam Interface The target interface type
     * @tparam Impl The implementation type
     * @param impl Shared pointer to the implementation
     * @return Shared pointer to Interface (either direct cast or adapter)
     */
    template<typename Interface, typename Impl>
    static std::shared_ptr<Interface>
    create(std::shared_ptr<Impl> impl) {
        if constexpr (implements_interface_v<Impl, Interface>) {
            return std::static_pointer_cast<Interface>(impl);
        } else {
            return std::make_shared<interface_adapter<Interface, Impl>>(impl);
        }
    }

    /**
     * @brief Create an adapter with explicit adapter type
     *
     * @tparam AdapterType The specific adapter class to use
     * @tparam Args Constructor argument types
     * @param args Arguments to forward to adapter constructor
     * @return Shared pointer to the created adapter
     */
    template<typename AdapterType, typename... Args>
    static std::shared_ptr<AdapterType> create_explicit(Args&&... args) {
        return std::make_shared<AdapterType>(std::forward<Args>(args)...);
    }

    /**
     * @brief Try to unwrap an interface to get underlying implementation
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

        if constexpr (std::is_base_of_v<adapter_base, Interface>) {
#ifdef __cpp_rtti
            if (auto* adapter =
                    dynamic_cast<interface_adapter<Interface, T>*>(ptr.get())) {
                return adapter->unwrap();
            }
#else
            auto* base = static_cast<adapter_base*>(ptr.get());
            if (base && base->get_type_id() ==
                            interface_adapter<Interface, T>::get_static_type_id()) {
                auto* adapter =
                    static_cast<interface_adapter<Interface, T>*>(ptr.get());
                return adapter->unwrap();
            }
#endif
        }

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
 * @brief Helper function to safely unwrap an adapter
 * @tparam T The expected underlying type
 * @tparam Interface The interface type
 * @param ptr Shared pointer to the interface
 * @return Shared pointer to unwrapped implementation, or nullptr
 */
template<typename T, typename Interface>
std::shared_ptr<T> safe_unwrap(std::shared_ptr<Interface> ptr) {
    return adapter_factory::try_unwrap<T, Interface>(ptr);
}

/**
 * @brief Check if an interface pointer is actually an adapter
 * @tparam Interface The interface type
 * @param ptr Shared pointer to check
 * @return true if ptr is an adapter, false otherwise
 */
template<typename Interface>
bool is_adapter(std::shared_ptr<Interface> ptr) {
    if (!ptr) {
        return false;
    }

    if constexpr (std::is_base_of_v<adapter_base, Interface>) {
        auto* base = static_cast<adapter_base*>(ptr.get());
        return base->is_adapter();
    }

    return false;
}

/**
 * @brief Convenience function for creating smart adapters
 *
 * @tparam Interface The target interface type
 * @tparam Impl The implementation type (deduced)
 * @param impl Shared pointer to implementation
 * @return Shared pointer to Interface
 */
template<typename Interface, typename Impl>
std::shared_ptr<Interface> make_interface_adapter(std::shared_ptr<Impl> impl) {
    return adapter_factory::create<Interface, Impl>(impl);
}

/**
 * @brief Convenience function for unwrapping adapters
 *
 * @tparam T The expected underlying type
 * @tparam Interface The interface type (deduced)
 * @param ptr Shared pointer to interface
 * @return Shared pointer to underlying implementation
 */
template<typename T, typename Interface>
std::shared_ptr<T> unwrap_adapter(std::shared_ptr<Interface> ptr) {
    return adapter_factory::try_unwrap<T, Interface>(ptr);
}

} // namespace kcenon::common::adapters
