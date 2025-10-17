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

// Provide a stub implementation when monitoring is disabled
#include <functional>
#include <memory>
#include <any>

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
     * @class null_event_bus
     * @brief No-op event bus used when monitoring is disabled.
     *
     * Thread-safety: This class is thread-safe. All methods are no-ops
     * that perform no state modifications. The singleton instance()
     * uses C++11 magic statics for thread-safe initialization.
     */
    class null_event_bus {
    public:
        template<typename EventType>
        void publish(const EventType&, event_priority = event_priority::normal) {
            // No-op - thread-safe as it performs no operations
        }

        // For generic event (overload for type deduction)
        void publish(event&&, event_priority = event_priority::normal) {
            // No-op - thread-safe as it performs no operations
        }

        template<typename EventType, typename HandlerFunc>
        uint64_t subscribe(HandlerFunc&&) {
            return 0; // Dummy subscription ID - thread-safe as it's stateless
        }

        // Non-template overload for generic event (enables type deduction)
        uint64_t subscribe(std::function<void(const event&)>&&) {
            return 0; // Dummy subscription ID - thread-safe as it's stateless
        }

        template<typename EventType, typename HandlerFunc, typename FilterFunc>
        uint64_t subscribe_filtered(HandlerFunc&&, FilterFunc&&) {
            return 0; // Dummy subscription ID - thread-safe as it's stateless
        }

        // Non-template overload for generic event filtering
        uint64_t subscribe_filtered(
            std::function<void(const event&)>&&,
            std::function<bool(const event&)>&&) {
            return 0; // Dummy subscription ID - thread-safe as it's stateless
        }

        void unsubscribe(uint64_t) {
            // No-op - thread-safe as it performs no operations
        }

        void start() {}
        void stop() {}
        bool is_running() const { return false; }

        /**
         * @brief Get the singleton instance (thread-safe via C++11 magic statics)
         * @return Reference to the singleton instance
         */
        static null_event_bus& instance() {
            static null_event_bus instance;
            return instance;
        }
    };

    using event_bus = null_event_bus;

    template<typename T>
    using event_handler = std::function<void(const T&)>;

    /**
     * @brief Access the (no-op) global event bus instance.
     * @return Reference to the singleton event bus
     */
    inline null_event_bus& get_event_bus() {
        return null_event_bus::instance();
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
