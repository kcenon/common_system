// BSD 3-Clause License
// Copyright (c) 2025, kcenon
// See the LICENSE file in the project root for full license information.

/**
 * @file cli_config_parser.h
 * @brief Command-line interface configuration parser.
 *
 * This header provides the cli_config_parser class for parsing command-line
 * arguments and applying configuration overrides.
 *
 * Supported arguments:
 * - --config=<path>    Load configuration from YAML file
 * - --set key=value    Override a specific configuration value
 * - --help, -h         Show help message
 * - --version, -v      Show version information
 *
 * Configuration Priority (highest to lowest):
 * 1. CLI arguments (--set key=value)
 * 2. Environment variables (UNIFIED_*)
 * 3. Configuration file (YAML)
 * 4. Default values
 *
 * @see TICKET-204 for implementation requirements.
 * @see config_loader.h for loading implementation.
 */

#pragma once

#include "config_loader.h"
#include "unified_config.h"

#include <kcenon/common/patterns/result.h>

#include <algorithm>
#include <iostream>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace kcenon::common::config {

/**
 * @brief CLI parsing error codes.
 */
namespace cli_error_codes {
    constexpr int invalid_argument = 2001;
    constexpr int missing_value = 2002;
    constexpr int invalid_key = 2003;
    constexpr int invalid_format = 2004;
}

/**
 * @struct parsed_args
 * @brief Parsed command-line arguments.
 */
struct parsed_args {
    /// Configuration file path (from --config)
    std::string config_path;

    /// Configuration overrides (from --set key=value)
    std::vector<std::pair<std::string, std::string>> overrides;

    /// Show help flag
    bool show_help = false;

    /// Show version flag
    bool show_version = false;

    /// Unparsed positional arguments
    std::vector<std::string> positional_args;
};

/**
 * @class cli_config_parser
 * @brief Parses command-line arguments for configuration.
 *
 * Usage Example:
 * @code
 * int main(int argc, char** argv) {
 *     auto result = cli_config_parser::load_with_cli_overrides(argc, argv);
 *     if (result.is_err()) {
 *         if (result.error().code == cli_error_codes::invalid_argument) {
 *             cli_config_parser::print_help(argv[0]);
 *         }
 *         return 1;
 *     }
 *
 *     auto config = result.value();
 *     // Use config...
 * }
 * @endcode
 */
class cli_config_parser {
public:
    /**
     * @brief Parse command-line arguments.
     *
     * Parses the given arguments and returns a parsed_args structure.
     *
     * @param argc Number of arguments
     * @param argv Argument array
     * @return Result containing parsed_args or error
     */
    static Result<parsed_args> parse(int argc, char** argv) {
        parsed_args result;

        for (int i = 1; i < argc; ++i) {
            std::string arg = argv[i];

            if (arg == "--help" || arg == "-h") {
                result.show_help = true;
            } else if (arg == "--version" || arg == "-v") {
                result.show_version = true;
            } else if (starts_with(arg, "--config=")) {
                result.config_path = arg.substr(9);
            } else if (arg == "--config") {
                if (i + 1 >= argc) {
                    return make_error<parsed_args>(
                        cli_error_codes::missing_value,
                        "Missing value for --config",
                        "cli_config_parser"
                    );
                }
                result.config_path = argv[++i];
            } else if (starts_with(arg, "--set=")) {
                auto kv_result = parse_key_value(arg.substr(6));
                if (kv_result.is_err()) {
                    return make_error<parsed_args>(kv_result.error());
                }
                result.overrides.push_back(kv_result.value());
            } else if (arg == "--set") {
                if (i + 1 >= argc) {
                    return make_error<parsed_args>(
                        cli_error_codes::missing_value,
                        "Missing value for --set",
                        "cli_config_parser"
                    );
                }
                auto kv_result = parse_key_value(argv[++i]);
                if (kv_result.is_err()) {
                    return make_error<parsed_args>(kv_result.error());
                }
                result.overrides.push_back(kv_result.value());
            } else if (starts_with(arg, "--")) {
                return make_error<parsed_args>(
                    cli_error_codes::invalid_argument,
                    "Unknown argument: " + arg,
                    "cli_config_parser"
                );
            } else if (starts_with(arg, "-")) {
                return make_error<parsed_args>(
                    cli_error_codes::invalid_argument,
                    "Unknown short argument: " + arg,
                    "cli_config_parser"
                );
            } else {
                // Positional argument
                result.positional_args.push_back(arg);
            }
        }

        return Result<parsed_args>::ok(std::move(result));
    }

