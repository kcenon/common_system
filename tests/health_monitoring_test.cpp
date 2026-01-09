// BSD 3-Clause License
// Copyright (c) 2025, kcenon
// See the LICENSE file in the project root for full license information.

/**
 * @file health_monitoring_test.cpp
 * @brief Tests for health monitoring API
 *
 * This file tests the health monitoring functionality including:
 * - health_check base class and types
 * - composite_health_check
 * - health_dependency_graph
 * - health_check_builder
 * - health_monitor
 * - global_health_monitor singleton
 */

#include <gtest/gtest.h>

#include <chrono>
#include <memory>
#include <string>

#include <kcenon/common/interfaces/monitoring.h>

using namespace kcenon::common;
using namespace kcenon::common::interfaces;

// =============================================================================
// health_check_type Tests
// =============================================================================

TEST(HealthCheckTypeTest, ToStringConversion) {
    EXPECT_EQ(to_string(health_check_type::liveness), "LIVENESS");
    EXPECT_EQ(to_string(health_check_type::readiness), "READINESS");
    EXPECT_EQ(to_string(health_check_type::startup), "STARTUP");
    EXPECT_EQ(to_string(health_check_type::dependency), "DEPENDENCY");
    EXPECT_EQ(to_string(health_check_type::custom), "CUSTOM");
}

TEST(HealthCheckTypeTest, FromStringConversion) {
    auto result1 = health_check_type_from_string("LIVENESS");
    ASSERT_TRUE(result1.is_ok());
    EXPECT_EQ(result1.value(), health_check_type::liveness);

    auto result2 = health_check_type_from_string("readiness");
    ASSERT_TRUE(result2.is_ok());
    EXPECT_EQ(result2.value(), health_check_type::readiness);

    auto result3 = health_check_type_from_string("INVALID");
    EXPECT_TRUE(result3.is_err());
}

// =============================================================================
// lambda_health_check Tests
// =============================================================================

TEST(LambdaHealthCheckTest, BasicCheck) {
    auto check = std::make_shared<lambda_health_check>(
        "test_check", health_check_type::liveness, []() {
            health_check_result result;
            result.status = health_status::healthy;
            result.message = "All good";
            return result;
        });

    EXPECT_EQ(check->get_name(), "test_check");
    EXPECT_EQ(check->get_type(), health_check_type::liveness);
    EXPECT_TRUE(check->is_critical());

    auto result = check->check();
    EXPECT_EQ(result.status, health_status::healthy);
    EXPECT_EQ(result.message, "All good");
}

TEST(LambdaHealthCheckTest, UnhealthyCheck) {
    auto check = std::make_shared<lambda_health_check>(
        "failing_check", health_check_type::dependency, []() {
            health_check_result result;
            result.status = health_status::unhealthy;
            result.message = "Connection failed";
            return result;
        });

    auto result = check->check();
    EXPECT_EQ(result.status, health_status::unhealthy);
    EXPECT_EQ(result.message, "Connection failed");
}

TEST(LambdaHealthCheckTest, CustomTimeoutAndCritical) {
    auto check = std::make_shared<lambda_health_check>(
        "non_critical_check", health_check_type::custom,
        []() {
            health_check_result result;
            result.status = health_status::healthy;
            return result;
        },
        false,  // not critical
        std::chrono::milliseconds{10000});

    EXPECT_FALSE(check->is_critical());
    EXPECT_EQ(check->get_timeout(), std::chrono::milliseconds{10000});
}

// =============================================================================
// composite_health_check Tests
// =============================================================================

TEST(CompositeHealthCheckTest, EmptyComposite) {
    auto composite = std::make_shared<composite_health_check>("composite");

    EXPECT_TRUE(composite->empty());
    EXPECT_EQ(composite->size(), 0);

    auto result = composite->check();
    EXPECT_EQ(result.status, health_status::healthy);
}

TEST(CompositeHealthCheckTest, AllHealthy) {
    auto composite = std::make_shared<composite_health_check>("composite");

    composite->add_check(std::make_shared<lambda_health_check>(
        "check1", health_check_type::liveness, []() {
            health_check_result r;
            r.status = health_status::healthy;
            return r;
        }));

    composite->add_check(std::make_shared<lambda_health_check>(
        "check2", health_check_type::liveness, []() {
            health_check_result r;
            r.status = health_status::healthy;
            return r;
        }));

    EXPECT_EQ(composite->size(), 2);

    auto result = composite->check();
    EXPECT_EQ(result.status, health_status::healthy);
    EXPECT_EQ(result.metadata["total_checks"], "2");
    EXPECT_EQ(result.metadata["healthy_count"], "2");
}

