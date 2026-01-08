// BSD 3-Clause License
// Copyright (c) 2025, kcenon
// See the LICENSE file in the project root for full license information.

/**
 * @file udp_client_interface.h
 * @brief UDP client interface for transport abstraction.
 *
 * This header defines the abstract UDP client interface that enables
 * dependency injection for UDP communication across the ecosystem.
 * Primary use cases include metric reporting (StatsD, Prometheus) and
 * low-latency message delivery.
 */

#pragma once

#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <vector>

#include <kcenon/common/patterns/result.h>

namespace kcenon::common {
namespace interfaces {

/**
 * @struct udp_endpoint
 * @brief Represents a UDP endpoint (host and port)
 */
struct udp_endpoint {
    /// Hostname or IP address
    std::string host;

    /// Port number
    uint16_t port = 0;

    udp_endpoint() = default;

    udp_endpoint(std::string host_, uint16_t port_)
        : host(std::move(host_)), port(port_) {}

    /**
     * @brief Convert to string representation
     * @return String in format "host:port"
     */
    [[nodiscard]] std::string to_string() const {
        return host + ":" + std::to_string(port);
    }

    bool operator==(const udp_endpoint& other) const {
        return host == other.host && port == other.port;
    }

    bool operator!=(const udp_endpoint& other) const {
        return !(*this == other);
    }
};

/**
 * @struct udp_send_options
 * @brief Options for UDP send operations
 */
struct udp_send_options {
    /// Whether to set the don't-fragment flag (if supported)
    bool dont_fragment = false;

    /// TTL (Time To Live) value, 0 means use system default
    uint8_t ttl = 0;

    /// Send timeout (0 means no timeout / non-blocking)
    std::chrono::milliseconds timeout{0};

    udp_send_options() = default;
};

/**
 * @struct udp_statistics
 * @brief Statistics for UDP client operations
 */
struct udp_statistics {
    /// Total number of packets sent
    uint64_t packets_sent = 0;

    /// Total number of bytes sent
    uint64_t bytes_sent = 0;

    /// Number of failed send operations
    uint64_t send_failures = 0;

    /// Timestamp of last successful send
    std::optional<std::chrono::steady_clock::time_point> last_send_time;

    /**
     * @brief Reset all statistics
     */
    void reset() {
        packets_sent = 0;
        bytes_sent = 0;
        send_failures = 0;
        last_send_time = std::nullopt;
    }
};

/**
 * @interface IUdpClient
 * @brief Abstract interface for UDP client implementations
 *
 * This interface defines the contract for UDP client implementations,
 * allowing modules to send UDP datagrams without direct dependencies
 * on specific networking libraries (e.g., ASIO, socket API, etc.).
 *
 * @note UDP is connectionless, but this interface provides optional
 * "connected" mode for performance optimization when sending multiple
 * datagrams to the same endpoint.
 *
 * @note Implementations should be thread-safe for concurrent sends.
 *
 * Example usage:
 * @code
 * // Connect to a metrics server
 * auto result = client->connect("metrics.example.com", 8125);
 * if (result.is_ok()) {
 *     // Send StatsD-formatted metric
 *     std::string metric = "app.requests.count:1|c";
 *     client->send(std::span<const uint8_t>(
 *         reinterpret_cast<const uint8_t*>(metric.data()),
 *         metric.size()));
 * }
 * @endcode
 */
class IUdpClient {
public:
    virtual ~IUdpClient() = default;

    /**
     * @brief Connect to a remote endpoint for optimized sending
     *
     * This establishes a "connected" UDP socket, which allows the kernel
     * to cache routing information and perform error checking.
     *
     * @param host Remote hostname or IP address
     * @param port Remote port number
     * @return VoidResult indicating success or error
     */
    virtual ::kcenon::common::VoidResult connect(const std::string& host, uint16_t port) = 0;

    /**
     * @brief Connect to a remote endpoint
     * @param endpoint Remote endpoint
     * @return VoidResult indicating success or error
     */
    virtual ::kcenon::common::VoidResult connect(const udp_endpoint& endpoint) {
        return connect(endpoint.host, endpoint.port);
    }

    /**
     * @brief Send data to the connected endpoint
     *
     * Requires a prior successful call to connect().
     *
     * @param data Data to send
     * @return VoidResult indicating success or error
     */
    virtual ::kcenon::common::VoidResult send(std::span<const uint8_t> data) = 0;

    /**
     * @brief Send data to the connected endpoint with options
     *
     * @param data Data to send
     * @param options Send options
     * @return VoidResult indicating success or error
     */
    virtual ::kcenon::common::VoidResult send(std::span<const uint8_t> data,
                            const udp_send_options& options) {
        // Default implementation ignores options
        (void)options;
        return send(data);
    }