    /**
     * @brief Load configuration with CLI overrides.
     *
     * Parses CLI arguments, loads configuration from file (if specified),
     * applies environment variable overrides, and then applies CLI overrides.
     *
     * Priority (highest to lowest):
     * 1. --set key=value overrides
     * 2. Environment variables (UNIFIED_*)
     * 3. Configuration file
     * 4. Defaults
     *
     * @param argc Number of arguments
     * @param argv Argument array
     * @return Result containing unified_config or error
     */
    static Result<unified_config> load_with_cli_overrides(int argc, char** argv) {
        // Parse CLI arguments
        auto parse_result = parse(argc, argv);
        if (parse_result.is_err()) {
            return make_error<unified_config>(parse_result.error());
        }

        auto args = parse_result.value();

        // Handle --help and --version early
        if (args.show_help || args.show_version) {
            // Return an "error" to indicate the caller should handle this
            return make_error<unified_config>(
                args.show_help ? cli_error_codes::invalid_argument : cli_error_codes::invalid_argument,
                args.show_help ? "help_requested" : "version_requested",
                "cli_config_parser"
            );
        }

        // Load configuration
        unified_config config;

        if (!args.config_path.empty()) {
            // Load from file
            auto load_result = config_loader::load(args.config_path);
            if (load_result.is_err()) {
                return make_error<unified_config>(load_result.error());
            }
            config = load_result.value();
        } else {
            // Load from environment only
            auto load_result = config_loader::load_from_env();
            if (load_result.is_err()) {
                return make_error<unified_config>(load_result.error());
            }
            config = load_result.value();
        }

        // Apply CLI overrides
        for (const auto& [key, value] : args.overrides) {
            auto result = apply_override(config, key, value);
            if (result.is_err()) {
                return make_error<unified_config>(result.error());
            }
        }

        // Validate final configuration
        auto validation_result = config_loader::validate(config);
        if (validation_result.is_err()) {
            return make_error<unified_config>(validation_result.error());
        }

        return Result<unified_config>::ok(std::move(config));
    }

    /**
     * @brief Print help message to stdout.
     *
     * @param program_name Name of the program (usually argv[0])
     */
    static void print_help(const std::string& program_name = "program") {
        std::cout << "Usage: " << program_name << " [OPTIONS]\n\n"
                  << "Options:\n"
                  << "  --config=<path>     Load configuration from YAML file\n"
                  << "  --set <key>=<value> Override a configuration value\n"
                  << "  --help, -h          Show this help message\n"
                  << "  --version, -v       Show version information\n\n"
                  << "Configuration keys:\n";

        // Print available configuration keys
        auto metadata = get_config_metadata();
        for (const auto& field : metadata) {
            std::cout << "  " << field.path;
            if (!field.allowed_values.empty()) {
                std::cout << " (";
                for (size_t i = 0; i < field.allowed_values.size(); ++i) {
                    if (i > 0) std::cout << "|";
                    std::cout << field.allowed_values[i];
                }
                std::cout << ")";
            }
            std::cout << "\n    " << field.description;
            if (!field.env_var.empty()) {
                std::cout << " [env: " << field.env_var << "]";
            }
            std::cout << "\n";
        }

        std::cout << "\nExamples:\n"
                  << "  " << program_name << " --config=config.yaml\n"
                  << "  " << program_name << " --set logger.level=debug\n"
                  << "  " << program_name << " --config=config.yaml --set thread.pool_size=16\n";
    }

