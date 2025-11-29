// BSD 3-Clause License
// Copyright (c) 2025, kcenon
// See the LICENSE file in the project root for full license information.

#include <gtest/gtest.h>

#include <kcenon/common/config/unified_config.h>

using namespace kcenon::common::config;

// ============================================================================
// Default Values Tests
// ============================================================================

TEST(UnifiedConfigTest, Defaults_ThreadConfig) {
    unified_config config = unified_config::defaults();

    EXPECT_EQ(config.thread.pool_size, 0);
    EXPECT_EQ(config.thread.queue_type, "lockfree");
    EXPECT_EQ(config.thread.max_queue_size, 10000);
    EXPECT_EQ(config.thread.thread_name_prefix, "worker");
}

TEST(UnifiedConfigTest, Defaults_LoggerConfig) {
    unified_config config = unified_config::defaults();

    EXPECT_EQ(config.logger.level, "info");
    EXPECT_EQ(config.logger.writers.size(), 1);
    EXPECT_EQ(config.logger.writers[0], "console");
    EXPECT_TRUE(config.logger.async);
    EXPECT_EQ(config.logger.buffer_size, 8192);
    EXPECT_EQ(config.logger.file_path, "./logs/app.log");
    EXPECT_EQ(config.logger.max_file_size, 10 * 1024 * 1024);
    EXPECT_EQ(config.logger.max_backup_files, 5);
}

TEST(UnifiedConfigTest, Defaults_MonitoringConfig) {
    unified_config config = unified_config::defaults();

    EXPECT_TRUE(config.monitoring.enabled);
    EXPECT_EQ(config.monitoring.metrics_interval.count(), 5000);
    EXPECT_EQ(config.monitoring.health_check_interval.count(), 30000);
    EXPECT_EQ(config.monitoring.prometheus_port, 9090);
    EXPECT_EQ(config.monitoring.prometheus_path, "/metrics");

    // Tracing defaults
    EXPECT_FALSE(config.monitoring.tracing.enabled);
    EXPECT_DOUBLE_EQ(config.monitoring.tracing.sampling_rate, 0.1);
    EXPECT_EQ(config.monitoring.tracing.exporter, "otlp");
    EXPECT_EQ(config.monitoring.tracing.endpoint, "http://localhost:4317");
}

TEST(UnifiedConfigTest, Defaults_DatabaseConfig) {
    unified_config config = unified_config::defaults();

    EXPECT_TRUE(config.database.backend.empty());
    EXPECT_TRUE(config.database.connection_string.empty());
    EXPECT_FALSE(config.database.log_queries);
    EXPECT_EQ(config.database.slow_query_threshold.count(), 1000);

    // Pool defaults
    EXPECT_EQ(config.database.pool.min_size, 5);
    EXPECT_EQ(config.database.pool.max_size, 20);
    EXPECT_EQ(config.database.pool.idle_timeout.count(), 60000);
    EXPECT_EQ(config.database.pool.acquire_timeout.count(), 5000);
}

TEST(UnifiedConfigTest, Defaults_NetworkConfig) {
    unified_config config = unified_config::defaults();

    EXPECT_EQ(config.network.compression, "lz4");
    EXPECT_EQ(config.network.buffer_size, 65536);
    EXPECT_EQ(config.network.connect_timeout.count(), 5000);
    EXPECT_EQ(config.network.io_timeout.count(), 30000);
    EXPECT_EQ(config.network.keepalive_interval.count(), 15000);
    EXPECT_EQ(config.network.max_connections, 10000);

    // TLS defaults
    EXPECT_TRUE(config.network.tls.enabled);
    EXPECT_EQ(config.network.tls.version, "1.3");
    EXPECT_TRUE(config.network.tls.cert_path.empty());
    EXPECT_TRUE(config.network.tls.key_path.empty());
    EXPECT_TRUE(config.network.tls.ca_path.empty());
    EXPECT_TRUE(config.network.tls.verify_peer);
}

// ============================================================================
// Configuration Modification Tests
// ============================================================================

TEST(UnifiedConfigTest, ModifyThreadConfig) {
    unified_config config;

    config.thread.pool_size = 16;
    config.thread.queue_type = "bounded";
    config.thread.max_queue_size = 50000;

    EXPECT_EQ(config.thread.pool_size, 16);
    EXPECT_EQ(config.thread.queue_type, "bounded");
    EXPECT_EQ(config.thread.max_queue_size, 50000);
}

TEST(UnifiedConfigTest, ModifyLoggerConfig) {
    unified_config config;

    config.logger.level = "debug";
    config.logger.writers = {"console", "file", "json"};
    config.logger.async = false;

    EXPECT_EQ(config.logger.level, "debug");
    EXPECT_EQ(config.logger.writers.size(), 3);
    EXPECT_FALSE(config.logger.async);
}

TEST(UnifiedConfigTest, ModifyMonitoringConfig) {
    unified_config config;

    config.monitoring.enabled = false;
    config.monitoring.metrics_interval = std::chrono::milliseconds{10000};
    config.monitoring.tracing.enabled = true;
    config.monitoring.tracing.sampling_rate = 0.5;

    EXPECT_FALSE(config.monitoring.enabled);
    EXPECT_EQ(config.monitoring.metrics_interval.count(), 10000);
    EXPECT_TRUE(config.monitoring.tracing.enabled);
    EXPECT_DOUBLE_EQ(config.monitoring.tracing.sampling_rate, 0.5);
}

