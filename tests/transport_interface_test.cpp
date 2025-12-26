// BSD 3-Clause License
// Copyright (c) 2025, kcenon
// See the LICENSE file in the project root for full license information.

/**
 * @file transport_interface_test.cpp
 * @brief Unit tests for transport interfaces (IHttpClient, IUdpClient)
 */

#include <gtest/gtest.h>
#include <kcenon/common/interfaces/transport.h>

#include <memory>
#include <string>
#include <vector>

using namespace kcenon::common;
using namespace kcenon::common::interfaces;

// =============================================================================
// HTTP Client Tests
// =============================================================================

/**
 * @brief Mock HTTP client for testing
 */
class MockHttpClient : public IHttpClient {
public:
    MockHttpClient() = default;

    void set_mock_response(http_response response) {
        mock_response_ = std::move(response);
        should_succeed_ = true;
    }

    void set_should_fail(bool fail, const std::string& error_msg = "Mock error") {
        should_succeed_ = !fail;
        error_message_ = error_msg;
    }

    Result<http_response> send(const http_request& request) override {
        last_request_ = request;
        send_count_++;

        if (!should_succeed_) {
            return make_error<http_response>(
                error_codes::NETWORK_ERROR,
                error_message_,
                "MockHttpClient");
        }

        return ok(mock_response_);
    }

    [[nodiscard]] bool is_available() const override {
        return available_;
    }

    [[nodiscard]] std::string get_implementation_name() const override {
        return "MockHttpClient";
    }

    void set_available(bool available) {
        available_ = available;
    }

    [[nodiscard]] const http_request& get_last_request() const {
        return last_request_;
    }

    [[nodiscard]] int get_send_count() const {
        return send_count_;
    }

private:
    http_response mock_response_;
    http_request last_request_;
    bool should_succeed_ = true;
    bool available_ = true;
    std::string error_message_;
    int send_count_ = 0;
};

class HttpClientInterfaceTest : public ::testing::Test {
protected:
    void SetUp() override {
        client_ = std::make_shared<MockHttpClient>();
    }

    std::shared_ptr<MockHttpClient> client_;
};

TEST_F(HttpClientInterfaceTest, HttpRequestConstruction) {
    // Default constructor
    http_request req1;
    EXPECT_TRUE(req1.url.empty());
    EXPECT_EQ(req1.method, "GET");
    EXPECT_TRUE(req1.headers.empty());
    EXPECT_TRUE(req1.body.empty());
    EXPECT_EQ(req1.timeout.count(), 30000);
    EXPECT_TRUE(req1.follow_redirects);
    EXPECT_EQ(req1.max_redirects, 5);

    // URL constructor
    http_request req2{"https://api.example.com"};
    EXPECT_EQ(req2.url, "https://api.example.com");
    EXPECT_EQ(req2.method, "GET");

    // URL + method constructor
    http_request req3{"https://api.example.com", "POST"};
    EXPECT_EQ(req3.method, "POST");

    // Full constructor
    http_headers headers{{"Content-Type", "application/json"}};
    std::vector<uint8_t> body{'t', 'e', 's', 't'};
    http_request req4{"https://api.example.com", "POST", headers, body};
    EXPECT_EQ(req4.headers.at("Content-Type"), "application/json");
    EXPECT_EQ(req4.body.size(), 4u);
}

TEST_F(HttpClientInterfaceTest, HttpRequestChaining) {
    http_request req;
    req.url = "https://api.example.com";
    req.method = "POST";
    req.set_content_type("application/json")
       .set_authorization("Bearer token123")
       .set_body("{\"key\":\"value\"}");

    EXPECT_EQ(req.headers.at("Content-Type"), "application/json");
    EXPECT_EQ(req.headers.at("Authorization"), "Bearer token123");
    EXPECT_EQ(req.body.size(), 15u);
}

TEST_F(HttpClientInterfaceTest, HttpResponseStatusChecks) {
    http_response resp;

    // Success status
    resp.status_code = 200;
    EXPECT_TRUE(resp.is_success());
    EXPECT_FALSE(resp.is_client_error());
    EXPECT_FALSE(resp.is_server_error());

    resp.status_code = 201;
    EXPECT_TRUE(resp.is_success());

    resp.status_code = 299;
    EXPECT_TRUE(resp.is_success());

    // Client error status
    resp.status_code = 400;
    EXPECT_FALSE(resp.is_success());
    EXPECT_TRUE(resp.is_client_error());
    EXPECT_FALSE(resp.is_server_error());

    resp.status_code = 404;
    EXPECT_TRUE(resp.is_client_error());

    // Server error status
    resp.status_code = 500;
    EXPECT_FALSE(resp.is_success());
    EXPECT_FALSE(resp.is_client_error());
    EXPECT_TRUE(resp.is_server_error());

    resp.status_code = 503;
    EXPECT_TRUE(resp.is_server_error());
}

