// BSD 3-Clause License
// Copyright (c) 2025, kcenon
// See the LICENSE file in the project root for full license information.

/**
 * @file http_client_interface.h
 * @brief HTTP client interface for transport abstraction.
 *
 * This header defines the abstract HTTP client interface that enables
 * dependency injection for HTTP communication across the ecosystem.
 * Implementations can be provided by network_system or monitoring_system.
 */

#pragma once

#include <chrono>
#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <vector>

#include <kcenon/common/patterns/result.h>

namespace kcenon::common {
namespace interfaces {

/**
 * @brief HTTP headers container type
 */
using http_headers = std::map<std::string, std::string>;

/**
 * @struct http_request
 * @brief Represents an HTTP request
 */
struct http_request {
    /// Request URL (must include scheme, e.g., "https://api.example.com/v1/data")
    std::string url;

    /// HTTP method (GET, POST, PUT, DELETE, PATCH, HEAD, OPTIONS)
    std::string method = "GET";

    /// Request headers
    http_headers headers;

    /// Request body (for POST, PUT, PATCH)
    std::vector<uint8_t> body;

    /// Request timeout (default: 30 seconds)
    std::chrono::milliseconds timeout{30000};

    /// Whether to follow redirects (default: true)
    bool follow_redirects = true;

    /// Maximum number of redirects to follow (default: 5)
    int max_redirects = 5;

    // Convenience constructors

    http_request() = default;

    explicit http_request(std::string url_)
        : url(std::move(url_)) {}

    http_request(std::string url_, std::string method_)
        : url(std::move(url_)), method(std::move(method_)) {}

    http_request(std::string url_, std::string method_, http_headers headers_)
        : url(std::move(url_))
        , method(std::move(method_))
        , headers(std::move(headers_)) {}

    http_request(std::string url_,
                 std::string method_,
                 http_headers headers_,
                 std::vector<uint8_t> body_)
        : url(std::move(url_))
        , method(std::move(method_))
        , headers(std::move(headers_))
        , body(std::move(body_)) {}

    /**
     * @brief Set Content-Type header
     * @param content_type MIME type string
     * @return Reference to this for chaining
     */
    http_request& set_content_type(const std::string& content_type) {
        headers["Content-Type"] = content_type;
        return *this;
    }

    /**
     * @brief Set Authorization header
     * @param auth Authorization value (e.g., "Bearer token123")
     * @return Reference to this for chaining
     */
    http_request& set_authorization(const std::string& auth) {
        headers["Authorization"] = auth;
        return *this;
    }

    /**
     * @brief Set body from string
     * @param str String body content
     * @return Reference to this for chaining
     */
    http_request& set_body(const std::string& str) {
        body.assign(str.begin(), str.end());
        return *this;
    }
};

/**
 * @struct http_response
 * @brief Represents an HTTP response
 */
struct http_response {
    /// HTTP status code (e.g., 200, 404, 500)
    int status_code = 0;

    /// Status reason phrase (e.g., "OK", "Not Found")
    std::string reason_phrase;

    /// Response headers
    http_headers headers;

    /// Response body
    std::vector<uint8_t> body;

    /// Time taken to receive the response
    std::chrono::milliseconds elapsed{0};

    /// Final URL after redirects (may differ from request URL)
    std::optional<std::string> final_url;

    // Convenience methods

    /**
     * @brief Check if response indicates success (2xx status)
     * @return true if status code is in 200-299 range
     */
    [[nodiscard]] bool is_success() const {
        return status_code >= 200 && status_code < 300;
    }

    /**
     * @brief Check if response indicates client error (4xx status)
     * @return true if status code is in 400-499 range
     */
    [[nodiscard]] bool is_client_error() const {
        return status_code >= 400 && status_code < 500;
    }

    /**
     * @brief Check if response indicates server error (5xx status)
     * @return true if status code is in 500-599 range
     */
    [[nodiscard]] bool is_server_error() const {
        return status_code >= 500 && status_code < 600;
    }

    /**
     * @brief Get body as string
     * @return Body content as string
     */
    [[nodiscard]] std::string body_as_string() const {
        return std::string(body.begin(), body.end());
    }

    /**
     * @brief Get a specific header value
     * @param name Header name (case-insensitive matching recommended by caller)
     * @return Optional containing header value if found
     */
    [[nodiscard]] std::optional<std::string> get_header(
        const std::string& name) const {
        auto it = headers.find(name);
        if (it != headers.end()) {
            return it->second;
        }
        return std::nullopt;
    }
};

/**
 * @interface IHttpClient
 * @brief Abstract interface for HTTP client implementations
 *
 * This interface defines the contract for HTTP client implementations,
 * allowing modules to make HTTP requests without direct dependencies
 * on specific HTTP libraries (e.g., libcurl, ASIO, etc.).
 *
 * @note Implementations should be thread-safe for concurrent requests.
 *
 * Example usage:
 * @code
 * // Synchronous request
 * http_request req{"https://api.example.com/data"};
 * req.set_content_type("application/json");
 *
 * auto result = client->send(req);
 * if (result.is_ok()) {
 *     auto& response = result.value();
 *     if (response.is_success()) {
 *         std::cout << response.body_as_string() << std::endl;
 *     }
 * }
 * @endcode
 */
class IHttpClient {
public:
    virtual ~IHttpClient() = default;

    /**
     * @brief Send an HTTP request synchronously
     * @param request The HTTP request to send
     * @return Result containing response or error information
     */
    virtual Result<http_response> send(const http_request& request) = 0;

    /**
     * @brief Check if the HTTP client is available and properly configured
     * @return true if client can make requests, false otherwise
     */
    [[nodiscard]] virtual bool is_available() const = 0;

    /**
     * @brief Get the implementation name for logging/debugging
     * @return Implementation identifier string
     */
    [[nodiscard]] virtual std::string get_implementation_name() const {
        return "IHttpClient";
    }
};

/**
 * @class null_http_client
 * @brief Null implementation for when HTTP transport is disabled
 *
 * This implementation always returns errors, useful for testing
 * or when HTTP functionality is intentionally disabled.
 */
class null_http_client : public IHttpClient {
public:
    Result<http_response> send(const http_request& /*request*/) override {
        return make_error<http_response>(
            error_codes::NOT_INITIALIZED,
            "HTTP client not available",
            "null_http_client");
    }

    [[nodiscard]] bool is_available() const override {
        return false;
    }

    [[nodiscard]] std::string get_implementation_name() const override {
        return "null_http_client";
    }
};

/**
 * @brief Factory function type for creating HTTP client instances
 */
using HttpClientFactory = std::function<std::shared_ptr<IHttpClient>()>;

/**
 * @interface IHttpClientProvider
 * @brief Interface for modules that provide HTTP client implementations
 */
class IHttpClientProvider {
public:
    virtual ~IHttpClientProvider() = default;

    /**
     * @brief Get the default HTTP client instance
     * @return Shared pointer to the HTTP client
     */
    virtual std::shared_ptr<IHttpClient> get_http_client() = 0;

    /**
     * @brief Create a new HTTP client with specific configuration
     * @param timeout Default timeout for all requests
     * @return Shared pointer to the new HTTP client
     */
    virtual std::shared_ptr<IHttpClient> create_http_client(
        std::chrono::milliseconds timeout = std::chrono::milliseconds{30000}) = 0;
};

}  // namespace interfaces
}  // namespace kcenon::common
