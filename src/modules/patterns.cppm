// BSD 3-Clause License
// Copyright (c) 2025, kcenon
// See the LICENSE file in the project root for full license information.

/**
 * @file patterns.cppm
 * @brief C++20 module partition for design patterns.
 *
 * This module partition exports design pattern implementations:
 * - EventBus: Simple event bus for publish/subscribe pattern
 *
 * Part of the kcenon.common module.
 */

module;

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <typeindex>
#include <typeinfo>
#include <unordered_map>
#include <vector>

export module kcenon.common:patterns;

export namespace kcenon::common::patterns {

// ============================================================================
// Event Bus
// ============================================================================

/**
 * @class SimpleEventBus
 * @brief Thread-safe event bus for publish/subscribe pattern.
 *
 * Allows components to communicate through events without direct coupling.
 * Supports type-safe event publishing and subscription.
 */
class SimpleEventBus {
public:
    SimpleEventBus() : running_(false), next_id_(1) {}

    ~SimpleEventBus() {
        stop();
    }

    /**
     * @brief Start the event bus.
     */
    void start() {
        running_.store(true, std::memory_order_release);
    }

    /**
     * @brief Stop the event bus.
     */
    void stop() {
        running_.store(false, std::memory_order_release);
    }

    /**
     * @brief Check if the event bus is running.
     */
    bool is_running() const {
        return running_.load(std::memory_order_acquire);
    }

    /**
     * @brief Subscribe to events of a specific type.
     * @tparam Event Event type to subscribe to
     * @param handler Handler function to call when event is published
     * @return Subscription ID for unsubscribing
     */
    template<typename Event>
    uint64_t subscribe(std::function<void(const Event&)> handler) {
        std::unique_lock lock(mutex_);
        uint64_t id = next_id_++;
        auto type = std::type_index(typeid(Event));

        auto& handlers = handlers_[type];
        handlers.push_back({id, [handler](const void* event) {
            handler(*static_cast<const Event*>(event));
        }});

        return id;
    }

    /**
     * @brief Unsubscribe from events.
     * @param subscription_id ID returned from subscribe
     */
    void unsubscribe(uint64_t subscription_id) {
        std::unique_lock lock(mutex_);
        for (auto& [type, handlers] : handlers_) {
            handlers.erase(
                std::remove_if(handlers.begin(), handlers.end(),
                    [subscription_id](const auto& entry) {
                        return entry.id == subscription_id;
                    }),
                handlers.end());
        }
    }

    /**
     * @brief Publish an event to all subscribers.
     * @tparam Event Event type
     * @param event Event to publish
     */
    template<typename Event>
    void publish(const Event& event) {
        if (!is_running()) return;

        std::shared_lock lock(mutex_);
        auto type = std::type_index(typeid(Event));
        auto it = handlers_.find(type);
        if (it != handlers_.end()) {
            for (const auto& entry : it->second) {
                entry.handler(&event);
            }
        }
    }

    /**
     * @brief Get the number of subscribers for an event type.
     */
    template<typename Event>
    size_t subscriber_count() const {
        std::shared_lock lock(mutex_);
        auto type = std::type_index(typeid(Event));
        auto it = handlers_.find(type);
        if (it != handlers_.end()) {
            return it->second.size();
        }
        return 0;
    }

    /**
     * @brief Clear all subscriptions.
     */
    void clear() {
        std::unique_lock lock(mutex_);
        handlers_.clear();
    }

private:
    struct HandlerEntry {
        uint64_t id;
        std::function<void(const void*)> handler;
    };

    mutable std::shared_mutex mutex_;
    std::atomic<bool> running_;
    std::atomic<uint64_t> next_id_;
    std::unordered_map<std::type_index, std::vector<HandlerEntry>> handlers_;
};

// ============================================================================
// Result Helpers (re-exported from result partition)
// ============================================================================

// Note: Result<T> and related types are in the :result partition
// This partition focuses on design patterns like EventBus

} // namespace kcenon::common::patterns
