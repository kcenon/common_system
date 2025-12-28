// BSD 3-Clause License
// Copyright (c) 2025, kcenon
// See the LICENSE file in the project root for full license information.

/**
 * @file concepts_test.cpp
 * @brief Unit tests for C++20 concepts definitions
 *
 * This file tests that concepts correctly validate interface implementations
 * and reject non-conforming types at compile-time.
 */

#include <gtest/gtest.h>

// Include interfaces first to ensure proper type definitions
// before concepts that may use forward declarations
#include <kcenon/common/interfaces/logger_interface.h>
#include <kcenon/common/interfaces/monitoring/metric_collector_interface.h>
#include <kcenon/common/interfaces/transport/http_client_interface.h>
#include <kcenon/common/interfaces/transport/udp_client_interface.h>

#include <kcenon/common/concepts/concepts.h>

#include <memory>
#include <string>

using namespace kcenon::common;
using namespace kcenon::common::concepts;
using namespace kcenon::common::interfaces;

// =============================================================================
// Logger Concepts Tests
// =============================================================================

/**
 * @brief Mock logger implementation for testing concepts
 */
class MockLogger : public ILogger {
public:
    VoidResult log(log_level level, const std::string& message) override {
        (void)level;
        (void)message;
        return ok();
    }

    VoidResult log(const log_entry& entry) override {
        (void)entry;
        return ok();
    }

    bool is_enabled(log_level level) const override {
        return level >= min_level_;
    }

    VoidResult set_level(log_level level) override {
        min_level_ = level;
        return ok();
    }

    log_level get_level() const override {
        return min_level_;
    }

    VoidResult flush() override {
        return ok();
    }

private:
    log_level min_level_ = log_level::info;
};

// Compile-time concept verification
static_assert(BasicLogger<MockLogger>,
              "MockLogger should satisfy BasicLogger concept");
static_assert(LevelAwareLogger<MockLogger>,
              "MockLogger should satisfy LevelAwareLogger concept");
static_assert(FlushableLogger<MockLogger>,
              "MockLogger should satisfy FlushableLogger concept");
static_assert(StructuredLogger<MockLogger>,
              "MockLogger should satisfy StructuredLogger concept");
static_assert(LoggerLike<MockLogger>,
              "MockLogger should satisfy LoggerLike concept");

class LoggerConceptsTest : public ::testing::Test {
protected:
    void SetUp() override {
        logger_ = std::make_shared<MockLogger>();
    }

    std::shared_ptr<MockLogger> logger_;
};

TEST_F(LoggerConceptsTest, MockLoggerSatisfiesBasicLogger) {
    // Runtime test to ensure concept works with actual usage
    EXPECT_TRUE((BasicLogger<MockLogger>));
}

TEST_F(LoggerConceptsTest, MockLoggerSatisfiesLoggerLike) {
    EXPECT_TRUE((LoggerLike<MockLogger>));
}

TEST_F(LoggerConceptsTest, LoggerCanBeUsedWithConcept) {
    // Test that we can use the concept as a template constraint
    auto test_logging = []<LoggerLike L>(L& logger) {
        logger.log(log_level::info, "Test message");
        return logger.is_enabled(log_level::debug);
    };

    bool debug_enabled = test_logging(*logger_);
    EXPECT_FALSE(debug_enabled);  // Default level is info

    logger_->set_level(log_level::debug);
    debug_enabled = test_logging(*logger_);
    EXPECT_TRUE(debug_enabled);
}

// =============================================================================
// Metric Collector Concepts Tests
// =============================================================================

// Compile-time concept verification for null_metric_collector
static_assert(CounterMetric<null_metric_collector>,
              "null_metric_collector should satisfy CounterMetric concept");
static_assert(GaugeMetric<null_metric_collector>,
              "null_metric_collector should satisfy GaugeMetric concept");
static_assert(HistogramMetric<null_metric_collector>,
              "null_metric_collector should satisfy HistogramMetric concept");
static_assert(TimingMetric<null_metric_collector>,
              "null_metric_collector should satisfy TimingMetric concept");
static_assert(MetricCollectorLike<null_metric_collector>,
              "null_metric_collector should satisfy MetricCollectorLike concept");

class MetricCollectorConceptsTest : public ::testing::Test {
protected:
    void SetUp() override {
        collector_ = std::make_shared<null_metric_collector>();
    }

    std::shared_ptr<null_metric_collector> collector_;
};

TEST_F(MetricCollectorConceptsTest, NullCollectorSatisfiesMetricCollectorLike) {
    EXPECT_TRUE((MetricCollectorLike<null_metric_collector>));
}

TEST_F(MetricCollectorConceptsTest, CollectorCanBeUsedWithConcept) {
    // Test that we can use the concept as a template constraint
    auto emit_metrics = []<MetricCollectorLike M>(M& collector) {
        collector.increment("requests", 1.0, {});
        collector.gauge("connections", 42.0, {});
        collector.histogram("size", 1024.0, {});
        collector.timing("duration", std::chrono::milliseconds{100}, {});
    };

    // Should compile and run without issues
    EXPECT_NO_THROW(emit_metrics(*collector_));
}

// =============================================================================
// HTTP Client Concepts Tests
// =============================================================================

