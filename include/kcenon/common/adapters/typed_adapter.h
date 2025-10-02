/*
 * BSD 3-Clause License
 * Copyright (c) 2025, kcenon
 */

#pragma once

#include <memory>
#include <stdexcept>
#include <typeinfo>
#include <string>

namespace common::adapters {

/**
 * @brief Base adapter class with type safety and depth tracking
 *
 * This template provides:
 * - Type identification via RTTI
 * - Wrapper depth tracking to prevent infinite chains
 * - Unwrap functionality to access underlying implementation
 * - Maximum depth limit (default: 2) to prevent performance issues
 *
 * @tparam Interface The interface type being adapted to
 * @tparam Implementation The concrete implementation being wrapped
 */
template<typename Interface, typename Implementation>
class typed_adapter : public Interface {
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
     * @brief Get type name for debugging
     * @return Type name string
     */
    static std::string adapter_type_name() {
        return typeid(typed_adapter<Interface, Implementation>).name();
    }

    /**
     * @brief Get the maximum allowed wrapper depth
     * @return Maximum depth value
     */
    static constexpr size_t max_depth() {
        return max_wrapper_depth_;
    }

protected:
    std::shared_ptr<Implementation> impl_;

private:
    size_t wrapper_depth_;
    static constexpr size_t max_wrapper_depth_ = 2;

    /**
     * @brief Calculate the depth of adapter wrapping
     * @param impl The implementation to check
     * @return Depth level (0 for direct implementation, 1+ for wrapped)
     */
    static size_t calculate_depth(std::shared_ptr<Implementation> impl) {
        if (!impl) {
            return 0;
        }

        // Try to cast to typed_adapter to detect wrapping
        // This works if Implementation itself is a typed_adapter
        if (auto* adapter = dynamic_cast<typed_adapter*>(impl.get())) {
            return 1 + adapter->wrapper_depth_;
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

    // Try to cast to typed_adapter
    if (auto* adapter = dynamic_cast<typed_adapter<Interface, T>*>(ptr.get())) {
        return adapter->unwrap();
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

    // Check if it can be cast to any typed_adapter
    return dynamic_cast<void*>(ptr.get()) != nullptr &&
           std::string(typeid(*ptr).name()).find("typed_adapter") != std::string::npos;
}

} // namespace common::adapters
