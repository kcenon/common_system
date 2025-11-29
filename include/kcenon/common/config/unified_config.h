// BSD 3-Clause License
// Copyright (c) 2025, kcenon
// See the LICENSE file in the project root for full license information.

/**
 * @file unified_config.h
 * @brief Unified configuration schema for the entire system.
 *
 * This header defines the configuration structures for all subsystems.
 * It provides a type-safe, hierarchical configuration schema with
 * default values and validation support.
 *
 * Configuration Priority (highest to lowest):
 * 1. CLI arguments (--set key=value)
 * 2. Environment variables (UNIFIED_*)
 * 3. Configuration file (YAML)
 * 4. Default values
 *
 * @see TICKET-201 for design requirements.
 * @see config_loader.h for loading implementation.
 */

#pragma once

#include <chrono>
#include <optional>
#include <string>
#include <vector>

namespace kcenon::common::config {

/**
 * @struct thread_config
 * @brief Thread pool configuration.
 */
struct thread_config {
    /// Number of worker threads (default: hardware concurrency)
    size_t pool_size = 0;  // 0 means auto-detect

    /// Queue type: "mutex", "lockfree", "bounded"
    std::string queue_type = "lockfree";

    /// Maximum queue size (for bounded queue)
    size_t max_queue_size = 10000;

    /// Thread naming prefix
    std::string thread_name_prefix = "worker";
};

/**
 * @struct logger_config
 * @brief Logging system configuration.
 */
struct logger_config {
    /// Log level: "trace", "debug", "info", "warn", "error", "critical", "off"
    std::string level = "info";

    /// List of writers: "console", "file", "rotating_file", "network", "json"
    std::vector<std::string> writers = {"console"};

    /// Enable async logging
    bool async = true;

    /// Async buffer size in bytes
    size_t buffer_size = 8192;

    /// Log file path (for file writers)
    std::string file_path = "./logs/app.log";

    /// Maximum file size in bytes (for rotating_file)
    size_t max_file_size = 10 * 1024 * 1024;  // 10MB

    /// Maximum number of backup files (for rotating_file)
    size_t max_backup_files = 5;

    /// Log format pattern
    std::string format_pattern = "[%Y-%m-%d %H:%M:%S.%e] [%l] [%t] %v";
};

/**
 * @struct tracing_config
 * @brief Distributed tracing configuration.
 */
struct tracing_config {
    /// Enable tracing
    bool enabled = false;

    /// Sampling rate (0.0 to 1.0)
    double sampling_rate = 0.1;

    /// Exporter type: "otlp", "jaeger", "zipkin", "console"
    std::string exporter = "otlp";

    /// Exporter endpoint
    std::string endpoint = "http://localhost:4317";
};

/**
 * @struct monitoring_config
 * @brief Monitoring system configuration.
 */
struct monitoring_config {
    /// Enable monitoring
    bool enabled = true;

    /// Metrics collection interval
    std::chrono::milliseconds metrics_interval{5000};

    /// Health check interval
    std::chrono::milliseconds health_check_interval{30000};

    /// Tracing configuration
    tracing_config tracing;

    /// Prometheus metrics port (0 to disable)
    uint16_t prometheus_port = 9090;

    /// Prometheus metrics path
    std::string prometheus_path = "/metrics";
};

/**
 * @struct pool_config
 * @brief Database connection pool configuration.
 */
struct pool_config {
    /// Minimum pool size
    size_t min_size = 5;

    /// Maximum pool size
    size_t max_size = 20;

    /// Idle connection timeout
    std::chrono::milliseconds idle_timeout{60000};

    /// Connection acquisition timeout
    std::chrono::milliseconds acquire_timeout{5000};
};

/**
 * @struct database_config
 * @brief Database system configuration.
 */
struct database_config {
    /// Database backend: "postgresql", "mysql", "sqlite", "mongodb", "redis"
    std::string backend;

    /// Connection string or URI
    std::string connection_string;

    /// Connection pool configuration
    pool_config pool;

    /// Enable query logging
    bool log_queries = false;

    /// Slow query threshold
    std::chrono::milliseconds slow_query_threshold{1000};
};

/**
 * @struct tls_config
 * @brief TLS/SSL configuration.
 */
struct tls_config {
    /// Enable TLS
    bool enabled = true;

    /// TLS version: "1.2", "1.3"
    std::string version = "1.3";

    /// Certificate file path
    std::string cert_path;

    /// Private key file path
    std::string key_path;

    /// CA certificate path (for client verification)
    std::string ca_path;

    /// Verify peer certificate
    bool verify_peer = true;
};

/**
 * @struct network_config
 * @brief Network system configuration.
 */
struct network_config {
    /// TLS configuration
    tls_config tls;

    /// Compression type: "none", "lz4", "gzip", "deflate", "zstd"
    std::string compression = "lz4";

    /// Send/receive buffer size
    size_t buffer_size = 65536;

    /// Connection timeout
    std::chrono::milliseconds connect_timeout{5000};

    /// Read/write timeout
    std::chrono::milliseconds io_timeout{30000};

    /// Keep-alive interval
    std::chrono::milliseconds keepalive_interval{15000};

