/*****************************************************************************
BSD 3-Clause License

Copyright (c) 2025, 🍀☀🌕🌥 🌊
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its
   contributors may be used to endorse or promote products derived from
   this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*****************************************************************************/

/**
 * @file event_bus.h
 * @brief Event bus abstraction and common events.
 *
 * This header defines a uniform way to publish/subscribe events across
 * modules. When monitoring integration is enabled, it forwards to the
 * monitoring_system implementation; otherwise it provides a no-op stub
 * that keeps client code consistent.
 *
 * Example:
 * @code
 * auto bus = common::get_event_bus();
 * bus->publish(common::events::module_started_event{"network_system"});
 * @endcode
 */

#pragma once

#include <string>
#include <chrono>
#include <cstdint>
#include <functional>

// Check if monitoring system integration is enabled
#if defined(ENABLE_MONITORING_INTEGRATION) || defined(WITH_MONITORING_SYSTEM) || defined(USE_MONITORING_SYSTEM)

// Forward to the actual event bus implementation
// Note: Projects using this must have monitoring_system in their include path
#include <kcenon/monitoring/core/event_bus.h>
#include <kcenon/monitoring/interfaces/event_bus_interface.h>

// Convenience namespace alias
namespace common {
    using event_bus = monitoring_system::event_bus;
    using event_priority = monitoring_system::event_priority;
    using event_subscription = monitoring_system::event_subscription;

    // Type aliases for common usage
    template<typename T>
    using event_handler = std::function<void(const T&)>;

    /**
     * @brief Get the global event bus instance
     *
     * This provides a convenient way to access the event bus
     * without direct dependency on monitoring_system namespace.
     */
    /**
     * @brief Access the global event bus instance.
     */
    inline std::shared_ptr<monitoring_system::event_bus_interface> get_event_bus() {
        return monitoring_system::event_bus::instance();
    }
}

#else // Monitoring integration disabled

// Provide a simple implementation when monitoring is disabled
#include <functional>
#include <memory>
#include <any>
#include <mutex>
#include <atomic>
#include <unordered_map>
#include <typeindex>
#include <algorithm>

namespace common {

    enum class event_priority {
        low = 0,
        normal = 1,
        high = 2
    };

    /**
     * @brief Type alias for subscription ID
     */
    using subscription_id = uint64_t;

    /**
     * @struct event
     * @brief Generic event structure for the event bus
     */
    struct event {
        std::string type_;
        std::string data_;

        event() = default;
        event(const std::string& type, const std::string& data = "")
            : type_(type), data_(data) {}

        void set_type(const std::string& type) { type_ = type; }
        void set_data(const std::string& data) { data_ = data; }
        std::string get_type() const { return type_; }
        std::string get_data() const { return data_; }
    };

    /**
     * @class simple_event_bus
     * @brief Simple synchronous event bus for testing when monitoring is disabled.
     *
     * Thread-safety: This class is thread-safe using a mutex to protect
     * subscription management and event dispatch.
     */
    class simple_event_bus {
    public:
        template<typename EventType>
        void publish(const EventType& evt, event_priority = event_priority::normal) {
            std::lock_guard<std::mutex> lock(mutex_);

            // Get type identifier
            auto type_id = std::type_index(typeid(EventType));

            // Find and call all handlers for this event type
            auto range = handlers_.equal_range(type_id);
            for (auto it = range.first; it != range.second; ++it) {
                try {
                    // Cast and invoke the handler
                    auto handler = std::any_cast<std::function<void(const EventType&)>>(it->second.handler);
                    handler(evt);
                } catch (...) {
                    // Ignore handler errors
                }
            }
        }

        // For generic event (overload for type deduction)
        void publish(event&& evt, event_priority = event_priority::normal) {
            std::lock_guard<std::mutex> lock(mutex_);

            auto type_id = std::type_index(typeid(event));
            auto range = handlers_.equal_range(type_id);
            for (auto it = range.first; it != range.second; ++it) {
                try {
                    auto handler = std::any_cast<std::function<void(const event&)>>(it->second.handler);
                    handler(evt);
                } catch (...) {
                    // Ignore handler errors
                }
            }
        }

