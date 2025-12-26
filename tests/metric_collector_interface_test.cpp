// BSD 3-Clause License
// Copyright (c) 2025, kcenon
// See the LICENSE file in the project root for full license information.

/**
 * @file metric_collector_interface_test.cpp
 * @brief Unit tests for metric collector interfaces (IMetricCollector)
 */

#include <gtest/gtest.h>
#include <kcenon/common/interfaces/monitoring.h>

#include <chrono>
#include <memory>
#include <string>
#include <thread>
#include <vector>

using namespace kcenon::common::interfaces;

// =============================================================================
// Mock Metric Collector for Testing
// =============================================================================

/**
 * @brief Mock metric collector that records all metric emissions
 */
class MockMetricCollector : public IMetricCollector {
public:
    struct metric_record {
        std::string type;  // "counter", "gauge", "histogram", "timing"
        std::string name;
        double value;
        metric_labels labels;
        std::chrono::nanoseconds duration{0};  // Only for timing
    };

    void increment(std::string_view name,
                  double value = 1.0,
                  const metric_labels& labels = {}) override {
        records_.push_back({
            "counter",
            std::string(name),
            value,
            labels,
            std::chrono::nanoseconds{0}
        });
    }

    void gauge(std::string_view name,
              double value,
              const metric_labels& labels = {}) override {
        records_.push_back({
            "gauge",
            std::string(name),
            value,
            labels,
            std::chrono::nanoseconds{0}
        });
    }

    void histogram(std::string_view name,
                  double value,
                  const metric_labels& labels = {}) override {
        records_.push_back({
            "histogram",
            std::string(name),
            value,
            labels,
            std::chrono::nanoseconds{0}
        });
    }

    void timing(std::string_view name,
               std::chrono::nanoseconds duration,
               const metric_labels& labels = {}) override {
        records_.push_back({
            "timing",
            std::string(name),
            static_cast<double>(duration.count()),
            labels,
            duration
        });
    }

    [[nodiscard]] std::string get_implementation_name() const override {
        return "MockMetricCollector";
    }

    [[nodiscard]] const std::vector<metric_record>& get_records() const {
        return records_;
    }

    void clear_records() {
        records_.clear();
    }

    [[nodiscard]] size_t get_record_count() const {
        return records_.size();
    }

    [[nodiscard]] const metric_record& get_last_record() const {
        return records_.back();
    }

private:
    std::vector<metric_record> records_;
};

// =============================================================================
// IMetricCollector Interface Tests
// =============================================================================

class MetricCollectorInterfaceTest : public ::testing::Test {
protected:
    void SetUp() override {
        collector_ = std::make_shared<MockMetricCollector>();
    }

    std::shared_ptr<MockMetricCollector> collector_;
};

TEST_F(MetricCollectorInterfaceTest, IncrementCounter) {
    collector_->increment("http_requests_total", 1.0);

    ASSERT_EQ(collector_->get_record_count(), 1u);
    const auto& record = collector_->get_last_record();
    EXPECT_EQ(record.type, "counter");
    EXPECT_EQ(record.name, "http_requests_total");
    EXPECT_DOUBLE_EQ(record.value, 1.0);
    EXPECT_TRUE(record.labels.empty());
}

TEST_F(MetricCollectorInterfaceTest, IncrementCounterWithLabels) {
    metric_labels labels{{"method", "GET"}, {"endpoint", "/api/users"}};
    collector_->increment("http_requests_total", 1.0, labels);

    ASSERT_EQ(collector_->get_record_count(), 1u);
    const auto& record = collector_->get_last_record();
    EXPECT_EQ(record.labels.size(), 2u);
    EXPECT_EQ(record.labels.at("method"), "GET");
    EXPECT_EQ(record.labels.at("endpoint"), "/api/users");
}

TEST_F(MetricCollectorInterfaceTest, IncrementCounterDefaultValue) {
    collector_->increment("events_total");

    ASSERT_EQ(collector_->get_record_count(), 1u);
    EXPECT_DOUBLE_EQ(collector_->get_last_record().value, 1.0);
}

TEST_F(MetricCollectorInterfaceTest, IncrementCounterCustomValue) {
    collector_->increment("bytes_processed", 1024.0);

    ASSERT_EQ(collector_->get_record_count(), 1u);
    EXPECT_DOUBLE_EQ(collector_->get_last_record().value, 1024.0);
}

TEST_F(MetricCollectorInterfaceTest, GaugeMetric) {
    collector_->gauge("active_connections", 42.0);

    ASSERT_EQ(collector_->get_record_count(), 1u);
    const auto& record = collector_->get_last_record();
    EXPECT_EQ(record.type, "gauge");
    EXPECT_EQ(record.name, "active_connections");
    EXPECT_DOUBLE_EQ(record.value, 42.0);
}