    /**
     * @brief Send data to a specific endpoint (connectionless)
     *
     * Sends data without requiring a prior connect() call.
     *
     * @param data Data to send
     * @param endpoint Target endpoint
     * @return VoidResult indicating success or error
     */
    virtual ::kcenon::common::VoidResult send_to(std::span<const uint8_t> data,
                               const udp_endpoint& endpoint) = 0;

    /**
     * @brief Check if the client is connected to an endpoint
     * @return true if connected, false otherwise
     */
    [[nodiscard]] virtual bool is_connected() const = 0;

    /**
     * @brief Get the currently connected endpoint
     * @return Optional containing endpoint if connected
     */
    [[nodiscard]] virtual std::optional<udp_endpoint> get_remote_endpoint() const {
        return std::nullopt;
    }

    /**
     * @brief Disconnect from the current endpoint
     *
     * After disconnecting, send() will fail until connect() is called again.
     * send_to() can still be used for connectionless sends.
     */
    virtual void disconnect() = 0;

    /**
     * @brief Get send statistics
     * @return Current statistics
     */
    [[nodiscard]] virtual udp_statistics get_statistics() const {
        return {};
    }

    /**
     * @brief Reset statistics
     */
    virtual void reset_statistics() {}

    /**
     * @brief Get the implementation name for logging/debugging
     * @return Implementation identifier string
     */
    [[nodiscard]] virtual std::string get_implementation_name() const {
        return "IUdpClient";
    }

    // Convenience methods for string data

    /**
     * @brief Send string data to the connected endpoint
     * @param data String to send
     * @return VoidResult indicating success or error
     */
    ::kcenon::common::VoidResult send(const std::string& data) {
        return send(std::span<const uint8_t>(
            reinterpret_cast<const uint8_t*>(data.data()),
            data.size()));
    }

    /**
     * @brief Send string data to a specific endpoint
     * @param data String to send
     * @param endpoint Target endpoint
     * @return VoidResult indicating success or error
     */
    ::kcenon::common::VoidResult send_to(const std::string& data, const udp_endpoint& endpoint) {
        return send_to(std::span<const uint8_t>(
            reinterpret_cast<const uint8_t*>(data.data()),
            data.size()), endpoint);
    }
};

/**
 * @class null_udp_client
 * @brief Null implementation for when UDP transport is disabled
 *
 * This implementation always returns errors, useful for testing
 * or when UDP functionality is intentionally disabled.
 */
class null_udp_client : public IUdpClient {
public:
    ::kcenon::common::VoidResult connect(const std::string& /*host*/, uint16_t /*port*/) override {
        return ::kcenon::common::VoidResult(::kcenon::common::error_info{
            ::kcenon::common::error_codes::NOT_INITIALIZED,
            "UDP client not available",
            "null_udp_client"});
    }

    ::kcenon::common::VoidResult send(std::span<const uint8_t> /*data*/) override {
        return ::kcenon::common::VoidResult(::kcenon::common::error_info{
            ::kcenon::common::error_codes::NOT_INITIALIZED,
            "UDP client not available",
            "null_udp_client"});
    }

    ::kcenon::common::VoidResult send_to(std::span<const uint8_t> /*data*/,
                       const udp_endpoint& /*endpoint*/) override {
        return ::kcenon::common::VoidResult(::kcenon::common::error_info{
            ::kcenon::common::error_codes::NOT_INITIALIZED,
            "UDP client not available",
            "null_udp_client"});
    }

    [[nodiscard]] bool is_connected() const override {
        return false;
    }

    void disconnect() override {}

    [[nodiscard]] std::string get_implementation_name() const override {
        return "null_udp_client";
    }
};

/**
 * @brief Factory function type for creating UDP client instances
 */
using UdpClientFactory = std::function<std::shared_ptr<IUdpClient>()>;

/**
 * @interface IUdpClientProvider
 * @brief Interface for modules that provide UDP client implementations
 */
class IUdpClientProvider {
public:
    virtual ~IUdpClientProvider() = default;

    /**
     * @brief Get the default UDP client instance
     * @return Shared pointer to the UDP client
     */
    virtual std::shared_ptr<IUdpClient> get_udp_client() = 0;

    /**
     * @brief Create a new UDP client
     * @return Shared pointer to the new UDP client
     */
    virtual std::shared_ptr<IUdpClient> create_udp_client() = 0;
};

}  // namespace interfaces
}  // namespace kcenon::common