        template<typename EventType, typename HandlerFunc>
        uint64_t subscribe(HandlerFunc&& func) {
            std::lock_guard<std::mutex> lock(mutex_);

            auto id = next_id_++;
            auto type_id = std::type_index(typeid(EventType));

            subscription_info info;
            info.id = id;
            info.handler = std::function<void(const EventType&)>(std::forward<HandlerFunc>(func));

            handlers_.emplace(type_id, std::move(info));

            return id;
        }

        // Non-template overload for generic event
        uint64_t subscribe(std::function<void(const event&)>&& func) {
            std::lock_guard<std::mutex> lock(mutex_);

            auto id = next_id_++;
            auto type_id = std::type_index(typeid(event));

            subscription_info info;
            info.id = id;
            info.handler = std::move(func);

            handlers_.emplace(type_id, std::move(info));

            return id;
        }

        template<typename EventType, typename HandlerFunc, typename FilterFunc>
        uint64_t subscribe_filtered(HandlerFunc&& func, FilterFunc&& filter) {
            // For simplicity, ignore filtering in this lightweight implementation
            return subscribe<EventType>(std::forward<HandlerFunc>(func));
        }

        // Non-template overload for generic event filtering
        uint64_t subscribe_filtered(
            std::function<void(const event&)>&& func,
            std::function<bool(const event&)>&&) {
            return subscribe(std::move(func));
        }

        void unsubscribe(uint64_t id) {
            std::lock_guard<std::mutex> lock(mutex_);

            // Find and remove the handler with this ID
            for (auto it = handlers_.begin(); it != handlers_.end(); ) {
                if (it->second.id == id) {
                    it = handlers_.erase(it);
                } else {
                    ++it;
                }
            }
        }

        void start() { running_ = true; }
        void stop() { running_ = false; }
        bool is_running() const { return running_; }

        /**
         * @brief Get the singleton instance (thread-safe via C++11 magic statics)
         * @return Reference to the singleton instance
         */
        static simple_event_bus& instance() {
            static simple_event_bus instance;
            return instance;
        }

    private:
        struct subscription_info {
            uint64_t id;
            std::any handler;
        };

        mutable std::mutex mutex_;
        std::unordered_multimap<std::type_index, subscription_info> handlers_;
        std::atomic<uint64_t> next_id_{1};
        std::atomic<bool> running_{true};
    };

    using event_bus = simple_event_bus;

    template<typename T>
    using event_handler = std::function<void(const T&)>;

    /**
     * @brief Access the global event bus instance.
     * @return Reference to the singleton event bus
     */
    inline simple_event_bus& get_event_bus() {
        return simple_event_bus::instance();
    }
}

#endif // ENABLE_MONITORING_INTEGRATION

// Common event types that can be used across modules
namespace common {
namespace events {

/**
 * @struct module_started_event
 * @brief Event published when a module starts.
 */
struct module_started_event {
    std::string module_name;
    std::chrono::steady_clock::time_point timestamp;

    module_started_event(const std::string& name)
        : module_name(name),
          timestamp(std::chrono::steady_clock::now()) {}
};

/**
 * @struct module_stopped_event
 * @brief Event published when a module stops.
 */
struct module_stopped_event {
    std::string module_name;
    std::chrono::steady_clock::time_point timestamp;

    module_stopped_event(const std::string& name)
        : module_name(name),
          timestamp(std::chrono::steady_clock::now()) {}
};

/**
 * @struct error_event
 * @brief Event published when an error occurs.
 */
struct error_event {
    std::string module_name;
    std::string error_message;
    int error_code;
    std::chrono::steady_clock::time_point timestamp;

    error_event(const std::string& module, const std::string& message, int code)
        : module_name(module), error_message(message), error_code(code),
          timestamp(std::chrono::steady_clock::now()) {}
};

/**
 * @struct metric_event
 * @brief Event for publishing metrics.
 */
struct metric_event {
    std::string name;
    double value;
    std::string unit;
    std::chrono::steady_clock::time_point timestamp;

    metric_event(const std::string& n, double v, const std::string& u = "")
        : name(n), value(v), unit(u),
          timestamp(std::chrono::steady_clock::now()) {}
};

} // namespace events
} // namespace common
