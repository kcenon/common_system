// BSD 3-Clause License
// Copyright (c) 2025, kcenon
// See the LICENSE file in the project root for full license information.

#pragma once

#include <array>
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

} // namespace kcenon::common::utils