TEST_F(HttpClientInterfaceTest, HttpResponseBodyAsString) {
    http_response resp;
    resp.body = {'H', 'e', 'l', 'l', 'o'};

    EXPECT_EQ(resp.body_as_string(), "Hello");
}

TEST_F(HttpClientInterfaceTest, HttpResponseGetHeader) {
    http_response resp;
    resp.headers["Content-Type"] = "application/json";
    resp.headers["X-Custom-Header"] = "custom-value";

    auto ct = resp.get_header("Content-Type");
    ASSERT_TRUE(ct.has_value());
    EXPECT_EQ(ct.value(), "application/json");

    auto custom = resp.get_header("X-Custom-Header");
    ASSERT_TRUE(custom.has_value());
    EXPECT_EQ(custom.value(), "custom-value");

    auto missing = resp.get_header("X-Missing");
    EXPECT_FALSE(missing.has_value());
}

TEST_F(HttpClientInterfaceTest, MockClientSendSuccess) {
    http_response mock_resp;
    mock_resp.status_code = 200;
    mock_resp.body = {'O', 'K'};
    client_->set_mock_response(mock_resp);

    http_request req{"https://api.example.com", "GET"};
    auto result = client_->send(req);

    ASSERT_TRUE(result.is_ok());
    EXPECT_EQ(result.value().status_code, 200);
    EXPECT_EQ(result.value().body_as_string(), "OK");
    EXPECT_EQ(client_->get_send_count(), 1);
}

TEST_F(HttpClientInterfaceTest, MockClientSendFailure) {
    client_->set_should_fail(true, "Connection timeout");

    http_request req{"https://api.example.com"};
    auto result = client_->send(req);

    ASSERT_TRUE(result.is_err());
    EXPECT_EQ(result.error().code, error_codes::NETWORK_ERROR);
    EXPECT_NE(result.error().message.find("Connection timeout"), std::string::npos);
}

TEST_F(HttpClientInterfaceTest, MockClientAvailability) {
    EXPECT_TRUE(client_->is_available());

    client_->set_available(false);
    EXPECT_FALSE(client_->is_available());
}

TEST_F(HttpClientInterfaceTest, MockClientImplementationName) {
    EXPECT_EQ(client_->get_implementation_name(), "MockHttpClient");
}

TEST_F(HttpClientInterfaceTest, NullHttpClient) {
    null_http_client null_client;

    http_request req{"https://api.example.com"};
    auto result = null_client.send(req);

    ASSERT_TRUE(result.is_err());
    EXPECT_EQ(result.error().code, error_codes::NOT_INITIALIZED);
    EXPECT_FALSE(null_client.is_available());
    EXPECT_EQ(null_client.get_implementation_name(), "null_http_client");
}

// =============================================================================
// UDP Client Tests
// =============================================================================

/**
 * @brief Mock UDP client for testing
 */
class MockUdpClient : public IUdpClient {
public:
    // Bring base class string convenience methods into scope
    using IUdpClient::send;
    using IUdpClient::send_to;

    MockUdpClient() = default;

    VoidResult connect(const std::string& host, uint16_t port) override {
        if (!should_succeed_) {
            return VoidResult(error_info{
                error_codes::NETWORK_ERROR,
                error_message_,
                "MockUdpClient"});
        }

        connected_endpoint_ = udp_endpoint{host, port};
        connected_ = true;
        return ok();
    }

    VoidResult send(std::span<const uint8_t> data) override {
        if (!connected_) {
            return VoidResult(error_info{
                error_codes::INVALID_ARGUMENT,
                "Not connected",
                "MockUdpClient"});
        }

        if (!should_succeed_) {
            stats_.send_failures++;
            return VoidResult(error_info{
                error_codes::NETWORK_ERROR,
                error_message_,
                "MockUdpClient"});
        }

        last_sent_data_.assign(data.begin(), data.end());
        stats_.packets_sent++;
        stats_.bytes_sent += data.size();
        stats_.last_send_time = std::chrono::steady_clock::now();
        return ok();
    }

    VoidResult send_to(std::span<const uint8_t> data,
                       const udp_endpoint& endpoint) override {
        if (!should_succeed_) {
            stats_.send_failures++;
            return VoidResult(error_info{
                error_codes::NETWORK_ERROR,
                error_message_,
                "MockUdpClient"});
        }

        last_sent_data_.assign(data.begin(), data.end());
        last_send_to_endpoint_ = endpoint;
        stats_.packets_sent++;
        stats_.bytes_sent += data.size();
        stats_.last_send_time = std::chrono::steady_clock::now();
        return ok();
    }

