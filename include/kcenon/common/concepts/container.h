// BSD 3-Clause License
// Copyright (c) 2025, kcenon
// See the LICENSE file in the project root for full license information.

/**
 * @file container.h
 * @brief C++20 concepts for container and collection types.
 *
 * This header provides concepts for validating container types used
 * throughout the common_system library. These concepts enable
 * compile-time validation with clear error messages.
 *
 * Requirements:
 * - C++20 compiler with concepts support
 * - GCC 10+, Clang 10+, MSVC 2022+
 *
 * Thread Safety:
 * - Concepts are evaluated at compile-time only.
 * - No runtime thread-safety considerations apply.
 *
 * @see https://en.cppreference.com/w/cpp/language/constraints
 */

#pragma once

#include <concepts>
#include <type_traits>
#include <iterator>

namespace kcenon::common {
namespace concepts {

/**
 * @concept Container
 * @brief A type that satisfies basic container requirements.
 *
 * Containers must provide begin(), end(), size(), and empty() methods.
 *
 * Example usage:
 * @code
 * template<Container C>
 * void print_size(const C& container) {
 *     std::cout << "Size: " << container.size() << std::endl;
 * }
 * @endcode
 */
template<typename T>
concept Container = requires(T t) {
    typename T::value_type;
    typename T::iterator;
    typename T::const_iterator;
    { t.begin() } -> std::input_or_output_iterator;
    { t.end() } -> std::input_or_output_iterator;
    { t.size() } -> std::convertible_to<std::size_t>;
    { t.empty() } -> std::convertible_to<bool>;
};

/**
 * @concept SequenceContainer
 * @brief A container that provides sequential access and modification.
 *
 * Sequence containers support push_back, pop_back, and front/back access.
 *
 * Example usage:
 * @code
 * template<SequenceContainer C>
 * void append(C& container, typename C::value_type value) {
 *     container.push_back(std::move(value));
 * }
 * @endcode
 */
template<typename T>
concept SequenceContainer = Container<T> && requires(T t, typename T::value_type v) {
    { t.push_back(v) };
    { t.front() } -> std::same_as<typename T::value_type&>;
    { t.back() } -> std::same_as<typename T::value_type&>;
};

/**
 * @concept AssociativeContainer
 * @brief A container that provides key-based access.
 *
 * Associative containers support find, count, and key-based operations.
 *
 * Example usage:
 * @code
 * template<AssociativeContainer C>
 * bool contains_key(const C& container, typename C::key_type key) {
 *     return container.find(key) != container.end();
 * }
 * @endcode
 */
template<typename T>
concept AssociativeContainer = Container<T> && requires(T t) {
    typename T::key_type;
    { t.find(std::declval<typename T::key_type>()) } -> std::same_as<typename T::iterator>;
    { t.count(std::declval<typename T::key_type>()) } -> std::convertible_to<std::size_t>;
};

/**
 * @concept MappingContainer
 * @brief A container that maps keys to values.
 *
 * Mapping containers provide key-value pair storage with indexed access.
 *
 * Example usage:
 * @code
 * template<MappingContainer C>
 * auto get_or_default(C& container, typename C::key_type key,
 *                     typename C::mapped_type default_val) {
 *     auto it = container.find(key);
 *     return it != container.end() ? it->second : default_val;
 * }
 * @endcode
 */
template<typename T>
concept MappingContainer = AssociativeContainer<T> && requires {
    typename T::mapped_type;
};

/**
 * @concept ResizableContainer
 * @brief A container that can be resized.
 *
 * Resizable containers support resize, reserve, and capacity operations.
 *
 * Example usage:
 * @code
 * template<ResizableContainer C>
 * void ensure_capacity(C& container, std::size_t capacity) {
 *     if (container.capacity() < capacity) {
 *         container.reserve(capacity);
 *     }
 * }
 * @endcode
 */
template<typename T>
concept ResizableContainer = Container<T> && requires(T t, std::size_t n) {
    { t.resize(n) };
    { t.reserve(n) };
    { t.capacity() } -> std::convertible_to<std::size_t>;
};

/**
 * @concept ClearableContainer
 * @brief A container that can be cleared.
 *
 * Example usage:
 * @code
 * template<ClearableContainer C>
 * void reset(C& container) {
 *     container.clear();
 * }
 * @endcode
 */
template<typename T>
concept ClearableContainer = Container<T> && requires(T t) {
    { t.clear() } -> std::same_as<void>;
};

/**
 * @concept InsertableContainer
 * @brief A container that supports insert operations.
 *
 * Example usage:
 * @code
 * template<InsertableContainer C>
 * void insert_at(C& container, typename C::iterator pos,
 *                typename C::value_type value) {
 *     container.insert(pos, std::move(value));
 * }
 * @endcode
 */
template<typename T>
concept InsertableContainer = Container<T> && requires(T t, typename T::iterator it, typename T::value_type v) {
    { t.insert(it, v) } -> std::same_as<typename T::iterator>;
};

/**
 * @concept ErasableContainer
 * @brief A container that supports erase operations.
 *
 * Example usage:
 * @code
 * template<ErasableContainer C>
 * void remove_at(C& container, typename C::iterator pos) {
 *     container.erase(pos);
 * }
 * @endcode
 */
template<typename T>
concept ErasableContainer = Container<T> && requires(T t, typename T::iterator it) {
    { t.erase(it) } -> std::same_as<typename T::iterator>;
};

/**
 * @concept RandomAccessContainer
 * @brief A container that supports random access via operator[].
 *
 * Example usage:
 * @code
 * template<RandomAccessContainer C>
 * auto get_at(const C& container, std::size_t index) {
 *     return container[index];
 * }
 * @endcode
 */
template<typename T>
concept RandomAccessContainer = Container<T> && requires(T t, std::size_t i) {
    { t[i] } -> std::same_as<typename T::value_type&>;
};

/**
 * @concept BoundedContainer
 * @brief A container with fixed maximum capacity.
 *
 * Bounded containers have a maximum size that cannot be exceeded.
 *
 * Example usage:
 * @code
 * template<BoundedContainer C>
 * bool can_add(const C& container) {
 *     return container.size() < container.max_size();
 * }
 * @endcode
 */
template<typename T>
concept BoundedContainer = Container<T> && requires(const T t) {
    { t.max_size() } -> std::convertible_to<std::size_t>;
};

/**
 * @concept ThreadSafeContainer
 * @brief A container designed for thread-safe access.
 *
 * Thread-safe containers must provide a lock() method returning a lock guard.
 *
 * Example usage:
 * @code
 * template<ThreadSafeContainer C>
 * void safe_access(C& container) {
 *     auto lock = container.lock();
 *     // Safe operations under lock
 * }
 * @endcode
 */
template<typename T>
concept ThreadSafeContainer = Container<T> && requires(T t) {
    { t.lock() };
};

/**
 * @concept PoolableContainer
 * @brief A container that supports object pooling operations.
 *
 * Poolable containers provide acquire and release operations for
 * reusing allocated objects.
 *
 * Example usage:
 * @code
 * template<PoolableContainer P>
 * auto get_pooled(P& pool) {
 *     return pool.acquire();
 * }
 * @endcode
 */
template<typename T>
concept PoolableContainer = requires(T t) {
    { t.acquire() };
    { t.release(std::declval<decltype(t.acquire())>()) };
    { t.available_count() } -> std::convertible_to<std::size_t>;
};

/**
 * @concept CircularBuffer
 * @brief A container that operates as a circular buffer.
 *
 * Circular buffers provide full() check and overwrite oldest data
 * when capacity is reached.
 *
 * Example usage:
 * @code
 * template<CircularBuffer C>
 * void add_with_overwrite(C& buffer, typename C::value_type value) {
 *     buffer.push_back(std::move(value));  // Overwrites if full
 * }
 * @endcode
 */
template<typename T>
concept CircularBuffer = Container<T> && requires(const T t) {
    { t.full() } -> std::convertible_to<bool>;
    { t.capacity() } -> std::convertible_to<std::size_t>;
};

} // namespace concepts
} // namespace kcenon::common
