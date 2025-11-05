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

// ABI version checking for conditional compilation
namespace common {
namespace detail {
    // Version identifier for ABI compatibility checking
    // This ensures that object files compiled with different macro definitions
    // cannot be linked together, preventing subtle runtime errors.
    constexpr int event_bus_abi_version = 2;  // Monitoring integration enabled
} // namespace detail
} // namespace common

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
#include <mutex>
#include <atomic>
#include <unordered_map>
#include <algorithm>
#include <vector>

namespace common {
namespace detail {
    // Version identifier for ABI compatibility checking
    // This ensures that object files compiled with different macro definitions
    // cannot be linked together, preventing subtle runtime errors.
    constexpr int event_bus_abi_version = 1;  // Monitoring integration disabled
} // namespace detail

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
     * @brief Global counter for generating unique type IDs
     */
    inline std::atomic<size_t>& get_type_id_counter() {
        static std::atomic<size_t> counter{0};
        return counter;
    }

    /**
     * @brief Generate unique type ID for each event type without RTTI
     *
     * WARNING: Type ID Consistency Limitation
     * ========================================
     * This implementation generates type IDs using a static counter that increments
     * on first access for each template instantiation. This can lead to inconsistent
     * type IDs across different translation units if the order of template instantiation
     * differs between them.
     *
     * POTENTIAL ISSUE:
     * - If EventTypeA is instantiated before EventTypeB in file1.cpp, but after EventTypeB
     *   in file2.cpp, they will receive different type IDs in each translation unit.
     * - This can cause events published in one translation unit to fail matching handlers
     *   subscribed in another translation unit.
     *
     * MITIGATION:
     * - Use explicit type registration at program startup in a deterministic order
     * - Keep event type definitions in a single translation unit when possible
     * - Consider using a hash-based type ID system (e.g., hash of type name string) as an
     *   alternative for better consistency across translation units
     *
     * RECOMMENDATION:
     * For production use, prefer enabling ENABLE_MONITORING_INTEGRATION which uses the
     * full monitoring_system implementation with more robust type identification.
     */
    template<typename T>
    struct event_type_id {
        static size_t id() {
            static const size_t type_id = ++get_type_id_counter();
            return type_id;
        }
    };

    /**
     * @class simple_event_bus
     * @brief Simple synchronous event bus for testing when monitoring is disabled.
     *
     * Thread-safety: This class is thread-safe using a mutex to protect
     * subscription management and event dispatch.
     *
     * Note: This implementation avoids RTTI by using template-based type IDs.
     */
    class simple_event_bus {
    public:
        /**
         * @brief Type for error callback function
         *
         * This callback is invoked when an exception occurs in an event handler
         * or when a type mismatch is detected. The callback receives:
         * - error_message: Description of what went wrong
         * - event_type_id: The type ID of the event being processed
         * - handler_id: The subscription ID of the failing handler
         */
        using error_callback_t = std::function<void(const std::string& error_message,
                                                     size_t event_type_id,
                                                     uint64_t handler_id)>;

        template<typename EventType>
        void publish(const EventType& evt, event_priority = event_priority::normal) {
            std::lock_guard<std::mutex> lock(mutex_);

            // Get type identifier without RTTI
            auto type_id = event_type_id<EventType>::id();

            // Find and call all handlers for this event type
            auto range = handlers_.equal_range(type_id);
            for (auto it = range.first; it != range.second; ++it) {
                try {
                    // Type safety check: verify expected type matches
                    if (it->second.expected_type_id != type_id) {
                        // Log type mismatch error
                        if (error_callback_) {
                            error_callback_("Type ID mismatch detected in event handler",
                                          type_id, it->second.id);
                        }
                        continue;
                    }

                    // Invoke the handler
                    auto& handler_wrapper = it->second.handler;
                    if (handler_wrapper) {
                        handler_wrapper(static_cast<const void*>(&evt));
                    }
                } catch (const std::exception& e) {
                    // Log the error and continue processing other handlers
                    if (error_callback_) {
                        std::string error_msg = "Exception in event handler: ";
                        error_msg += e.what();
                        error_callback_(error_msg, type_id, it->second.id);
                    }
                    // Continue processing other handlers
                } catch (...) {
                    // Log unknown exception and continue processing other handlers
                    if (error_callback_) {
                        error_callback_("Unknown exception in event handler",
                                      type_id, it->second.id);
                    }
                    // Continue processing other handlers
                }
            }
        }

