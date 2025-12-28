// BSD 3-Clause License
// Copyright (c) 2025, kcenon
// See the LICENSE file in the project root for full license information.

/**
 * @file transport.h
 * @brief C++20 concepts for transport client interfaces.
 *
 * This header provides concepts for validating HTTP and UDP client types
 * used in network communication. These concepts replace abstract class-based
 * constraints with compile-time validation and clearer error messages.
 *
 * Requirements:
 * - C++20 compiler with concepts support
 * - GCC 10+, Clang 10+, MSVC 2022+
 *
 * Thread Safety:
 * - Concepts are evaluated at compile-time only.
 * - No runtime thread-safety considerations apply.
 *
 * @see http_client_interface.h for IHttpClient definition
 * @see udp_client_interface.h for IUdpClient definition
 * @see https://en.cppreference.com/w/cpp/language/constraints
 */

#pragma once

#include <concepts>
#include <cstdint>
#include <span>
#include <string>
#include <type_traits>

namespace kcenon::common {

// Forward declarations
namespace interfaces {
struct http_request;
struct http_response;
struct udp_endpoint;
}  // namespace interfaces

namespace concepts {

/**
 * @concept HttpSender
 * @brief A type that can send HTTP requests.
 *
 * Types satisfying this concept can send HTTP requests and receive responses.
 * This is the core functionality of an HTTP client.
 *
 * Example usage:
 * @code
 * template<HttpSender H>
 * void fetch_data(H& client, const interfaces::http_request& req) {
 *     auto result = client.send(req);
 *     if (result.is_ok()) {
 *         // Process response
 *     }
 * }
 * @endcode
 */
template <typename T>
concept HttpSender = requires(T t, const interfaces::http_request& request) {
    { t.send(request) };
};

/**
 * @concept HttpAvailabilityChecker
 * @brief A type that can check HTTP client availability.
 *
 * Types satisfying this concept can report whether the HTTP client
 * is properly configured and available for making requests.
 *
 * Example usage:
 * @code
 * template<HttpAvailabilityChecker H>
 * bool can_make_requests(const H& client) {
 *     return client.is_available();
 * }
 * @endcode
 */
template <typename T>
concept HttpAvailabilityChecker = requires(const T t) {
    { t.is_available() } -> std::convertible_to<bool>;
};

/**
 * @concept HttpClientLike
 * @brief A complete HTTP client type satisfying IHttpClient interface.
 *
 * Types satisfying this concept provide full HTTP client functionality
 * including sending requests and availability checking.
 * This concept matches the IHttpClient interface contract.
 *
 * Example usage:
 * @code
 * template<HttpClientLike H>
 * auto make_request(H& client, const std::string& url) {
 *     if (!client.is_available()) {
 *         return make_error("HTTP client not available");
 *     }
 *     interfaces::http_request req{url};
 *     return client.send(req);
 * }
 * @endcode
 */
template <typename T>
concept HttpClientLike = HttpSender<T> && HttpAvailabilityChecker<T>;

/**
 * @concept HttpClientProviderLike
 * @brief A type that can provide HTTP client instances.
 *
 * Types satisfying this concept can create and retrieve HTTP clients,
 * enabling dependency injection for HTTP communication.
 *
 * Example usage:
 * @code
 * template<HttpClientProviderLike P>
 * auto get_client(P& provider) {
 *     return provider.get_http_client();
 * }
 * @endcode
 */
template <typename T>
concept HttpClientProviderLike = requires(T t) {
    { t.get_http_client() };
    { t.create_http_client() };
};

/**
 * @concept UdpConnectable
 * @brief A type that supports UDP connection operations.
 *
 * Types satisfying this concept can establish a "connected" UDP socket
 * for optimized sending to a specific endpoint.
 *
 * Example usage:
 * @code
 * template<UdpConnectable U>
 * void setup_metrics_client(U& client) {
 *     client.connect("metrics.example.com", 8125);
 * }
 * @endcode
 */
template <typename T>
concept UdpConnectable = requires(T t,
                                  const std::string& host,
                                  uint16_t port) {
    { t.connect(host, port) };
    { t.disconnect() } -> std::same_as<void>;
};

/**
 * @concept UdpSender
 * @brief A type that can send UDP datagrams.
 *
 * Types satisfying this concept can send data via UDP,
 * either to a connected endpoint or to a specified endpoint.
 *
 * Example usage:
 * @code
 * template<UdpSender U>
 * void send_metric(U& client, const std::string& data) {
 *     auto bytes = std::span<const uint8_t>(
 *         reinterpret_cast<const uint8_t*>(data.data()),
 *         data.size());
 *     client.send(bytes);
 * }
 * @endcode
 */
template <typename T>
concept UdpSender = requires(T t,
                             std::span<const uint8_t> data,
                             const interfaces::udp_endpoint& endpoint) {
    { t.send(data) };
    { t.send_to(data, endpoint) };
};

/**
 * @concept UdpConnectionStatus
 * @brief A type that can report UDP connection status.
 *
 * Types satisfying this concept can report whether they are
 * currently connected to a remote endpoint.
 *
 * Example usage:
 * @code
 * template<UdpConnectionStatus U>
 * bool ensure_connected(const U& client) {
 *     return client.is_connected();
 * }
 * @endcode
 */
template <typename T>
concept UdpConnectionStatus = requires(const T t) {
    { t.is_connected() } -> std::convertible_to<bool>;
};

/**
 * @concept UdpClientLike
 * @brief A complete UDP client type satisfying IUdpClient interface.
 *
 * Types satisfying this concept provide full UDP client functionality
 * including connection management, sending, and status reporting.
 * This concept matches the IUdpClient interface contract.
 *
 * Example usage:
 * @code
 * template<UdpClientLike U>
 * void send_statsd_metric(U& client, const std::string& metric) {
 *     if (!client.is_connected()) {
 *         client.connect("localhost", 8125);
 *     }
 *     client.send(metric);
 * }
 * @endcode
 */
template <typename T>
concept UdpClientLike = UdpConnectable<T> &&
                        UdpSender<T> &&
                        UdpConnectionStatus<T>;

/**
 * @concept UdpClientProviderLike
 * @brief A type that can provide UDP client instances.
 *
 * Types satisfying this concept can create and retrieve UDP clients,
 * enabling dependency injection for UDP communication.
 *
 * Example usage:
 * @code
 * template<UdpClientProviderLike P>
 * auto get_client(P& provider) {
 *     return provider.get_udp_client();
 * }
 * @endcode
 */
template <typename T>
concept UdpClientProviderLike = requires(T t) {
    { t.get_udp_client() };
    { t.create_udp_client() };
};

/**
 * @concept TransportClient
 * @brief A type that represents any transport client (HTTP or UDP).
 *
 * This is a utility concept for generic transport handling.
 *
 * Example usage:
 * @code
 * template<TransportClient T>
 * std::string get_transport_name(const T& client) {
 *     return client.get_implementation_name();
 * }
 * @endcode
 */
template <typename T>
concept TransportClient = HttpClientLike<T> || UdpClientLike<T>;

}  // namespace concepts
}  // namespace kcenon::common