    /// Maximum concurrent connections (server)
    size_t max_connections = 10000;
};

/**
 * @struct unified_config
 * @brief Root configuration structure for the unified system.
 *
 * This structure contains all subsystem configurations and provides
 * default values for all settings.
 */
struct unified_config {
    /// Thread system configuration
    thread_config thread;

    /// Logger system configuration
    logger_config logger;

    /// Monitoring system configuration
    monitoring_config monitoring;

    /// Database system configuration
    database_config database;

    /// Network system configuration
    network_config network;

    /**
     * @brief Create a configuration with all default values.
     * @return Default unified_config instance
     */
    static unified_config defaults() {
        return unified_config{};
    }
};

/**
 * @brief Environment variable prefix for configuration overrides.
 *
 * All environment variables should be prefixed with UNIFIED_ and use
 * underscores to separate nested keys.
 *
 * Examples:
 * - UNIFIED_THREAD_POOL_SIZE=16
 * - UNIFIED_LOGGER_LEVEL=debug
 * - UNIFIED_MONITORING_ENABLED=false
 * - UNIFIED_DATABASE_CONNECTION_STRING=postgresql://localhost/mydb
 * - UNIFIED_NETWORK_TLS_ENABLED=true
 */
constexpr const char* ENV_PREFIX = "UNIFIED_";

/**
 * @brief Configuration field metadata for validation and documentation.
 */
struct field_metadata {
    /// Field path (e.g., "logger.level")
    std::string path;

    /// Human-readable description
    std::string description;

    /// Whether the field can be hot-reloaded
    bool hot_reloadable = false;

    /// Environment variable name (if applicable)
    std::string env_var;

    /// Allowed values (for enum-like fields)
    std::vector<std::string> allowed_values;
};

/**
 * @brief Get metadata for all configuration fields.
 * @return Vector of field metadata
 */
inline std::vector<field_metadata> get_config_metadata() {
    return {
        // Thread configuration
        {"thread.pool_size", "Number of worker threads (0 for auto)", false,
         "UNIFIED_THREAD_POOL_SIZE", {}},
        {"thread.queue_type", "Task queue type", false,
         "UNIFIED_THREAD_QUEUE_TYPE", {"mutex", "lockfree", "bounded"}},
        {"thread.max_queue_size", "Maximum task queue size", false,
         "UNIFIED_THREAD_MAX_QUEUE_SIZE", {}},

        // Logger configuration
        {"logger.level", "Log level", true,
         "UNIFIED_LOGGER_LEVEL",
         {"trace", "debug", "info", "warn", "error", "critical", "off"}},
        {"logger.async", "Enable async logging", false,
         "UNIFIED_LOGGER_ASYNC", {}},
        {"logger.buffer_size", "Async buffer size", false,
         "UNIFIED_LOGGER_BUFFER_SIZE", {}},
        {"logger.file_path", "Log file path", true,
         "UNIFIED_LOGGER_FILE_PATH", {}},

        // Monitoring configuration
        {"monitoring.enabled", "Enable monitoring", false,
         "UNIFIED_MONITORING_ENABLED", {}},
        {"monitoring.metrics_interval", "Metrics collection interval (ms)", true,
         "UNIFIED_MONITORING_METRICS_INTERVAL_MS", {}},
        {"monitoring.tracing.enabled", "Enable distributed tracing", false,
         "UNIFIED_MONITORING_TRACING_ENABLED", {}},
        {"monitoring.tracing.sampling_rate", "Trace sampling rate", true,
         "UNIFIED_MONITORING_TRACING_SAMPLING_RATE", {}},

        // Database configuration
        {"database.backend", "Database backend type", false,
         "UNIFIED_DATABASE_BACKEND",
         {"postgresql", "mysql", "sqlite", "mongodb", "redis"}},
        {"database.connection_string", "Database connection string", false,
         "UNIFIED_DATABASE_CONNECTION_STRING", {}},
        {"database.pool.min_size", "Minimum pool size", false,
         "UNIFIED_DATABASE_POOL_MIN_SIZE", {}},
        {"database.pool.max_size", "Maximum pool size", false,
         "UNIFIED_DATABASE_POOL_MAX_SIZE", {}},

        // Network configuration
        {"network.tls.enabled", "Enable TLS", false,
         "UNIFIED_NETWORK_TLS_ENABLED", {}},
        {"network.tls.version", "TLS version", false,
         "UNIFIED_NETWORK_TLS_VERSION", {"1.2", "1.3"}},
        {"network.compression", "Compression algorithm", false,
         "UNIFIED_NETWORK_COMPRESSION", {"none", "lz4", "gzip", "deflate", "zstd"}},
        {"network.buffer_size", "I/O buffer size", false,
         "UNIFIED_NETWORK_BUFFER_SIZE", {}},
    };
}

/**
 * @brief Check if a configuration field supports hot-reload.
 * @param field_path The dot-separated field path
 * @return true if the field can be changed at runtime
 */
inline bool is_hot_reloadable(const std::string& field_path) {
    static const std::vector<std::string> hot_reloadable_fields = {
        "logger.level",
        "logger.file_path",
        "monitoring.metrics_interval",
        "monitoring.tracing.sampling_rate",
    };

    for (const auto& field : hot_reloadable_fields) {
        if (field_path == field) {
            return true;
        }
    }
    return false;
}

}  // namespace kcenon::common::config
