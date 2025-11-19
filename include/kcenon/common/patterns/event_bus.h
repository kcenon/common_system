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
 * This header provides a standalone event bus implementation that can be used
 * across modules without external dependencies. The simple_event_bus provides
 * a thread-safe, synchronous publish/subscribe mechanism for inter-module
 * communication.
 *
 * Other systems (like monitoring_system) can extend or wrap this implementation
 * if they need additional features like async processing or advanced filtering.
 *
 * Example:
 * @code
 * auto& bus = common::get_event_bus();
 * bus.publish(common::events::module_started_event{"network_system"});
 * @endcode
 */

#pragma once

#include <string>
#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <atomic>
#include <unordered_map>
#include <algorithm>
#include <vector>
#include <typeinfo>
#include <typeindex>

// Provide a simple standalone implementation
// Note: monitoring_system can extend or wrap this if needed

namespace kcenon::common {
namespace detail {
    // Version identifier for ABI compatibility checking
    // This ensures that object files compiled with different macro definitions
    // cannot be linked together, preventing subtle runtime errors.
    constexpr int event_bus_abi_version = 1;  // Standalone implementation

    /**
     * @brief Generate unique type ID for each event type using std::type_index
     *
     * IMPLEMENTATION:
     * ===============
     * This implementation uses std::type_index and std::hash to generate consistent
     * type IDs across all translation units. Unlike counter-based approaches, this
     * ensures that the same event type always receives the same ID regardless of
     * instantiation order.
     *
     * BENEFITS:
     * - Consistent IDs across translation units (TUs)
     * - No initialization order dependencies
     * - Type-safe without relying on manual registration
     * - Events published in one TU will correctly match handlers in another TU
     *
     * THREAD SAFETY:
     * - The hash is computed once per type and cached in a static variable
     * - C++11 guarantees thread-safe static local initialization
     * - No race conditions or synchronization needed
     *
     * COMPATIBILITY:
     * - Requires RTTI (typeid support)
     * - Hash collision risk is negligible (std::hash quality)
     * - IDs are stable across program runs on the same platform/compiler
     *
     * NOTE:
     * For RTTI-free environments, consider enabling ENABLE_MONITORING_INTEGRATION
     * which provides alternative type identification mechanisms.
     */
    template<typename T>
    struct event_type_id {
        static size_t id() {
            // Use std::type_index hash for deterministic, consistent IDs
            static const size_t type_id = std::hash<std::type_index>{}(std::type_index(typeid(T)));
            return type_id;
        }
    };
} // namespace detail

    // Public API - expose detail::event_type_id for backward compatibility
    using detail::event_type_id;

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

        /**
         * @brief Subscribe to events with a filter function
         * @tparam EventType The type of event to subscribe to
         * @tparam HandlerFunc The handler function type
         * @tparam FilterFunc The filter function type
         * @param func The handler function to call when the event passes the filter
         * @param filter The filter function to determine if the handler should be called
         * @return Subscription ID for later unsubscription
         *
         * @note The filter function is called before the handler. If it returns false,
         *       the handler is not invoked. This allows for efficient event filtering
         *       without creating multiple event types.
         */
        template<typename EventType, typename HandlerFunc, typename FilterFunc>
        uint64_t subscribe_filtered(HandlerFunc&& func, FilterFunc&& filter) {
            // Create a wrapped handler that includes the filtering logic
            auto wrapped_handler = [f = std::forward<HandlerFunc>(func),
                                   flt = std::forward<FilterFunc>(filter)]
                                  (const EventType& evt) {
                // Only call the handler if the filter passes
                if (flt(evt)) {
                    f(evt);
                }
            };

            // Subscribe with the wrapped handler
            return subscribe<EventType>(std::move(wrapped_handler));
        }

        // Non-template overload for generic event filtering
        uint64_t subscribe_filtered(
            std::function<void(const event&)>&& func,
            std::function<bool(const event&)>&& filter) {
            // Create a wrapped handler that includes the filtering logic
            auto wrapped_handler = [f = std::move(func),
                                   flt = std::move(filter)]
                                  (const event& evt) {
                // Only call the handler if the filter passes
                if (flt(evt)) {
                    f(evt);
                }
            };

            return subscribe(std::move(wrapped_handler));
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

// ABI compatibility check functions
namespace kcenon::common {
namespace detail {
    /**
     * @brief Get the ABI version of the event_bus implementation
     *
     * This function is used to detect ABI incompatibilities at link time.
     * The ABI version ensures that all linked modules use the same event_bus
     * implementation, preventing subtle runtime errors.
     *
     * @return The ABI version number (currently 1 for standalone implementation)
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
} // namespace kcenon::common

// Common event types that can be used across modules
namespace kcenon::common {
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
} // namespace kcenon::common