TEST(CompositeHealthCheckTest, OneUnhealthy) {
    auto composite = std::make_shared<composite_health_check>("composite");

    composite->add_check(std::make_shared<lambda_health_check>(
        "healthy", health_check_type::liveness, []() {
            health_check_result r;
            r.status = health_status::healthy;
            return r;
        }));

    composite->add_check(std::make_shared<lambda_health_check>(
        "unhealthy", health_check_type::liveness, []() {
            health_check_result r;
            r.status = health_status::unhealthy;
            r.message = "Failed";
            return r;
        }));

    auto result = composite->check();
    EXPECT_EQ(result.status, health_status::unhealthy);
}

TEST(CompositeHealthCheckTest, RemoveCheck) {
    auto composite = std::make_shared<composite_health_check>("composite");

    composite->add_check(std::make_shared<lambda_health_check>(
        "to_remove", health_check_type::liveness, []() {
            health_check_result r;
            r.status = health_status::healthy;
            return r;
        }));

    EXPECT_EQ(composite->size(), 1);
    EXPECT_TRUE(composite->remove_check("to_remove"));
    EXPECT_EQ(composite->size(), 0);
    EXPECT_FALSE(composite->remove_check("nonexistent"));
}

// =============================================================================
// health_dependency_graph Tests
// =============================================================================

TEST(HealthDependencyGraphTest, AddAndRemoveNodes) {
    health_dependency_graph graph;

    auto check = std::make_shared<lambda_health_check>(
        "node1", health_check_type::liveness, []() {
            health_check_result r;
            r.status = health_status::healthy;
            return r;
        });

    auto result = graph.add_node("node1", check);
    ASSERT_TRUE(result.is_ok());
    EXPECT_TRUE(graph.has_node("node1"));
    EXPECT_EQ(graph.size(), 1);

    // Duplicate add should fail
    auto dup_result = graph.add_node("node1", check);
    EXPECT_TRUE(dup_result.is_err());

    auto remove_result = graph.remove_node("node1");
    ASSERT_TRUE(remove_result.is_ok());
    EXPECT_FALSE(graph.has_node("node1"));
}

TEST(HealthDependencyGraphTest, AddDependency) {
    health_dependency_graph graph;

    auto check1 = std::make_shared<lambda_health_check>(
        "database", health_check_type::dependency, []() {
            health_check_result r;
            r.status = health_status::healthy;
            return r;
        });

    auto check2 = std::make_shared<lambda_health_check>(
        "api", health_check_type::liveness, []() {
            health_check_result r;
            r.status = health_status::healthy;
            return r;
        });

    graph.add_node("database", check1);
    graph.add_node("api", check2);

    // api depends on database
    auto dep_result = graph.add_dependency("api", "database");
    ASSERT_TRUE(dep_result.is_ok());

    auto deps = graph.get_dependencies("api");
    EXPECT_EQ(deps.size(), 1);
    EXPECT_TRUE(deps.count("database") > 0);

    auto dependents = graph.get_dependents("database");
    EXPECT_EQ(dependents.size(), 1);
    EXPECT_TRUE(dependents.count("api") > 0);
}

TEST(HealthDependencyGraphTest, CycleDetection) {
    health_dependency_graph graph;

    auto check_a = std::make_shared<lambda_health_check>(
        "a", health_check_type::liveness, []() {
            health_check_result r;
            r.status = health_status::healthy;
            return r;
        });

    auto check_b = std::make_shared<lambda_health_check>(
        "b", health_check_type::liveness, []() {
            health_check_result r;
            r.status = health_status::healthy;
            return r;
        });

    auto check_c = std::make_shared<lambda_health_check>(
        "c", health_check_type::liveness, []() {
            health_check_result r;
            r.status = health_status::healthy;
            return r;
        });

    graph.add_node("a", check_a);
    graph.add_node("b", check_b);
    graph.add_node("c", check_c);

    // a -> b -> c
    graph.add_dependency("a", "b");
    graph.add_dependency("b", "c");

    // c -> a would create a cycle
    EXPECT_TRUE(graph.would_create_cycle("c", "a"));
    auto cycle_result = graph.add_dependency("c", "a");
    EXPECT_TRUE(cycle_result.is_err());
}

TEST(HealthDependencyGraphTest, TopologicalSort) {
    health_dependency_graph graph;

    auto make_check = [](const std::string& name) {
        return std::make_shared<lambda_health_check>(
            name, health_check_type::liveness, []() {
                health_check_result r;
                r.status = health_status::healthy;
                return r;
            });
    };

    graph.add_node("database", make_check("database"));
    graph.add_node("cache", make_check("cache"));
    graph.add_node("api", make_check("api"));

    // api depends on both database and cache
    graph.add_dependency("api", "database");
    graph.add_dependency("api", "cache");

    auto sort_result = graph.topological_sort();
    ASSERT_TRUE(sort_result.is_ok());

    auto sorted = sort_result.value();
    EXPECT_EQ(sorted.size(), 3);

    // api should come after database and cache
    auto api_pos = std::find(sorted.begin(), sorted.end(), "api");
    auto db_pos = std::find(sorted.begin(), sorted.end(), "database");
    auto cache_pos = std::find(sorted.begin(), sorted.end(), "cache");

    EXPECT_LT(db_pos, api_pos);
    EXPECT_LT(cache_pos, api_pos);
}

