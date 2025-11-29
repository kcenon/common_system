// BSD 3-Clause License
// Copyright (c) 2025, kcenon
// See the LICENSE file in the project root for full license information.

/**
 * @file config_loader.h
 * @brief YAML-based configuration loader for the unified system.
 *
 * This header provides the config_loader class for loading configuration
 * from YAML files, environment variables, and merging them with defaults.
 *
 * Configuration Priority (highest to lowest):
 * 1. Environment variables (UNIFIED_*)
 * 2. Configuration file (YAML)
 * 3. Default values
 *
 * Features:
 * - YAML file loading (requires yaml-cpp when BUILD_WITH_YAML_CPP is defined)
 * - Environment variable substitution (${VAR_NAME} syntax)
 * - Environment variable overrides (UNIFIED_* prefix)
 * - Configuration validation
 * - Default value fallback
 *
 * @see TICKET-202 for implementation requirements.
 * @see unified_config.h for configuration schema.
 */

#pragma once

#include "unified_config.h"

#include <kcenon/common/patterns/result.h>

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <regex>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

#ifdef BUILD_WITH_YAML_CPP
#include <yaml-cpp/yaml.h>
#endif

namespace kcenon::common::config {

/**
 * @brief Configuration loading error codes.
 */
namespace config_error_codes {
    constexpr int file_not_found = 1001;
    constexpr int parse_error = 1002;
    constexpr int validation_error = 1003;
    constexpr int invalid_value = 1004;
    constexpr int io_error = 1005;
}

/**
 * @brief Validation result for a configuration field.
 */
struct validation_issue {
    std::string field_path;
    std::string message;
    bool is_warning = false;  // false = error, true = warning
};

/**
 * @class config_loader
 * @brief Loads and validates unified configuration from various sources.
 *
 * The config_loader supports loading configuration from:
 * - YAML files (when BUILD_WITH_YAML_CPP is defined)
 * - Environment variables (UNIFIED_* prefix)
 * - Programmatic defaults
 *
 * Usage Example:
 * @code
 * // Load from file with environment overrides
 * auto result = config_loader::load("config.yaml");
 * if (result.is_ok()) {
 *     auto config = result.value();
 *     // Use config...
 * }
 *
 * // Load from environment only
 * auto env_result = config_loader::load_from_env();
 *
 * // Get defaults
 * auto defaults = config_loader::defaults();
 * @endcode
 */
class config_loader {
public:
    /**
     * @brief Load configuration from a YAML file.
     *
     * Loads the configuration from the specified file path, applies
     * environment variable substitution, and merges with environment
     * variable overrides.
     *
     * @param path Path to the YAML configuration file
     * @return Result containing unified_config or error
     */
    static Result<unified_config> load(const std::string& path) {
#ifdef BUILD_WITH_YAML_CPP
        // Check if file exists
        if (!std::filesystem::exists(path)) {
            return make_error<unified_config>(
                config_error_codes::file_not_found,
                "Configuration file not found: " + path,
                "config_loader"
            );
        }

        try {
            std::ifstream file(path);
            if (!file.is_open()) {
                return make_error<unified_config>(
                    config_error_codes::io_error,
                    "Failed to open configuration file: " + path,
                    "config_loader"
                );
            }

            std::stringstream buffer;
            buffer << file.rdbuf();
            return load_from_string(buffer.str());
        } catch (const std::exception& e) {
            return make_error<unified_config>(
                config_error_codes::io_error,
                std::string("Failed to read configuration file: ") + e.what(),
                "config_loader"
            );
        }
#else
        // Without yaml-cpp, we can only load from environment
        (void)path;  // Suppress unused parameter warning
        return make_error<unified_config>(
            config_error_codes::parse_error,
            "YAML support not available. Build with -DBUILD_WITH_YAML_CPP=ON",
            "config_loader"
        );
#endif
    }