TEST_F(MetricCollectorInterfaceTest, GaugeMetricWithLabels) {
    metric_labels labels{{"pool", "worker_pool_1"}};
    collector_->gauge("thread_count", 8.0, labels);

    ASSERT_EQ(collector_->get_record_count(), 1u);
    const auto& record = collector_->get_last_record();
    EXPECT_EQ(record.labels.at("pool"), "worker_pool_1");
}

TEST_F(MetricCollectorInterfaceTest, GaugeNegativeValue) {
    collector_->gauge("temperature_celsius", -15.5);

    ASSERT_EQ(collector_->get_record_count(), 1u);
    EXPECT_DOUBLE_EQ(collector_->get_last_record().value, -15.5);
}

TEST_F(MetricCollectorInterfaceTest, HistogramMetric) {
    collector_->histogram("request_size_bytes", 2048.0);

    ASSERT_EQ(collector_->get_record_count(), 1u);
    const auto& record = collector_->get_last_record();
    EXPECT_EQ(record.type, "histogram");
    EXPECT_EQ(record.name, "request_size_bytes");
    EXPECT_DOUBLE_EQ(record.value, 2048.0);
}

TEST_F(MetricCollectorInterfaceTest, HistogramMetricWithLabels) {
    metric_labels labels{{"service", "api"}, {"version", "v2"}};
    collector_->histogram("response_size_bytes", 512.0, labels);

    ASSERT_EQ(collector_->get_record_count(), 1u);
    const auto& record = collector_->get_last_record();
    EXPECT_EQ(record.labels.size(), 2u);
}

TEST_F(MetricCollectorInterfaceTest, TimingMetric) {
    auto duration = std::chrono::milliseconds{150};
    collector_->timing("request_duration",
                      std::chrono::duration_cast<std::chrono::nanoseconds>(duration));

    ASSERT_EQ(collector_->get_record_count(), 1u);
    const auto& record = collector_->get_last_record();
    EXPECT_EQ(record.type, "timing");
    EXPECT_EQ(record.name, "request_duration");
    EXPECT_EQ(record.duration, std::chrono::nanoseconds{150000000});
}

TEST_F(MetricCollectorInterfaceTest, TimingMetricWithLabels) {
    metric_labels labels{{"handler", "user_api"}};
    collector_->timing("handler_duration",
                      std::chrono::microseconds{500},
                      labels);

    ASSERT_EQ(collector_->get_record_count(), 1u);
    const auto& record = collector_->get_last_record();
    EXPECT_EQ(record.labels.at("handler"), "user_api");
}

TEST_F(MetricCollectorInterfaceTest, MultipleMetrics) {
    collector_->increment("counter1");
    collector_->gauge("gauge1", 10.0);
    collector_->histogram("hist1", 100.0);
    collector_->timing("time1", std::chrono::milliseconds{50});

    EXPECT_EQ(collector_->get_record_count(), 4u);

    const auto& records = collector_->get_records();
    EXPECT_EQ(records[0].type, "counter");
    EXPECT_EQ(records[1].type, "gauge");
    EXPECT_EQ(records[2].type, "histogram");
    EXPECT_EQ(records[3].type, "timing");
}

TEST_F(MetricCollectorInterfaceTest, ImplementationName) {
    EXPECT_EQ(collector_->get_implementation_name(), "MockMetricCollector");
}

// =============================================================================
// scoped_timer Tests
// =============================================================================

class ScopedTimerTest : public ::testing::Test {
protected:
    void SetUp() override {
        collector_ = std::make_shared<MockMetricCollector>();
    }

    std::shared_ptr<MockMetricCollector> collector_;
};

TEST_F(ScopedTimerTest, BasicTiming) {
    {
        scoped_timer timer(*collector_, "test_operation");
        // Simulate some work
        std::this_thread::sleep_for(std::chrono::milliseconds{10});
    }  // Timer reports on destruction

    ASSERT_EQ(collector_->get_record_count(), 1u);
    const auto& record = collector_->get_last_record();
    EXPECT_EQ(record.type, "timing");
    EXPECT_EQ(record.name, "test_operation");
    // Duration should be at least 10ms
    EXPECT_GE(record.duration, std::chrono::milliseconds{10});
}

TEST_F(ScopedTimerTest, TimingWithLabels) {
    metric_labels labels{{"function", "process_data"}};
    {
        scoped_timer timer(*collector_, "function_duration", labels);
    }

    ASSERT_EQ(collector_->get_record_count(), 1u);
    const auto& record = collector_->get_last_record();
    EXPECT_EQ(record.labels.at("function"), "process_data");
}

TEST_F(ScopedTimerTest, ElapsedTime) {
    scoped_timer timer(*collector_, "test_elapsed");

    std::this_thread::sleep_for(std::chrono::milliseconds{5});
    auto elapsed = timer.elapsed();

    EXPECT_GE(elapsed, std::chrono::milliseconds{5});
    // Timer should not have reported yet
    EXPECT_EQ(collector_->get_record_count(), 0u);
}