TEST(HealthDependencyGraphTest, CheckWithDependencies) {
    health_dependency_graph graph;

    auto healthy_check = std::make_shared<lambda_health_check>(
        "healthy", health_check_type::liveness, []() {
            health_check_result r;
            r.status = health_status::healthy;
            return r;
        });

    auto dependent_check = std::make_shared<lambda_health_check>(
        "dependent", health_check_type::liveness, []() {
            health_check_result r;
            r.status = health_status::healthy;
            return r;
        });

    graph.add_node("healthy", healthy_check);
    graph.add_node("dependent", dependent_check);
    graph.add_dependency("dependent", "healthy");

    auto result = graph.check_with_dependencies("dependent");
    ASSERT_TRUE(result.is_ok());
    EXPECT_EQ(result.value().status, health_status::healthy);
}

TEST(HealthDependencyGraphTest, FailureImpact) {
    health_dependency_graph graph;

    auto make_check = [](const std::string& name) {
        return std::make_shared<lambda_health_check>(
            name, health_check_type::liveness, []() {
                health_check_result r;
                r.status = health_status::healthy;
                return r;
            });
    };

    graph.add_node("database", make_check("database"));
    graph.add_node("cache", make_check("cache"));
    graph.add_node("api", make_check("api"));
    graph.add_node("frontend", make_check("frontend"));

    graph.add_dependency("api", "database");
    graph.add_dependency("api", "cache");
    graph.add_dependency("frontend", "api");

    auto impact = graph.get_failure_impact("database");
    EXPECT_EQ(impact.size(), 2);  // api and frontend
    EXPECT_TRUE(impact.count("api") > 0);
    EXPECT_TRUE(impact.count("frontend") > 0);
}

// =============================================================================
// health_check_builder Tests
// =============================================================================

TEST(HealthCheckBuilderTest, BasicBuild) {
    auto result = health_check_builder()
                      .name("test_check")
                      .type(health_check_type::liveness)
                      .with_check([]() {
                          health_check_result r;
                          r.status = health_status::healthy;
                          return r;
                      })
                      .build();

    ASSERT_TRUE(result.is_ok());
    auto check = result.value();
    EXPECT_EQ(check->get_name(), "test_check");
    EXPECT_EQ(check->get_type(), health_check_type::liveness);
}

TEST(HealthCheckBuilderTest, MissingName) {
    auto result = health_check_builder()
                      .with_check([]() {
                          health_check_result r;
                          r.status = health_status::healthy;
                          return r;
                      })
                      .build();

    EXPECT_TRUE(result.is_err());
}

TEST(HealthCheckBuilderTest, MissingCheckFunction) {
    auto result = health_check_builder().name("test").build();

    EXPECT_TRUE(result.is_err());
}

TEST(HealthCheckBuilderTest, FullConfiguration) {
    auto result = health_check_builder()
                      .name("full_check")
                      .type(health_check_type::dependency)
                      .critical(false)
                      .timeout(std::chrono::milliseconds{10000})
                      .with_check([]() {
                          health_check_result r;
                          r.status = health_status::healthy;
                          return r;
                      })
                      .build();

    ASSERT_TRUE(result.is_ok());
    auto check = result.value();
    EXPECT_EQ(check->get_name(), "full_check");
    EXPECT_EQ(check->get_type(), health_check_type::dependency);
    EXPECT_FALSE(check->is_critical());
    EXPECT_EQ(check->get_timeout(), std::chrono::milliseconds{10000});
}

// =============================================================================
// health_monitor Tests
// =============================================================================

TEST(HealthMonitorTest, RegisterAndUnregister) {
    health_monitor monitor;

    auto check = std::make_shared<lambda_health_check>(
        "test", health_check_type::liveness, []() {
            health_check_result r;
            r.status = health_status::healthy;
            return r;
        });

    auto reg_result = monitor.register_check("test", check);
    ASSERT_TRUE(reg_result.is_ok());
    EXPECT_TRUE(monitor.has_check("test"));

    auto unreg_result = monitor.unregister_check("test");
    ASSERT_TRUE(unreg_result.is_ok());
    EXPECT_FALSE(monitor.has_check("test"));
}