    /**
     * @brief Load configuration from a YAML string.
     *
     * Parses the YAML content, applies environment variable substitution,
     * and merges with environment variable overrides.
     *
     * @param yaml_content YAML content as a string
     * @return Result containing unified_config or error
     */
    static Result<unified_config> load_from_string(const std::string& yaml_content) {
#ifdef BUILD_WITH_YAML_CPP
        try {
            // Expand environment variables in the YAML content
            std::string expanded = expand_env_vars(yaml_content);

            YAML::Node root = YAML::Load(expanded);

            // Start with defaults
            unified_config config = defaults();

            // Parse the YAML into config
            auto parse_result = parse_yaml(root, config);
            if (parse_result.is_err()) {
                return make_error<unified_config>(parse_result.error());
            }

            // Apply environment variable overrides
            auto env_result = merge_env_overrides(config);
            if (env_result.is_err()) {
                return make_error<unified_config>(env_result.error());
            }

            // Validate the configuration
            auto validation_result = validate(config);
            if (validation_result.is_err()) {
                return make_error<unified_config>(validation_result.error());
            }

            return Result<unified_config>::ok(std::move(config));
        } catch (const YAML::ParserException& e) {
            return make_error<unified_config>(
                config_error_codes::parse_error,
                std::string("YAML parse error: ") + e.what(),
                "config_loader"
            );
        } catch (const std::exception& e) {
            return make_error<unified_config>(
                config_error_codes::parse_error,
                std::string("Failed to parse configuration: ") + e.what(),
                "config_loader"
            );
        }
#else
        (void)yaml_content;
        return make_error<unified_config>(
            config_error_codes::parse_error,
            "YAML support not available. Build with -DBUILD_WITH_YAML_CPP=ON",
            "config_loader"
        );
#endif
    }

    /**
     * @brief Load configuration from environment variables only.
     *
     * Creates a default configuration and applies all UNIFIED_*
     * environment variable overrides.
     *
     * @return Result containing unified_config or error
     */
    static Result<unified_config> load_from_env() {
        unified_config config = defaults();

        auto result = merge_env_overrides(config);
        if (result.is_err()) {
            return make_error<unified_config>(result.error());
        }

        auto validation_result = validate(config);
        if (validation_result.is_err()) {
            return make_error<unified_config>(validation_result.error());
        }

        return Result<unified_config>::ok(std::move(config));
    }

    /**
     * @brief Get default configuration.
     * @return unified_config with all default values
     */
    static unified_config defaults() {
        return unified_config::defaults();
    }

    /**
     * @brief Validate a configuration.
     *
     * Checks all configuration values against their allowed ranges
     * and valid options.
     *
     * @param config Configuration to validate
     * @return VoidResult indicating success or validation error
     */
    static VoidResult validate(const unified_config& config) {
        std::vector<validation_issue> issues;

        // Validate thread configuration
        validate_thread_config(config.thread, issues);

        // Validate logger configuration
        validate_logger_config(config.logger, issues);

        // Validate monitoring configuration
        validate_monitoring_config(config.monitoring, issues);

        // Validate database configuration
        validate_database_config(config.database, issues);

        // Validate network configuration
        validate_network_config(config.network, issues);

        // Check for errors (not warnings)
        for (const auto& issue : issues) {
            if (!issue.is_warning) {
                return make_error<std::monostate>(
                    config_error_codes::validation_error,
                    "Validation failed for " + issue.field_path + ": " + issue.message,
                    "config_loader"
                );
            }
        }

        return VoidResult::ok({});
    }

    /**
     * @brief Get all validation issues for a configuration.
     *
     * Returns both errors and warnings.
     *
     * @param config Configuration to validate
     * @return Vector of validation issues
     */
    static std::vector<validation_issue> get_validation_issues(const unified_config& config) {
        std::vector<validation_issue> issues;

        validate_thread_config(config.thread, issues);
        validate_logger_config(config.logger, issues);
        validate_monitoring_config(config.monitoring, issues);
        validate_database_config(config.database, issues);
        validate_network_config(config.network, issues);

        return issues;
    }