// Compile-time concept verification for null_http_client
static_assert(HttpSender<null_http_client>,
              "null_http_client should satisfy HttpSender concept");
static_assert(HttpAvailabilityChecker<null_http_client>,
              "null_http_client should satisfy HttpAvailabilityChecker concept");
static_assert(HttpClientLike<null_http_client>,
              "null_http_client should satisfy HttpClientLike concept");

class HttpClientConceptsTest : public ::testing::Test {
protected:
    void SetUp() override {
        client_ = std::make_shared<null_http_client>();
    }

    std::shared_ptr<null_http_client> client_;
};

TEST_F(HttpClientConceptsTest, NullClientSatisfiesHttpClientLike) {
    EXPECT_TRUE((HttpClientLike<null_http_client>));
}

TEST_F(HttpClientConceptsTest, HttpClientCanBeUsedWithConcept) {
    // Test that we can use the concept as a template constraint
    auto check_availability = []<HttpClientLike H>(const H& client) {
        return client.is_available();
    };

    EXPECT_FALSE(check_availability(*client_));
}

// =============================================================================
// UDP Client Concepts Tests
// =============================================================================

// Compile-time concept verification for null_udp_client
static_assert(UdpConnectable<null_udp_client>,
              "null_udp_client should satisfy UdpConnectable concept");
static_assert(UdpSender<null_udp_client>,
              "null_udp_client should satisfy UdpSender concept");
static_assert(UdpConnectionStatus<null_udp_client>,
              "null_udp_client should satisfy UdpConnectionStatus concept");
static_assert(UdpClientLike<null_udp_client>,
              "null_udp_client should satisfy UdpClientLike concept");

class UdpClientConceptsTest : public ::testing::Test {
protected:
    void SetUp() override {
        client_ = std::make_shared<null_udp_client>();
    }

    std::shared_ptr<null_udp_client> client_;
};

TEST_F(UdpClientConceptsTest, NullClientSatisfiesUdpClientLike) {
    EXPECT_TRUE((UdpClientLike<null_udp_client>));
}

TEST_F(UdpClientConceptsTest, UdpClientCanBeUsedWithConcept) {
    // Test that we can use the concept as a template constraint
    auto check_connection = []<UdpClientLike U>(const U& client) {
        return client.is_connected();
    };

    EXPECT_FALSE(check_connection(*client_));
}

// =============================================================================
// Negative Tests - Types that should NOT satisfy concepts
// =============================================================================

/**
 * @brief A type that does NOT satisfy LoggerLike
 */
struct NotALogger {
    void log(const std::string& msg) {
        (void)msg;
    }
};

static_assert(!BasicLogger<NotALogger>,
              "NotALogger should NOT satisfy BasicLogger concept");
static_assert(!LoggerLike<NotALogger>,
              "NotALogger should NOT satisfy LoggerLike concept");

/**
 * @brief A type that does NOT satisfy MetricCollectorLike
 */
struct NotACollector {
    void record(double value) {
        (void)value;
    }
};

static_assert(!CounterMetric<NotACollector>,
              "NotACollector should NOT satisfy CounterMetric concept");
static_assert(!MetricCollectorLike<NotACollector>,
              "NotACollector should NOT satisfy MetricCollectorLike concept");

/**
 * @brief A type that does NOT satisfy HttpClientLike
 */
struct NotAnHttpClient {
    void get(const std::string& url) {
        (void)url;
    }
};

static_assert(!HttpSender<NotAnHttpClient>,
              "NotAnHttpClient should NOT satisfy HttpSender concept");
static_assert(!HttpClientLike<NotAnHttpClient>,
              "NotAnHttpClient should NOT satisfy HttpClientLike concept");

/**
 * @brief A type that does NOT satisfy UdpClientLike
 */
struct NotAUdpClient {
    void send_message(const std::string& data) {
        (void)data;
    }
};

static_assert(!UdpConnectable<NotAUdpClient>,
              "NotAUdpClient should NOT satisfy UdpConnectable concept");
static_assert(!UdpClientLike<NotAUdpClient>,
              "NotAUdpClient should NOT satisfy UdpClientLike concept");

TEST(NegativeConceptsTest, NonConformingTypesDoNotSatisfyConcepts) {
    // These are compile-time checks, but we add a runtime test for clarity
    EXPECT_FALSE((BasicLogger<NotALogger>));
    EXPECT_FALSE((MetricCollectorLike<NotACollector>));
    EXPECT_FALSE((HttpClientLike<NotAnHttpClient>));
    EXPECT_FALSE((UdpClientLike<NotAUdpClient>));
}

// =============================================================================
// TransportClient Concept Tests
// =============================================================================

static_assert(TransportClient<null_http_client>,
              "null_http_client should satisfy TransportClient concept");
static_assert(TransportClient<null_udp_client>,
              "null_udp_client should satisfy TransportClient concept");
static_assert(!TransportClient<NotAnHttpClient>,
              "NotAnHttpClient should NOT satisfy TransportClient concept");

TEST(TransportClientConceptTest, BothHttpAndUdpSatisfyTransportClient) {
    EXPECT_TRUE((TransportClient<null_http_client>));
    EXPECT_TRUE((TransportClient<null_udp_client>));
}