    [[nodiscard]] bool is_connected() const override {
        return connected_;
    }

    [[nodiscard]] std::optional<udp_endpoint> get_remote_endpoint() const override {
        if (connected_) {
            return connected_endpoint_;
        }
        return std::nullopt;
    }

    void disconnect() override {
        connected_ = false;
        connected_endpoint_ = udp_endpoint{};
    }

    [[nodiscard]] udp_statistics get_statistics() const override {
        return stats_;
    }

    void reset_statistics() override {
        stats_.reset();
    }

    [[nodiscard]] std::string get_implementation_name() const override {
        return "MockUdpClient";
    }

    // Test helpers
    void set_should_fail(bool fail, const std::string& error_msg = "Mock error") {
        should_succeed_ = !fail;
        error_message_ = error_msg;
    }

    [[nodiscard]] const std::vector<uint8_t>& get_last_sent_data() const {
        return last_sent_data_;
    }

    [[nodiscard]] const udp_endpoint& get_last_send_to_endpoint() const {
        return last_send_to_endpoint_;
    }

private:
    bool connected_ = false;
    bool should_succeed_ = true;
    std::string error_message_;
    udp_endpoint connected_endpoint_;
    udp_endpoint last_send_to_endpoint_;
    std::vector<uint8_t> last_sent_data_;
    udp_statistics stats_;
};

class UdpClientInterfaceTest : public ::testing::Test {
protected:
    void SetUp() override {
        client_ = std::make_shared<MockUdpClient>();
    }

    std::shared_ptr<MockUdpClient> client_;
};

TEST_F(UdpClientInterfaceTest, UdpEndpointConstruction) {
    // Default constructor
    udp_endpoint ep1;
    EXPECT_TRUE(ep1.host.empty());
    EXPECT_EQ(ep1.port, 0);

    // Full constructor
    udp_endpoint ep2{"localhost", 8125};
    EXPECT_EQ(ep2.host, "localhost");
    EXPECT_EQ(ep2.port, 8125);

    // to_string
    EXPECT_EQ(ep2.to_string(), "localhost:8125");

    // Equality operators
    udp_endpoint ep3{"localhost", 8125};
    udp_endpoint ep4{"localhost", 9999};
    EXPECT_EQ(ep2, ep3);
    EXPECT_NE(ep2, ep4);
}

TEST_F(UdpClientInterfaceTest, UdpStatisticsReset) {
    udp_statistics stats;
    stats.packets_sent = 100;
    stats.bytes_sent = 5000;
    stats.send_failures = 5;
    stats.last_send_time = std::chrono::steady_clock::now();

    stats.reset();

    EXPECT_EQ(stats.packets_sent, 0u);
    EXPECT_EQ(stats.bytes_sent, 0u);
    EXPECT_EQ(stats.send_failures, 0u);
    EXPECT_FALSE(stats.last_send_time.has_value());
}

TEST_F(UdpClientInterfaceTest, ConnectAndSend) {
    auto result = client_->connect("localhost", 8125);
    ASSERT_TRUE(result.is_ok());
    EXPECT_TRUE(client_->is_connected());

    auto endpoint = client_->get_remote_endpoint();
    ASSERT_TRUE(endpoint.has_value());
    EXPECT_EQ(endpoint->host, "localhost");
    EXPECT_EQ(endpoint->port, 8125);

    std::vector<uint8_t> data{'t', 'e', 's', 't'};
    result = client_->send(std::span<const uint8_t>(data));
    ASSERT_TRUE(result.is_ok());

    EXPECT_EQ(client_->get_last_sent_data(), data);

    auto stats = client_->get_statistics();
    EXPECT_EQ(stats.packets_sent, 1u);
    EXPECT_EQ(stats.bytes_sent, 4u);
}

TEST_F(UdpClientInterfaceTest, SendWithoutConnect) {
    EXPECT_FALSE(client_->is_connected());

    std::vector<uint8_t> data{'t', 'e', 's', 't'};
    auto result = client_->send(std::span<const uint8_t>(data));

    ASSERT_TRUE(result.is_err());
    EXPECT_EQ(result.error().code, error_codes::INVALID_ARGUMENT);
}

TEST_F(UdpClientInterfaceTest, SendTo) {
    udp_endpoint target{"metrics.example.com", 8125};
    std::vector<uint8_t> data{'m', 'e', 't', 'r', 'i', 'c'};

    auto result = client_->send_to(std::span<const uint8_t>(data), target);
    ASSERT_TRUE(result.is_ok());

    EXPECT_EQ(client_->get_last_send_to_endpoint(), target);
    EXPECT_EQ(client_->get_last_sent_data(), data);
}