    /**
     * @brief Expand environment variables in a string.
     *
     * Replaces ${VAR_NAME} patterns with the corresponding
     * environment variable values. If a variable is not set,
     * the pattern is left unchanged.
     *
     * @param value String containing environment variable references
     * @return String with environment variables expanded
     */
    static std::string expand_env_vars(const std::string& value) {
        static const std::regex env_pattern(R"(\$\{([^}]+)\})");

        std::string result = value;
        std::smatch match;
        std::string::const_iterator search_start = result.cbegin();

        std::string output;
        size_t last_pos = 0;

        while (std::regex_search(search_start, result.cend(), match, env_pattern)) {
            size_t match_pos = static_cast<size_t>(match.position()) +
                              static_cast<size_t>(std::distance(result.cbegin(), search_start));

            // Append text before the match
            output += result.substr(last_pos, match_pos - last_pos);

            // Get environment variable
            std::string var_name = match[1].str();
            const char* env_value = std::getenv(var_name.c_str());

            if (env_value != nullptr) {
                output += env_value;
            } else {
                // Keep the original pattern if variable not found
                output += match[0].str();
            }

            last_pos = match_pos + match[0].length();
            search_start = match.suffix().first;
        }

        // Append remaining text
        output += result.substr(last_pos);

        return output;
    }

private:
    /**
     * @brief Apply environment variable overrides to configuration.
     */
    static VoidResult merge_env_overrides(unified_config& config) {
        // Thread configuration
        apply_env_override("UNIFIED_THREAD_POOL_SIZE", config.thread.pool_size);
        apply_env_override("UNIFIED_THREAD_QUEUE_TYPE", config.thread.queue_type);
        apply_env_override("UNIFIED_THREAD_MAX_QUEUE_SIZE", config.thread.max_queue_size);
        apply_env_override("UNIFIED_THREAD_NAME_PREFIX", config.thread.thread_name_prefix);

        // Logger configuration
        apply_env_override("UNIFIED_LOGGER_LEVEL", config.logger.level);
        apply_env_override_bool("UNIFIED_LOGGER_ASYNC", config.logger.async);
        apply_env_override("UNIFIED_LOGGER_BUFFER_SIZE", config.logger.buffer_size);
        apply_env_override("UNIFIED_LOGGER_FILE_PATH", config.logger.file_path);
        apply_env_override("UNIFIED_LOGGER_MAX_FILE_SIZE", config.logger.max_file_size);
        apply_env_override("UNIFIED_LOGGER_MAX_BACKUP_FILES", config.logger.max_backup_files);
        apply_env_override("UNIFIED_LOGGER_FORMAT_PATTERN", config.logger.format_pattern);
        apply_env_override_vector("UNIFIED_LOGGER_WRITERS", config.logger.writers);

        // Monitoring configuration
        apply_env_override_bool("UNIFIED_MONITORING_ENABLED", config.monitoring.enabled);
        apply_env_override_ms("UNIFIED_MONITORING_METRICS_INTERVAL_MS", config.monitoring.metrics_interval);
        apply_env_override_ms("UNIFIED_MONITORING_HEALTH_CHECK_INTERVAL_MS", config.monitoring.health_check_interval);
        apply_env_override("UNIFIED_MONITORING_PROMETHEUS_PORT", config.monitoring.prometheus_port);
        apply_env_override("UNIFIED_MONITORING_PROMETHEUS_PATH", config.monitoring.prometheus_path);

        // Tracing configuration
        apply_env_override_bool("UNIFIED_MONITORING_TRACING_ENABLED", config.monitoring.tracing.enabled);
        apply_env_override_double("UNIFIED_MONITORING_TRACING_SAMPLING_RATE", config.monitoring.tracing.sampling_rate);
        apply_env_override("UNIFIED_MONITORING_TRACING_EXPORTER", config.monitoring.tracing.exporter);
        apply_env_override("UNIFIED_MONITORING_TRACING_ENDPOINT", config.monitoring.tracing.endpoint);

        // Database configuration
        apply_env_override("UNIFIED_DATABASE_BACKEND", config.database.backend);
        apply_env_override("UNIFIED_DATABASE_CONNECTION_STRING", config.database.connection_string);
        apply_env_override_bool("UNIFIED_DATABASE_LOG_QUERIES", config.database.log_queries);
        apply_env_override_ms("UNIFIED_DATABASE_SLOW_QUERY_THRESHOLD_MS", config.database.slow_query_threshold);
        apply_env_override("UNIFIED_DATABASE_POOL_MIN_SIZE", config.database.pool.min_size);
        apply_env_override("UNIFIED_DATABASE_POOL_MAX_SIZE", config.database.pool.max_size);
        apply_env_override_ms("UNIFIED_DATABASE_POOL_IDLE_TIMEOUT_MS", config.database.pool.idle_timeout);
        apply_env_override_ms("UNIFIED_DATABASE_POOL_ACQUIRE_TIMEOUT_MS", config.database.pool.acquire_timeout);

        // Network configuration
        apply_env_override("UNIFIED_NETWORK_COMPRESSION", config.network.compression);
        apply_env_override("UNIFIED_NETWORK_BUFFER_SIZE", config.network.buffer_size);
        apply_env_override_ms("UNIFIED_NETWORK_CONNECT_TIMEOUT_MS", config.network.connect_timeout);
        apply_env_override_ms("UNIFIED_NETWORK_IO_TIMEOUT_MS", config.network.io_timeout);
        apply_env_override_ms("UNIFIED_NETWORK_KEEPALIVE_INTERVAL_MS", config.network.keepalive_interval);
        apply_env_override("UNIFIED_NETWORK_MAX_CONNECTIONS", config.network.max_connections);

        // TLS configuration
        apply_env_override_bool("UNIFIED_NETWORK_TLS_ENABLED", config.network.tls.enabled);
        apply_env_override("UNIFIED_NETWORK_TLS_VERSION", config.network.tls.version);
        apply_env_override("UNIFIED_NETWORK_TLS_CERT_PATH", config.network.tls.cert_path);
        apply_env_override("UNIFIED_NETWORK_TLS_KEY_PATH", config.network.tls.key_path);
        apply_env_override("UNIFIED_NETWORK_TLS_CA_PATH", config.network.tls.ca_path);
        apply_env_override_bool("UNIFIED_NETWORK_TLS_VERIFY_PEER", config.network.tls.verify_peer);

        return VoidResult::ok({});
    }