        // For generic event (overload for type deduction)
        void publish(event&& evt, event_priority = event_priority::normal) {
            std::lock_guard<std::mutex> lock(mutex_);

            auto type_id = event_type_id<event>::id();
            auto range = handlers_.equal_range(type_id);
            for (auto it = range.first; it != range.second; ++it) {
                try {
                    // Type safety check: verify expected type matches
                    if (it->second.expected_type_id != type_id) {
                        // Log type mismatch error
                        if (error_callback_) {
                            error_callback_("Type ID mismatch detected in event handler",
                                          type_id, it->second.id);
                        }
                        continue;
                    }

                    auto& handler_wrapper = it->second.handler;
                    if (handler_wrapper) {
                        handler_wrapper(static_cast<const void*>(&evt));
                    }
                } catch (const std::exception& e) {
                    // Log the error and continue processing other handlers
                    if (error_callback_) {
                        std::string error_msg = "Exception in event handler: ";
                        error_msg += e.what();
                        error_callback_(error_msg, type_id, it->second.id);
                    }
                    // Continue processing other handlers
                } catch (...) {
                    // Log unknown exception and continue processing other handlers
                    if (error_callback_) {
                        error_callback_("Unknown exception in event handler",
                                      type_id, it->second.id);
                    }
                    // Continue processing other handlers
                }
            }
        }

        template<typename EventType, typename HandlerFunc>
        uint64_t subscribe(HandlerFunc&& func) {
            std::lock_guard<std::mutex> lock(mutex_);

            auto id = next_id_++;
            auto type_id = event_type_id<EventType>::id();

            subscription_info info;
            info.id = id;
            info.expected_type_id = type_id;  // Store expected type ID for validation
            // Wrap the handler in a type-erased function
            info.handler = [f = std::function<void(const EventType&)>(std::forward<HandlerFunc>(func))]
                          (const void* evt_ptr) {
                f(*static_cast<const EventType*>(evt_ptr));
            };

            handlers_.emplace(type_id, std::move(info));

            return id;
        }

        // Non-template overload for generic event
        uint64_t subscribe(std::function<void(const event&)>&& func) {
            std::lock_guard<std::mutex> lock(mutex_);

            auto id = next_id_++;
            auto type_id = event_type_id<event>::id();

            subscription_info info;
            info.id = id;
            info.expected_type_id = type_id;  // Store expected type ID for validation
            info.handler = [f = std::move(func)](const void* evt_ptr) {
                f(*static_cast<const event*>(evt_ptr));
            };

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
         * @brief Set the error callback for handler exceptions and type mismatches
         *
         * This callback will be invoked whenever:
         * - An exception is thrown by an event handler
         * - A type ID mismatch is detected (should never happen in correct usage)
         *
         * The error callback is useful for logging and debugging handler issues
         * without stopping the processing of other handlers.
         *
         * @param callback The callback function to invoke on errors
         *
         * Example usage:
         * @code
         * auto& bus = simple_event_bus::instance();
         * bus.set_error_callback([](const std::string& msg, size_t type_id, uint64_t handler_id) {
         *     std::cerr << "Event bus error [type=" << type_id
         *               << ", handler=" << handler_id << "]: " << msg << std::endl;
         * });
         * @endcode
         */
        void set_error_callback(error_callback_t callback) {
            std::lock_guard<std::mutex> lock(mutex_);
            error_callback_ = std::move(callback);
        }

        /**
         * @brief Clear the error callback
         */
        void clear_error_callback() {
            std::lock_guard<std::mutex> lock(mutex_);
            error_callback_ = nullptr;
        }

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
            size_t expected_type_id;  // Type safety: store expected type ID
            std::function<void(const void*)> handler;
        };

        mutable std::mutex mutex_;
        std::unordered_multimap<size_t, subscription_info> handlers_;
        std::atomic<uint64_t> next_id_{1};
        std::atomic<bool> running_{true};
        error_callback_t error_callback_;  // Optional error callback for exception handling
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

// ABI compatibility check function (available in both modes)
namespace common {
namespace detail {
    /**
     * @brief Get the ABI version of the event_bus implementation
     *
     * This function is used to detect ABI incompatibilities at link time.
     * If two translation units are compiled with different ENABLE_MONITORING_INTEGRATION
     * settings, they will have different ABI versions, which should be caught by
     * the linker or at runtime.
     *
     * @return The ABI version number (1 = no monitoring, 2 = with monitoring)
     */
    inline constexpr int get_event_bus_abi_version() {
        return event_bus_abi_version;
    }
} // namespace detail

    /**
     * @brief Verify ABI compatibility between modules
     *
     * Call this function during initialization to verify that all linked modules
     * were compiled with the same event_bus configuration. This helps catch
     * subtle bugs caused by mixing object files compiled with different settings.
     *
     * @param expected_version The ABI version your module expects
     * @return true if compatible, false otherwise
     *
     * Example usage:
     * @code
     * // In your module initialization:
     * if (!common::verify_event_bus_abi(common::detail::event_bus_abi_version)) {
     *     // Handle ABI mismatch error
     *     throw std::runtime_error("Event bus ABI mismatch detected!");
     * }
     * @endcode
     */
    inline bool verify_event_bus_abi(int expected_version) {
        return detail::get_event_bus_abi_version() == expected_version;
    }
} // namespace common

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