TEST(UnifiedConfigTest, ModifyDatabaseConfig) {
    unified_config config;

    config.database.backend = "postgresql";
    config.database.connection_string = "postgresql://localhost:5432/test";
    config.database.pool.max_size = 50;

    EXPECT_EQ(config.database.backend, "postgresql");
    EXPECT_EQ(config.database.connection_string, "postgresql://localhost:5432/test");
    EXPECT_EQ(config.database.pool.max_size, 50);
}

TEST(UnifiedConfigTest, ModifyNetworkConfig) {
    unified_config config;

    config.network.compression = "zstd";
    config.network.buffer_size = 131072;
    config.network.tls.enabled = false;
    config.network.max_connections = 50000;

    EXPECT_EQ(config.network.compression, "zstd");
    EXPECT_EQ(config.network.buffer_size, 131072);
    EXPECT_FALSE(config.network.tls.enabled);
    EXPECT_EQ(config.network.max_connections, 50000);
}

// ============================================================================
// Metadata Tests
// ============================================================================

TEST(UnifiedConfigTest, GetConfigMetadata_NotEmpty) {
    auto metadata = get_config_metadata();

    EXPECT_FALSE(metadata.empty());
    EXPECT_GT(metadata.size(), 10);
}

TEST(UnifiedConfigTest, GetConfigMetadata_HasRequiredFields) {
    auto metadata = get_config_metadata();

    // Check for some expected fields
    bool has_logger_level = false;
    bool has_thread_pool_size = false;
    bool has_database_backend = false;

    for (const auto& field : metadata) {
        if (field.path == "logger.level") {
            has_logger_level = true;
            EXPECT_FALSE(field.env_var.empty());
            EXPECT_FALSE(field.allowed_values.empty());
        }
        if (field.path == "thread.pool_size") {
            has_thread_pool_size = true;
            EXPECT_EQ(field.env_var, "UNIFIED_THREAD_POOL_SIZE");
        }
        if (field.path == "database.backend") {
            has_database_backend = true;
            EXPECT_FALSE(field.allowed_values.empty());
        }
    }

    EXPECT_TRUE(has_logger_level);
    EXPECT_TRUE(has_thread_pool_size);
    EXPECT_TRUE(has_database_backend);
}

// ============================================================================
// Hot Reload Tests
// ============================================================================

TEST(UnifiedConfigTest, IsHotReloadable_LoggerLevel_True) {
    EXPECT_TRUE(is_hot_reloadable("logger.level"));
}

TEST(UnifiedConfigTest, IsHotReloadable_LoggerFilePath_True) {
    EXPECT_TRUE(is_hot_reloadable("logger.file_path"));
}

TEST(UnifiedConfigTest, IsHotReloadable_MonitoringMetricsInterval_True) {
    EXPECT_TRUE(is_hot_reloadable("monitoring.metrics_interval"));
}

TEST(UnifiedConfigTest, IsHotReloadable_TracingSamplingRate_True) {
    EXPECT_TRUE(is_hot_reloadable("monitoring.tracing.sampling_rate"));
}

TEST(UnifiedConfigTest, IsHotReloadable_ThreadPoolSize_False) {
    EXPECT_FALSE(is_hot_reloadable("thread.pool_size"));
}

TEST(UnifiedConfigTest, IsHotReloadable_DatabaseBackend_False) {
    EXPECT_FALSE(is_hot_reloadable("database.backend"));
}

TEST(UnifiedConfigTest, IsHotReloadable_NetworkTlsEnabled_False) {
    EXPECT_FALSE(is_hot_reloadable("network.tls.enabled"));
}

TEST(UnifiedConfigTest, IsHotReloadable_UnknownField_False) {
    EXPECT_FALSE(is_hot_reloadable("unknown.field"));
}

// ============================================================================
// Environment Variable Prefix Test
// ============================================================================

TEST(UnifiedConfigTest, EnvPrefix_IsUnified) {
    EXPECT_STREQ(ENV_PREFIX, "UNIFIED_");
}

// ============================================================================
// Copy and Assignment Tests
// ============================================================================

TEST(UnifiedConfigTest, CopyConstruct) {
    unified_config original;
    original.thread.pool_size = 32;
    original.logger.level = "debug";

    unified_config copy = original;

    EXPECT_EQ(copy.thread.pool_size, 32);
    EXPECT_EQ(copy.logger.level, "debug");
}

TEST(UnifiedConfigTest, MoveConstruct) {
    unified_config original;
    original.thread.pool_size = 32;
    original.logger.level = "debug";

    unified_config moved = std::move(original);

    EXPECT_EQ(moved.thread.pool_size, 32);
    EXPECT_EQ(moved.logger.level, "debug");
}

TEST(UnifiedConfigTest, CopyAssign) {
    unified_config original;
    original.thread.pool_size = 32;

    unified_config copy;
    copy = original;

    EXPECT_EQ(copy.thread.pool_size, 32);
}

TEST(UnifiedConfigTest, MoveAssign) {
    unified_config original;
    original.thread.pool_size = 32;

    unified_config moved;
    moved = std::move(original);

    EXPECT_EQ(moved.thread.pool_size, 32);
}