    // Environment variable helpers

    static void apply_env_override(const char* env_name, std::string& target) {
        const char* value = std::getenv(env_name);
        if (value != nullptr) {
            target = value;
        }
    }

    static void apply_env_override(const char* env_name, size_t& target) {
        const char* value = std::getenv(env_name);
        if (value != nullptr) {
            try {
                target = std::stoull(value);
            } catch (...) {
                // Ignore invalid values
            }
        }
    }

    static void apply_env_override(const char* env_name, uint16_t& target) {
        const char* value = std::getenv(env_name);
        if (value != nullptr) {
            try {
                auto parsed = std::stoul(value);
                if (parsed <= 65535) {
                    target = static_cast<uint16_t>(parsed);
                }
            } catch (...) {
                // Ignore invalid values
            }
        }
    }

    static void apply_env_override_bool(const char* env_name, bool& target) {
        const char* value = std::getenv(env_name);
        if (value != nullptr) {
            std::string str_value(value);
            // Convert to lowercase for comparison
            std::transform(str_value.begin(), str_value.end(), str_value.begin(),
                          [](unsigned char c) { return std::tolower(c); });

            if (str_value == "true" || str_value == "1" || str_value == "yes" || str_value == "on") {
                target = true;
            } else if (str_value == "false" || str_value == "0" || str_value == "no" || str_value == "off") {
                target = false;
            }
        }
    }

    static void apply_env_override_double(const char* env_name, double& target) {
        const char* value = std::getenv(env_name);
        if (value != nullptr) {
            try {
                target = std::stod(value);
            } catch (...) {
                // Ignore invalid values
            }
        }
    }

    static void apply_env_override_ms(const char* env_name, std::chrono::milliseconds& target) {
        const char* value = std::getenv(env_name);
        if (value != nullptr) {
            try {
                target = std::chrono::milliseconds{std::stoll(value)};
            } catch (...) {
                // Ignore invalid values
            }
        }
    }