    /**
     * @brief Print version information to stdout.
     *
     * @param version Version string
     */
    static void print_version(const std::string& version = "0.1.0.0") {
        std::cout << "unified_system version " << version << "\n";
    }

private:
    /**
     * @brief Check if a string starts with a prefix.
     */
    static bool starts_with(const std::string& str, const std::string& prefix) {
        return str.size() >= prefix.size() &&
               str.compare(0, prefix.size(), prefix) == 0;
    }

    /**
     * @brief Parse a key=value string.
     */
    static Result<std::pair<std::string, std::string>> parse_key_value(const std::string& str) {
        auto pos = str.find('=');
        if (pos == std::string::npos) {
            return make_error<std::pair<std::string, std::string>>(
                cli_error_codes::invalid_format,
                "Invalid key=value format: " + str,
                "cli_config_parser"
            );
        }

        std::string key = str.substr(0, pos);
        std::string value = str.substr(pos + 1);

        // Trim whitespace from key
        key.erase(0, key.find_first_not_of(" \t"));
        key.erase(key.find_last_not_of(" \t") + 1);

        if (key.empty()) {
            return make_error<std::pair<std::string, std::string>>(
                cli_error_codes::invalid_key,
                "Empty key in key=value pair",
                "cli_config_parser"
            );
        }

        return Result<std::pair<std::string, std::string>>::ok(
            std::make_pair(std::move(key), std::move(value)));
    }