TEST_F(UdpClientInterfaceTest, StringSendConvenience) {
    client_->connect("localhost", 8125);

    std::string metric = "app.requests:1|c";
    auto result = client_->send(metric);
    ASSERT_TRUE(result.is_ok());

    auto& sent = client_->get_last_sent_data();
    std::string sent_str(sent.begin(), sent.end());
    EXPECT_EQ(sent_str, metric);
}

TEST_F(UdpClientInterfaceTest, StringSendToConvenience) {
    udp_endpoint target{"localhost", 8125};
    std::string metric = "app.requests:1|c";

    auto result = client_->send_to(metric, target);
    ASSERT_TRUE(result.is_ok());

    auto& sent = client_->get_last_sent_data();
    std::string sent_str(sent.begin(), sent.end());
    EXPECT_EQ(sent_str, metric);
}

TEST_F(UdpClientInterfaceTest, Disconnect) {
    client_->connect("localhost", 8125);
    EXPECT_TRUE(client_->is_connected());

    client_->disconnect();
    EXPECT_FALSE(client_->is_connected());
    EXPECT_FALSE(client_->get_remote_endpoint().has_value());
}

TEST_F(UdpClientInterfaceTest, ConnectFailure) {
    client_->set_should_fail(true, "DNS resolution failed");

    auto result = client_->connect("invalid.host", 8125);
    ASSERT_TRUE(result.is_err());
    EXPECT_EQ(result.error().code, error_codes::NETWORK_ERROR);
}

TEST_F(UdpClientInterfaceTest, SendFailure) {
    client_->connect("localhost", 8125);
    client_->set_should_fail(true, "Network unreachable");

    std::vector<uint8_t> data{'t', 'e', 's', 't'};
    auto result = client_->send(std::span<const uint8_t>(data));

    ASSERT_TRUE(result.is_err());
    auto stats = client_->get_statistics();
    EXPECT_EQ(stats.send_failures, 1u);
}

TEST_F(UdpClientInterfaceTest, StatisticsTracking) {
    client_->connect("localhost", 8125);

    for (int i = 0; i < 5; i++) {
        std::string metric = "metric" + std::to_string(i);
        client_->send(metric);
    }

    auto stats = client_->get_statistics();
    EXPECT_EQ(stats.packets_sent, 5u);
    EXPECT_GT(stats.bytes_sent, 0u);
    EXPECT_TRUE(stats.last_send_time.has_value());
}

TEST_F(UdpClientInterfaceTest, ResetStatistics) {
    client_->connect("localhost", 8125);
    client_->send("test");

    auto stats = client_->get_statistics();
    EXPECT_GT(stats.packets_sent, 0u);

    client_->reset_statistics();
    stats = client_->get_statistics();
    EXPECT_EQ(stats.packets_sent, 0u);
}

TEST_F(UdpClientInterfaceTest, NullUdpClient) {
    null_udp_client null_client;

    auto connect_result = null_client.connect("localhost", 8125);
    ASSERT_TRUE(connect_result.is_err());
    EXPECT_EQ(connect_result.error().code, error_codes::NOT_INITIALIZED);

    std::vector<uint8_t> data{'t', 'e', 's', 't'};
    auto send_result = null_client.send(std::span<const uint8_t>(data));
    ASSERT_TRUE(send_result.is_err());

    udp_endpoint target{"localhost", 8125};
    auto send_to_result = null_client.send_to(std::span<const uint8_t>(data), target);
    ASSERT_TRUE(send_to_result.is_err());

    EXPECT_FALSE(null_client.is_connected());
    null_client.disconnect();  // Should not throw

    EXPECT_EQ(null_client.get_implementation_name(), "null_udp_client");
}

// =============================================================================
// Interface Polymorphism Tests
// =============================================================================

TEST(TransportInterfacePolymorphismTest, HttpClientPolymorphism) {
    std::shared_ptr<IHttpClient> client = std::make_shared<MockHttpClient>();

    EXPECT_TRUE(client->is_available());

    http_request req{"https://api.example.com"};
    auto result = client->send(req);
    EXPECT_TRUE(result.is_ok());
}

TEST(TransportInterfacePolymorphismTest, UdpClientPolymorphism) {
    std::shared_ptr<IUdpClient> client = std::make_shared<MockUdpClient>();

    auto result = client->connect("localhost", 8125);
    EXPECT_TRUE(result.is_ok());
    EXPECT_TRUE(client->is_connected());
}

TEST(TransportInterfacePolymorphismTest, NullImplementationsAsBase) {
    std::shared_ptr<IHttpClient> http_client = std::make_shared<null_http_client>();
    std::shared_ptr<IUdpClient> udp_client = std::make_shared<null_udp_client>();

    EXPECT_FALSE(http_client->is_available());
    EXPECT_FALSE(udp_client->is_connected());
}