    static void apply_env_override_vector(const char* env_name, std::vector<std::string>& target) {
        const char* value = std::getenv(env_name);
        if (value != nullptr) {
            target.clear();
            std::string str_value(value);
            std::stringstream ss(str_value);
            std::string item;
            while (std::getline(ss, item, ',')) {
                // Trim whitespace
                size_t start = item.find_first_not_of(" \t");
                size_t end = item.find_last_not_of(" \t");
                if (start != std::string::npos && end != std::string::npos) {
                    target.push_back(item.substr(start, end - start + 1));
                }
            }
        }
    }

#ifdef BUILD_WITH_YAML_CPP
    /**
     * @brief Parse YAML node into unified_config.
     */
    static VoidResult parse_yaml(const YAML::Node& root, unified_config& config) {
        try {
            // Look for unified_system key or use root directly
            YAML::Node system_node = root["unified_system"];
            if (!system_node) {
                system_node = root;
            }

            // Parse thread configuration
            if (system_node["thread"]) {
                parse_thread_config(system_node["thread"], config.thread);
            }

            // Parse logger configuration
            if (system_node["logger"]) {
                parse_logger_config(system_node["logger"], config.logger);
            }

            // Parse monitoring configuration
            if (system_node["monitoring"]) {
                parse_monitoring_config(system_node["monitoring"], config.monitoring);
            }

            // Parse database configuration
            if (system_node["database"]) {
                parse_database_config(system_node["database"], config.database);
            }

            // Parse network configuration
            if (system_node["network"]) {
                parse_network_config(system_node["network"], config.network);
            }

            return VoidResult::ok({});
        } catch (const YAML::Exception& e) {
            return make_error<std::monostate>(
                config_error_codes::parse_error,
                std::string("YAML parse error: ") + e.what(),
                "config_loader"
            );
        }
    }

    static void parse_thread_config(const YAML::Node& node, thread_config& config) {
        if (node["pool_size"]) {
            config.pool_size = node["pool_size"].as<size_t>();
        }
        if (node["queue_type"]) {
            config.queue_type = node["queue_type"].as<std::string>();
        }
        if (node["max_queue_size"]) {
            config.max_queue_size = node["max_queue_size"].as<size_t>();
        }
        if (node["thread_name_prefix"]) {
            config.thread_name_prefix = node["thread_name_prefix"].as<std::string>();
        }
    }

    static void parse_logger_config(const YAML::Node& node, logger_config& config) {
        if (node["level"]) {
            config.level = node["level"].as<std::string>();
        }
        if (node["writers"]) {
            config.writers.clear();
            for (const auto& writer : node["writers"]) {
                config.writers.push_back(writer.as<std::string>());
            }
        }
        if (node["async"]) {
            config.async = node["async"].as<bool>();
        }
        if (node["buffer_size"]) {
            config.buffer_size = node["buffer_size"].as<size_t>();
        }
        if (node["file_path"]) {
            config.file_path = node["file_path"].as<std::string>();
        }
        if (node["max_file_size"]) {
            config.max_file_size = node["max_file_size"].as<size_t>();
        }
        if (node["max_backup_files"]) {
            config.max_backup_files = node["max_backup_files"].as<size_t>();
        }
        if (node["format_pattern"]) {
            config.format_pattern = node["format_pattern"].as<std::string>();
        }
    }

    static void parse_monitoring_config(const YAML::Node& node, monitoring_config& config) {
        if (node["enabled"]) {
            config.enabled = node["enabled"].as<bool>();
        }
        if (node["metrics_interval_ms"]) {
            config.metrics_interval = std::chrono::milliseconds{node["metrics_interval_ms"].as<int64_t>()};
        }
        if (node["health_check_interval_ms"]) {
            config.health_check_interval = std::chrono::milliseconds{node["health_check_interval_ms"].as<int64_t>()};
        }
        if (node["prometheus_port"]) {
            config.prometheus_port = node["prometheus_port"].as<uint16_t>();
        }
        if (node["prometheus_path"]) {
            config.prometheus_path = node["prometheus_path"].as<std::string>();
        }

        // Tracing configuration
        if (node["tracing"]) {
            auto tracing = node["tracing"];
            if (tracing["enabled"]) {
                config.tracing.enabled = tracing["enabled"].as<bool>();
            }
            if (tracing["sampling_rate"]) {
                config.tracing.sampling_rate = tracing["sampling_rate"].as<double>();
            }
            if (tracing["exporter"]) {
                config.tracing.exporter = tracing["exporter"].as<std::string>();
            }
            if (tracing["endpoint"]) {
                config.tracing.endpoint = tracing["endpoint"].as<std::string>();
            }
        }
    }