    /**
     * @brief Apply a configuration override.
     *
     * @param config Configuration to modify
     * @param key Configuration key (dot-separated path)
     * @param value Value to set
     * @return VoidResult indicating success or error
     */
    static VoidResult apply_override(unified_config& config,
                                     const std::string& key,
                                     const std::string& value) {
        // Thread configuration
        if (key == "thread.pool_size") {
            config.thread.pool_size = parse_size_t(value);
        } else if (key == "thread.queue_type") {
            config.thread.queue_type = value;
        } else if (key == "thread.max_queue_size") {
            config.thread.max_queue_size = parse_size_t(value);
        } else if (key == "thread.thread_name_prefix") {
            config.thread.thread_name_prefix = value;
        }
        // Logger configuration
        else if (key == "logger.level") {
            config.logger.level = value;
        } else if (key == "logger.async") {
            config.logger.async = parse_bool(value);
        } else if (key == "logger.buffer_size") {
            config.logger.buffer_size = parse_size_t(value);
        } else if (key == "logger.file_path") {
            config.logger.file_path = value;
        } else if (key == "logger.max_file_size") {
            config.logger.max_file_size = parse_size_t(value);
        } else if (key == "logger.max_backup_files") {
            config.logger.max_backup_files = parse_size_t(value);
        } else if (key == "logger.format_pattern") {
            config.logger.format_pattern = value;
        }
        // Monitoring configuration
        else if (key == "monitoring.enabled") {
            config.monitoring.enabled = parse_bool(value);
        } else if (key == "monitoring.metrics_interval" ||
                   key == "monitoring.metrics_interval_ms") {
            config.monitoring.metrics_interval = std::chrono::milliseconds{parse_int64(value)};
        } else if (key == "monitoring.health_check_interval" ||
                   key == "monitoring.health_check_interval_ms") {
            config.monitoring.health_check_interval = std::chrono::milliseconds{parse_int64(value)};
        } else if (key == "monitoring.prometheus_port") {
            config.monitoring.prometheus_port = static_cast<uint16_t>(parse_size_t(value));
        } else if (key == "monitoring.prometheus_path") {
            config.monitoring.prometheus_path = value;
        }
        // Tracing configuration
        else if (key == "monitoring.tracing.enabled") {
            config.monitoring.tracing.enabled = parse_bool(value);
        } else if (key == "monitoring.tracing.sampling_rate") {
            config.monitoring.tracing.sampling_rate = parse_double(value);
        } else if (key == "monitoring.tracing.exporter") {
            config.monitoring.tracing.exporter = value;
        } else if (key == "monitoring.tracing.endpoint") {
            config.monitoring.tracing.endpoint = value;
        }
        // Database configuration
        else if (key == "database.backend") {
            config.database.backend = value;
        } else if (key == "database.connection_string") {
            config.database.connection_string = value;
        } else if (key == "database.log_queries") {
            config.database.log_queries = parse_bool(value);
        } else if (key == "database.slow_query_threshold" ||
                   key == "database.slow_query_threshold_ms") {
            config.database.slow_query_threshold = std::chrono::milliseconds{parse_int64(value)};
        } else if (key == "database.pool.min_size") {
            config.database.pool.min_size = parse_size_t(value);
        } else if (key == "database.pool.max_size") {
            config.database.pool.max_size = parse_size_t(value);
        } else if (key == "database.pool.idle_timeout" ||
                   key == "database.pool.idle_timeout_ms") {
            config.database.pool.idle_timeout = std::chrono::milliseconds{parse_int64(value)};
        } else if (key == "database.pool.acquire_timeout" ||
                   key == "database.pool.acquire_timeout_ms") {
            config.database.pool.acquire_timeout = std::chrono::milliseconds{parse_int64(value)};
        }
        // Network configuration
        else if (key == "network.compression") {
            config.network.compression = value;
        } else if (key == "network.buffer_size") {
            config.network.buffer_size = parse_size_t(value);
        } else if (key == "network.connect_timeout" ||
                   key == "network.connect_timeout_ms") {
            config.network.connect_timeout = std::chrono::milliseconds{parse_int64(value)};
        } else if (key == "network.io_timeout" ||
                   key == "network.io_timeout_ms") {
            config.network.io_timeout = std::chrono::milliseconds{parse_int64(value)};
        } else if (key == "network.keepalive_interval" ||
                   key == "network.keepalive_interval_ms") {
            config.network.keepalive_interval = std::chrono::milliseconds{parse_int64(value)};
        } else if (key == "network.max_connections") {
            config.network.max_connections = parse_size_t(value);
        }
        // TLS configuration
        else if (key == "network.tls.enabled") {
            config.network.tls.enabled = parse_bool(value);
        } else if (key == "network.tls.version") {
            config.network.tls.version = value;
        } else if (key == "network.tls.cert_path") {
            config.network.tls.cert_path = value;
        } else if (key == "network.tls.key_path") {
            config.network.tls.key_path = value;
        } else if (key == "network.tls.ca_path") {
            config.network.tls.ca_path = value;
        } else if (key == "network.tls.verify_peer") {
            config.network.tls.verify_peer = parse_bool(value);
        }
        // Unknown key
        else {
            return make_error<std::monostate>(
                cli_error_codes::invalid_key,
                "Unknown configuration key: " + key,
                "cli_config_parser"
            );
        }

        return VoidResult::ok({});
    }

    // Value parsing helpers

    static size_t parse_size_t(const std::string& value) {
        try {
            return std::stoull(value);
        } catch (...) {
            return 0;
        }
    }

    static int64_t parse_int64(const std::string& value) {
        try {
            return std::stoll(value);
        } catch (...) {
            return 0;
        }
    }

    static double parse_double(const std::string& value) {
        try {
            return std::stod(value);
        } catch (...) {
            return 0.0;
        }
    }

    static bool parse_bool(const std::string& value) {
        std::string lower = value;
        std::transform(lower.begin(), lower.end(), lower.begin(),
                      [](unsigned char c) { return std::tolower(c); });
        return lower == "true" || lower == "1" || lower == "yes" || lower == "on";
    }
};

}  // namespace kcenon::common::config