TEST(HealthMonitorTest, CheckExecution) {
    health_monitor monitor;

    auto check = std::make_shared<lambda_health_check>(
        "test", health_check_type::liveness, []() {
            health_check_result r;
            r.status = health_status::healthy;
            r.message = "OK";
            return r;
        });

    monitor.register_check("test", check);

    auto result = monitor.check("test");
    ASSERT_TRUE(result.is_ok());
    EXPECT_EQ(result.value().status, health_status::healthy);
}

TEST(HealthMonitorTest, StartStop) {
    health_monitor monitor;

    EXPECT_FALSE(monitor.is_running());

    auto start_result = monitor.start();
    ASSERT_TRUE(start_result.is_ok());
    EXPECT_TRUE(monitor.is_running());

    // Double start should fail
    auto start2_result = monitor.start();
    EXPECT_TRUE(start2_result.is_err());

    auto stop_result = monitor.stop();
    ASSERT_TRUE(stop_result.is_ok());
    EXPECT_FALSE(monitor.is_running());
}

TEST(HealthMonitorTest, RefreshAndStats) {
    health_monitor monitor;

    monitor.register_check("healthy", std::make_shared<lambda_health_check>(
                                          "healthy", health_check_type::liveness, []() {
                                              health_check_result r;
                                              r.status = health_status::healthy;
                                              return r;
                                          }));

    monitor.register_check("degraded", std::make_shared<lambda_health_check>(
                                           "degraded", health_check_type::liveness, []() {
                                               health_check_result r;
                                               r.status = health_status::degraded;
                                               return r;
                                           }));

    monitor.refresh();

    auto stats = monitor.get_stats();
    EXPECT_EQ(stats.total_checks, 2);
    EXPECT_EQ(stats.healthy_count, 1);
    EXPECT_EQ(stats.degraded_count, 1);
}

TEST(HealthMonitorTest, RecoveryHandler) {
    health_monitor monitor;
    bool recovery_called = false;

    monitor.register_check(
        "failing", std::make_shared<lambda_health_check>(
                       "failing", health_check_type::liveness, []() {
                           health_check_result r;
                           r.status = health_status::unhealthy;
                           return r;
                       }));

    monitor.register_recovery_handler("failing", [&recovery_called]() {
        recovery_called = true;
        return true;
    });

    monitor.check("failing");

    EXPECT_TRUE(recovery_called);
    auto stats = monitor.get_stats();
    EXPECT_EQ(stats.recovery_attempts, 1);
    EXPECT_EQ(stats.successful_recoveries, 1);
}

TEST(HealthMonitorTest, HealthReport) {
    health_monitor monitor;

    monitor.register_check("test", std::make_shared<lambda_health_check>(
                                       "test", health_check_type::liveness, []() {
                                           health_check_result r;
                                           r.status = health_status::healthy;
                                           r.message = "All systems operational";
                                           return r;
                                       }));

    monitor.refresh();

    std::string report = monitor.get_health_report();
    EXPECT_FALSE(report.empty());
    EXPECT_TRUE(report.find("Health Report") != std::string::npos);
    EXPECT_TRUE(report.find("test") != std::string::npos);
}

TEST(HealthMonitorTest, DependencyManagement) {
    health_monitor monitor;

    monitor.register_check("database", std::make_shared<lambda_health_check>(
                                           "database", health_check_type::dependency, []() {
                                               health_check_result r;
                                               r.status = health_status::healthy;
                                               return r;
                                           }));

    monitor.register_check("api", std::make_shared<lambda_health_check>(
                                      "api", health_check_type::liveness, []() {
                                          health_check_result r;
                                          r.status = health_status::healthy;
                                          return r;
                                      }));

    auto dep_result = monitor.add_dependency("api", "database");
    ASSERT_TRUE(dep_result.is_ok());

    auto check_result = monitor.check("api");
    ASSERT_TRUE(check_result.is_ok());
    EXPECT_EQ(check_result.value().status, health_status::healthy);
}

// =============================================================================
// global_health_monitor Tests
// =============================================================================

TEST(GlobalHealthMonitorTest, Singleton) {
    auto& monitor1 = global_health_monitor();
    auto& monitor2 = global_health_monitor();

    // Should be the same instance
    EXPECT_EQ(&monitor1, &monitor2);
}

TEST(GlobalHealthMonitorTest, BasicUsage) {
    auto& monitor = global_health_monitor();

    // Clean up any existing checks
    for (const auto& name : monitor.get_check_names()) {
        monitor.unregister_check(name);
    }

    auto check = std::make_shared<lambda_health_check>(
        "global_test", health_check_type::liveness, []() {
            health_check_result r;
            r.status = health_status::healthy;
            return r;
        });

    monitor.register_check("global_test", check);
    EXPECT_TRUE(monitor.has_check("global_test"));

    monitor.unregister_check("global_test");
    EXPECT_FALSE(monitor.has_check("global_test"));
}