TEST_F(ScopedTimerTest, MultipleTimers) {
    {
        scoped_timer timer1(*collector_, "operation1");
        {
            scoped_timer timer2(*collector_, "operation2");
        }  // timer2 reports here
    }  // timer1 reports here

    ASSERT_EQ(collector_->get_record_count(), 2u);
    const auto& records = collector_->get_records();
    // timer2 reports first (inner scope exits first)
    EXPECT_EQ(records[0].name, "operation2");
    EXPECT_EQ(records[1].name, "operation1");
}

TEST_F(ScopedTimerTest, ZeroDuration) {
    {
        scoped_timer timer(*collector_, "instant_operation");
        // No delay
    }

    ASSERT_EQ(collector_->get_record_count(), 1u);
    // Duration should be very small but non-negative
    EXPECT_GE(collector_->get_last_record().duration, std::chrono::nanoseconds{0});
}

// =============================================================================
// null_metric_collector Tests
// =============================================================================

class NullMetricCollectorTest : public ::testing::Test {
protected:
    null_metric_collector collector_;
};

TEST_F(NullMetricCollectorTest, IncrementDoesNothing) {
    // Should not throw or crash
    collector_.increment("test_counter");
    collector_.increment("test_counter", 100.0);
    collector_.increment("test_counter", 1.0, {{"key", "value"}});
}

TEST_F(NullMetricCollectorTest, GaugeDoesNothing) {
    collector_.gauge("test_gauge", 42.0);
    collector_.gauge("test_gauge", -10.0, {{"key", "value"}});
}

TEST_F(NullMetricCollectorTest, HistogramDoesNothing) {
    collector_.histogram("test_histogram", 100.0);
    collector_.histogram("test_histogram", 0.0, {{"key", "value"}});
}

TEST_F(NullMetricCollectorTest, TimingDoesNothing) {
    collector_.timing("test_timing", std::chrono::milliseconds{100});
    collector_.timing("test_timing", std::chrono::nanoseconds{0}, {{"key", "value"}});
}

TEST_F(NullMetricCollectorTest, ImplementationName) {
    EXPECT_EQ(collector_.get_implementation_name(), "null_metric_collector");
}

TEST_F(NullMetricCollectorTest, ScopedTimerWithNullCollector) {
    // scoped_timer should work correctly with null_metric_collector
    {
        scoped_timer timer(collector_, "null_operation", {{"test", "value"}});
        std::this_thread::sleep_for(std::chrono::milliseconds{1});
    }
    // No crash, no side effects
}

// =============================================================================
// Interface Polymorphism Tests
// =============================================================================

TEST(MetricCollectorPolymorphismTest, PolymorphicUsage) {
    std::shared_ptr<IMetricCollector> collector =
        std::make_shared<MockMetricCollector>();

    collector->increment("poly_counter");
    collector->gauge("poly_gauge", 10.0);
    collector->histogram("poly_histogram", 100.0);
    collector->timing("poly_timing", std::chrono::milliseconds{50});

    auto* mock = dynamic_cast<MockMetricCollector*>(collector.get());
    ASSERT_NE(mock, nullptr);
    EXPECT_EQ(mock->get_record_count(), 4u);
}

TEST(MetricCollectorPolymorphismTest, NullCollectorAsBase) {
    std::shared_ptr<IMetricCollector> collector =
        std::make_shared<null_metric_collector>();

    // Should work without issues
    collector->increment("test");
    collector->gauge("test", 1.0);
    collector->histogram("test", 1.0);
    collector->timing("test", std::chrono::milliseconds{1});

    EXPECT_EQ(collector->get_implementation_name(), "null_metric_collector");
}

TEST(MetricCollectorPolymorphismTest, ScopedTimerWithPolymorphicCollector) {
    std::shared_ptr<IMetricCollector> collector =
        std::make_shared<MockMetricCollector>();

    {
        scoped_timer timer(*collector, "polymorphic_timing");
    }

    auto* mock = dynamic_cast<MockMetricCollector*>(collector.get());
    ASSERT_NE(mock, nullptr);
    EXPECT_EQ(mock->get_record_count(), 1u);
}

// =============================================================================
// metric_labels Tests
// =============================================================================

TEST(MetricLabelsTest, EmptyLabels) {
    metric_labels labels;
    EXPECT_TRUE(labels.empty());
}

TEST(MetricLabelsTest, InitializerList) {
    metric_labels labels{{"key1", "value1"}, {"key2", "value2"}};
    EXPECT_EQ(labels.size(), 2u);
    EXPECT_EQ(labels.at("key1"), "value1");
    EXPECT_EQ(labels.at("key2"), "value2");
}

TEST(MetricLabelsTest, InsertAndAccess) {
    metric_labels labels;
    labels["service"] = "api";
    labels["version"] = "1.0";

    EXPECT_EQ(labels.size(), 2u);
    EXPECT_EQ(labels["service"], "api");
}

TEST(MetricLabelsTest, OverwriteValue) {
    metric_labels labels{{"key", "old_value"}};
    labels["key"] = "new_value";

    EXPECT_EQ(labels.at("key"), "new_value");
}
