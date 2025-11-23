// BSD 3-Clause License
// Copyright (c) 2025, kcenon
// See the LICENSE file in the project root for full license information.

#pragma once

#include <cstddef>
#include <functional>
#include <memory>
#include <mutex>
#include <stack>
#include <utility>
#include <new>
#include <vector>

namespace kcenon::common::utils {

namespace detail {
template<typename T>
struct RawDelete {
    void operator()(T* ptr) const noexcept {
        ::operator delete(static_cast<void*>(ptr));
    }
};
} // namespace detail

/**
 * @brief Thread-safe object pool that reuses raw storage for expensive objects.
 *
 * The pool allocates raw memory once and performs placement-new construction
 * on acquisition. Objects are destructed when released, but the underlying
 * storage is retained for fast reuse.
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
     * @return A unique_ptr to the acquired object with a custom deleter that returns it to the pool.
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
     * @brief Add @p count additional blocks to the pool.
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
