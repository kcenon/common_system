// BSD 3-Clause License
// Copyright (c) 2025, 🍀☀🌕🌥 🌊
// See the LICENSE file in the project root for full license information.

#pragma once

#include <array>
#include <atomic>
#include <cstddef>
#include <mutex>
#include <optional>
#include <utility>

namespace kcenon::common::utils {

template<typename T, std::size_t Capacity>
class CircularBuffer {
    static_assert(Capacity > 0, "CircularBuffer capacity must be greater than zero");

public:
    CircularBuffer() = default;

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

/**
 * @brief Lock-free circular buffer for single-producer single-consumer (SPSC).
 *
 * Uses std::atomic head/tail with acquire/release ordering. No mutex is needed
 * because only the producer modifies tail_ and only the consumer modifies head_.
 * Capacity must be > 0. One slot is reserved to distinguish full from empty,
 * so the usable capacity is exactly Capacity elements.
 */
template<typename T, std::size_t Capacity>
class SPSCCircularBuffer {
    static_assert(Capacity > 0, "SPSCCircularBuffer capacity must be greater than zero");

public:
    SPSCCircularBuffer() = default;

    /**
     * @brief Push a value (producer thread only).
     * @return true if pushed, false if full.
     */
    bool push(const T& value) {
        const auto tail = tail_.load(std::memory_order_relaxed);
        const auto next_tail = advance(tail);
        if (next_tail == head_.load(std::memory_order_acquire)) {
            return false; // full
        }
        buffer_[tail] = value;
        tail_.store(next_tail, std::memory_order_release);
        return true;
    }

    bool push(T&& value) {
        const auto tail = tail_.load(std::memory_order_relaxed);
        const auto next_tail = advance(tail);
        if (next_tail == head_.load(std::memory_order_acquire)) {
            return false; // full
        }
        buffer_[tail] = std::move(value);
        tail_.store(next_tail, std::memory_order_release);
        return true;
    }

    /**
     * @brief Pop a value (consumer thread only).
     * @return The value if available, std::nullopt if empty.
     */
    [[nodiscard]] std::optional<T> pop() {
        const auto head = head_.load(std::memory_order_relaxed);
        if (head == tail_.load(std::memory_order_acquire)) {
            return std::nullopt; // empty
        }
        auto value = std::move(buffer_[head]);
        head_.store(advance(head), std::memory_order_release);
        return value;
    }

    [[nodiscard]] bool empty() const noexcept {
        return head_.load(std::memory_order_acquire)
            == tail_.load(std::memory_order_acquire);
    }

    [[nodiscard]] bool full() const noexcept {
        return advance(tail_.load(std::memory_order_acquire))
            == head_.load(std::memory_order_acquire);
    }

    [[nodiscard]] std::size_t size() const noexcept {
        const auto head = head_.load(std::memory_order_acquire);
        const auto tail = tail_.load(std::memory_order_acquire);
        if (tail >= head) {
            return tail - head;
        }
        return (Capacity + 1) - head + tail;
    }

    [[nodiscard]] constexpr std::size_t capacity() const noexcept {
        return Capacity;
    }

private:
    static std::size_t advance(std::size_t index) noexcept {
        return (index + 1) % (Capacity + 1);
    }

    // Internal buffer has Capacity+1 slots to distinguish full from empty
    std::array<T, Capacity + 1> buffer_{};
    alignas(64) std::atomic<std::size_t> head_{0};
    alignas(64) std::atomic<std::size_t> tail_{0};
};

} // namespace kcenon::common::utils