    static void parse_database_config(const YAML::Node& node, database_config& config) {
        if (node["backend"]) {
            config.backend = node["backend"].as<std::string>();
        }
        if (node["connection_string"]) {
            config.connection_string = node["connection_string"].as<std::string>();
        }
        if (node["log_queries"]) {
            config.log_queries = node["log_queries"].as<bool>();
        }
        if (node["slow_query_threshold_ms"]) {
            config.slow_query_threshold = std::chrono::milliseconds{node["slow_query_threshold_ms"].as<int64_t>()};
        }

        // Pool configuration
        if (node["pool"]) {
            auto pool = node["pool"];
            if (pool["min_size"]) {
                config.pool.min_size = pool["min_size"].as<size_t>();
            }
            if (pool["max_size"]) {
                config.pool.max_size = pool["max_size"].as<size_t>();
            }
            if (pool["idle_timeout_ms"]) {
                config.pool.idle_timeout = std::chrono::milliseconds{pool["idle_timeout_ms"].as<int64_t>()};
            }
            if (pool["acquire_timeout_ms"]) {
                config.pool.acquire_timeout = std::chrono::milliseconds{pool["acquire_timeout_ms"].as<int64_t>()};
            }
        }
    }

    static void parse_network_config(const YAML::Node& node, network_config& config) {
        if (node["compression"]) {
            config.compression = node["compression"].as<std::string>();
        }
        if (node["buffer_size"]) {
            config.buffer_size = node["buffer_size"].as<size_t>();
        }
        if (node["connect_timeout_ms"]) {
            config.connect_timeout = std::chrono::milliseconds{node["connect_timeout_ms"].as<int64_t>()};
        }
        if (node["io_timeout_ms"]) {
            config.io_timeout = std::chrono::milliseconds{node["io_timeout_ms"].as<int64_t>()};
        }
        if (node["keepalive_interval_ms"]) {
            config.keepalive_interval = std::chrono::milliseconds{node["keepalive_interval_ms"].as<int64_t>()};
        }
        if (node["max_connections"]) {
            config.max_connections = node["max_connections"].as<size_t>();
        }

        // TLS configuration
        if (node["tls"]) {
            auto tls = node["tls"];
            if (tls["enabled"]) {
                config.tls.enabled = tls["enabled"].as<bool>();
            }
            if (tls["version"]) {
                config.tls.version = tls["version"].as<std::string>();
            }
            if (tls["cert_path"]) {
                config.tls.cert_path = tls["cert_path"].as<std::string>();
            }
            if (tls["key_path"]) {
                config.tls.key_path = tls["key_path"].as<std::string>();
            }
            if (tls["ca_path"]) {
                config.tls.ca_path = tls["ca_path"].as<std::string>();
            }
            if (tls["verify_peer"]) {
                config.tls.verify_peer = tls["verify_peer"].as<bool>();
            }
        }
    }
#endif

    // Validation helpers

    static void validate_thread_config(const thread_config& config, std::vector<validation_issue>& issues) {
        // Queue type validation
        static const std::vector<std::string> valid_queue_types = {"mutex", "lockfree", "bounded"};
        if (std::find(valid_queue_types.begin(), valid_queue_types.end(), config.queue_type) == valid_queue_types.end()) {
            issues.push_back({"thread.queue_type",
                            "Invalid queue type: " + config.queue_type + ". Valid values: mutex, lockfree, bounded",
                            false});
        }

        // Max queue size validation
        if (config.max_queue_size == 0) {
            issues.push_back({"thread.max_queue_size", "Queue size must be greater than 0", false});
        }
    }

