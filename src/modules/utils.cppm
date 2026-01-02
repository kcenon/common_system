// BSD 3-Clause License
// Copyright (c) 2025, kcenon
// See the LICENSE file in the project root for full license information.

/**
 * @file utils.cppm
 * @brief C++20 module partition for common_system utilities.
 *
 * This module partition exports utility classes:
 * - source_location: C++17-compatible source location
 * - CircularBuffer: Thread-safe circular buffer
 * - ObjectPool: Thread-safe object pool for reuse
 *
 * Part of the kcenon.common module.
 */

module;

// Standard library imports needed before module declaration
#include <array>
#include <cstddef>
#include <functional>
#include <memory>
#include <mutex>
#include <new>
#include <optional>
#include <stack>
#include <utility>
#include <vector>

// Feature detection for source_location
#if __has_include(<source_location>)
    #include <source_location>
    #define KCENON_MODULE_HAS_SOURCE_LOCATION 1
#else
    #define KCENON_MODULE_HAS_SOURCE_LOCATION 0
#endif

export module kcenon.common:utils;

export namespace kcenon::common {

// ============================================================================
// Source Location
// ============================================================================

#if KCENON_MODULE_HAS_SOURCE_LOCATION
using source_location = std::source_location;
#else
/**
 * @struct source_location
 * @brief C++17-compatible source_location implementation using compiler builtins.
 */
struct source_location {
public:
    constexpr source_location(
        const char* file = __builtin_FILE(),
        const char* function = __builtin_FUNCTION(),
        int line = __builtin_LINE()
    ) noexcept
        : file_(file), function_(function), line_(line), column_(0) {}

    constexpr const char* file_name() const noexcept { return file_; }
    constexpr const char* function_name() const noexcept { return function_; }
    constexpr int line() const noexcept { return line_; }
    constexpr int column() const noexcept { return column_; }

    static constexpr source_location current(
        const char* file = __builtin_FILE(),
        const char* function = __builtin_FUNCTION(),
        int line = __builtin_LINE()
    ) noexcept {
        return source_location(file, function, line);
    }

private:
    const char* file_;
    const char* function_;
    int line_;
    int column_;
};
#endif

} // namespace kcenon::common

export namespace kcenon::common::utils {

// ============================================================================
// Circular Buffer
// ============================================================================

/**
 * @class CircularBuffer
 * @brief Thread-safe fixed-size circular buffer.
 *
 * @tparam T Element type
 * @tparam Capacity Maximum number of elements
 */
template<typename T, std::size_t Capacity>
class CircularBuffer {
    static_assert(Capacity > 0, "CircularBuffer capacity must be greater than zero");

public:
    CircularBuffer() = default;

    /**
     * @brief Push a value to the buffer.
     * @param value Value to push
     * @param overwrite If true, overwrite oldest value when full
     * @return true if pushed successfully, false if full and overwrite is false
     */
    bool push(const T& value, bool overwrite = false) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (is_full_locked() && !overwrite) {
            return false;
        }
        if (is_full_locked()) {
            pop_locked();
        }
        buffer_[tail_] = value;
        advance(tail_);
        ++size_;
        return true;
    }

    bool push(T&& value, bool overwrite = false) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (is_full_locked() && !overwrite) {
            return false;
        }
        if (is_full_locked()) {
            pop_locked();
        }
        buffer_[tail_] = std::move(value);
        advance(tail_);
        ++size_;
        return true;
    }

    /**
     * @brief Pop a value from the buffer.
     * @return The oldest value, or nullopt if empty
     */
    [[nodiscard]] std::optional<T> pop() {
        std::lock_guard<std::mutex> lock(mutex_);
        return pop_locked();
    }

    [[nodiscard]] bool empty() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return size_ == 0;
    }

    [[nodiscard]] bool full() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return size_ == Capacity;
    }

    [[nodiscard]] std::size_t size() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return size_;
    }

    [[nodiscard]] constexpr std::size_t capacity() const {
        return Capacity;
    }

private:
    void advance(std::size_t& index) noexcept {
        index = (index + 1) % Capacity;
    }

    bool is_full_locked() const noexcept {
        return size_ == Capacity;
    }

    std::optional<T> pop_locked() {
        if (size_ == 0) {
            return std::nullopt;
        }
        auto value = std::move(buffer_[head_]);
        advance(head_);
        --size_;
        return value;
    }

    mutable std::mutex mutex_;
    std::array<T, Capacity> buffer_{};
    std::size_t head_{0};
    std::size_t tail_{0};
    std::size_t size_{0};
};

// ============================================================================
// Object Pool
// ============================================================================

namespace detail {
template<typename T>
struct RawDelete {
    void operator()(T* ptr) const noexcept {
        ::operator delete(static_cast<void*>(ptr));
    }
};
} // namespace detail

/**
 * @class ObjectPool
 * @brief Thread-safe object pool that reuses raw storage for expensive objects.
 *
 * The pool allocates raw memory once and performs placement-new construction
 * on acquisition. Objects are destructed when released, but the underlying
 * storage is retained for fast reuse.
 *
 * @tparam T Object type to pool
 */
template<typename T>
class ObjectPool {
public:
    using value_type = T;

    explicit ObjectPool(std::size_t growth = 32)
        : growth_(growth == 0 ? 1 : growth) {}

    /**
     * @brief Acquire an object constructed with the provided arguments.
     * @param reused Optional pointer that receives whether an existing block was reused.
     * @param args Arguments forwarded to the object's constructor.
     * @return A unique_ptr to the acquired object with a custom deleter.
     */
    template<typename... Args>
    std::unique_ptr<T, std::function<void(T*)>> acquire(bool* reused, Args&&... args) {
        T* raw = nullptr;
        bool reused_local = false;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (free_list_.empty()) {
                allocate_block_unlocked(growth_);
            } else {
                reused_local = true;
            }

            raw = free_list_.top();
            free_list_.pop();
        }

        if (reused) {
            *reused = reused_local;
        }

        new (raw) T(std::forward<Args>(args)...);
        return std::unique_ptr<T, std::function<void(T*)>>(raw, [this](T* ptr) {
            this->release(ptr);
        });
    }

    template<typename... Args>
    std::unique_ptr<T, std::function<void(T*)>> acquire(Args&&... args) {
        return acquire(static_cast<bool*>(nullptr), std::forward<Args>(args)...);
    }

    /**
     * @brief Return raw storage to the pool (destructor already run).
     */
    void release(T* ptr) noexcept {
        if (!ptr) {
            return;
        }

        ptr->~T();
        std::lock_guard<std::mutex> lock(mutex_);
        free_list_.push(ptr);
    }

    /**
     * @brief Add count additional blocks to the pool.
     */
    void reserve(std::size_t count) {
        if (count == 0) {
            return;
        }
        std::lock_guard<std::mutex> lock(mutex_);
        allocate_block_unlocked(count);
    }

    /**
     * @brief Destroy all cached instances and release memory.
     */
    void clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        free_list_ = std::stack<T*>();
        storage_.clear();
    }

    [[nodiscard]] std::size_t available() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return free_list_.size();
    }

private:
    using RawPtr = std::unique_ptr<T, detail::RawDelete<T>>;

    void allocate_block_unlocked(std::size_t count) {
        for (std::size_t i = 0; i < count; ++i) {
            RawPtr block(static_cast<T*>(::operator new(sizeof(T))));
            free_list_.push(block.get());
            storage_.push_back(std::move(block));
        }
    }

    std::size_t growth_;
    mutable std::mutex mutex_;
    std::stack<T*> free_list_;
    std::vector<RawPtr> storage_;
};

} // namespace kcenon::common::utils
