/**
 * @file event_bus.h
 * @brief Forwarding header for the monitoring system's event bus
 *
 * This header provides a standardized way to access the event bus
 * functionality across all modules, enabling loose coupling through
 * publish-subscribe patterns.
 *
 * Usage:
 *   #include <common/event_bus.h>
 *
 *   // Publishing an event
 *   auto bus = monitoring_system::event_bus::instance();
 *   bus->publish(my_event);
 *
 *   // Subscribing to events
 *   bus->subscribe<MyEventType>([](const MyEventType& event) {
 *       // Handle event
 *   });
 */

#pragma once

#include <string>
#include <chrono>
#include <cstdint>

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
     * @class null_event_bus
     * @brief No-op event bus implementation for when monitoring is disabled
     */
    class null_event_bus {
    public:
        template<typename EventType>
        void publish(const EventType&, event_priority = event_priority::normal) {
            // No-op
        }

        template<typename EventType>
        uint64_t subscribe(std::function<void(const EventType&)>) {
            return 0; // Dummy subscription ID
        }

        void unsubscribe(uint64_t) {
            // No-op
        }

        void start() {}
        void stop() {}
        bool is_running() const { return false; }

        static std::shared_ptr<null_event_bus> instance() {
            static auto instance = std::make_shared<null_event_bus>();
            return instance;
        }
    };

    using event_bus = null_event_bus;

    template<typename T>
    using event_handler = std::function<void(const T&)>;

    inline std::shared_ptr<null_event_bus> get_event_bus() {
        return null_event_bus::instance();
    }
}

#endif // ENABLE_MONITORING_INTEGRATION

// Common event types that can be used across modules
namespace common {
namespace events {

/**
 * @struct module_started_event
 * @brief Event published when a module starts
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
 * @brief Event published when a module stops
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
 * @brief Event published when an error occurs
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
 * @brief Event for publishing metrics
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