    static void validate_logger_config(const logger_config& config, std::vector<validation_issue>& issues) {
        // Log level validation
        static const std::vector<std::string> valid_levels = {"trace", "debug", "info", "warn", "error", "critical", "off"};
        if (std::find(valid_levels.begin(), valid_levels.end(), config.level) == valid_levels.end()) {
            issues.push_back({"logger.level",
                            "Invalid log level: " + config.level + ". Valid values: trace, debug, info, warn, error, critical, off",
                            false});
        }

        // Writers validation
        static const std::vector<std::string> valid_writers = {"console", "file", "rotating_file", "network", "json"};
        for (const auto& writer : config.writers) {
            if (std::find(valid_writers.begin(), valid_writers.end(), writer) == valid_writers.end()) {
                issues.push_back({"logger.writers",
                                "Invalid writer: " + writer + ". Valid values: console, file, rotating_file, network, json",
                                false});
            }
        }

        // Buffer size warning
        if (config.async && config.buffer_size < 1024) {
            issues.push_back({"logger.buffer_size",
                            "Buffer size is very small for async logging. Consider using at least 1024 bytes.",
                            true});
        }
    }

    static void validate_monitoring_config(const monitoring_config& config, std::vector<validation_issue>& issues) {
        // Sampling rate validation
        if (config.tracing.sampling_rate < 0.0 || config.tracing.sampling_rate > 1.0) {
            issues.push_back({"monitoring.tracing.sampling_rate",
                            "Sampling rate must be between 0.0 and 1.0",
                            false});
        }

        // Exporter validation
        static const std::vector<std::string> valid_exporters = {"otlp", "jaeger", "zipkin", "console"};
        if (std::find(valid_exporters.begin(), valid_exporters.end(), config.tracing.exporter) == valid_exporters.end()) {
            issues.push_back({"monitoring.tracing.exporter",
                            "Invalid exporter: " + config.tracing.exporter + ". Valid values: otlp, jaeger, zipkin, console",
                            false});
        }

        // Metrics interval warning
        if (config.metrics_interval.count() < 1000) {
            issues.push_back({"monitoring.metrics_interval",
                            "Metrics interval is very short (<1s). This may cause performance issues.",
                            true});
        }
    }

    static void validate_database_config(const database_config& config, std::vector<validation_issue>& issues) {
        // Backend validation (if specified)
        if (!config.backend.empty()) {
            static const std::vector<std::string> valid_backends = {"postgresql", "mysql", "sqlite", "mongodb", "redis"};
            if (std::find(valid_backends.begin(), valid_backends.end(), config.backend) == valid_backends.end()) {
                issues.push_back({"database.backend",
                                "Invalid backend: " + config.backend + ". Valid values: postgresql, mysql, sqlite, mongodb, redis",
                                false});
            }
        }

        // Pool size validation
        if (config.pool.min_size > config.pool.max_size) {
            issues.push_back({"database.pool",
                            "min_size cannot be greater than max_size",
                            false});
        }

        if (config.pool.max_size == 0) {
            issues.push_back({"database.pool.max_size",
                            "Pool max_size must be greater than 0",
                            false});
        }
    }

    static void validate_network_config(const network_config& config, std::vector<validation_issue>& issues) {
        // Compression validation
        static const std::vector<std::string> valid_compressions = {"none", "lz4", "gzip", "deflate", "zstd"};
        if (std::find(valid_compressions.begin(), valid_compressions.end(), config.compression) == valid_compressions.end()) {
            issues.push_back({"network.compression",
                            "Invalid compression: " + config.compression + ". Valid values: none, lz4, gzip, deflate, zstd",
                            false});
        }

        // TLS version validation
        static const std::vector<std::string> valid_tls_versions = {"1.2", "1.3"};
        if (std::find(valid_tls_versions.begin(), valid_tls_versions.end(), config.tls.version) == valid_tls_versions.end()) {
            issues.push_back({"network.tls.version",
                            "Invalid TLS version: " + config.tls.version + ". Valid values: 1.2, 1.3",
                            false});
        }

        // Buffer size warning
        if (config.buffer_size < 4096) {
            issues.push_back({"network.buffer_size",
                            "Buffer size is very small (<4KB). This may cause performance issues.",
                            true});
        }

        // TLS certificate validation
        if (config.tls.enabled && config.tls.verify_peer && config.tls.ca_path.empty()) {
            issues.push_back({"network.tls.ca_path",
                            "TLS is enabled with verify_peer but no CA path specified.",
                            true});
        }
    }
};

}  // namespace kcenon::common::config
