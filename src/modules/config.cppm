// BSD 3-Clause License
// Copyright (c) 2025, kcenon
// See the LICENSE file in the project root for full license information.

/**
 * @file config.cppm
 * @brief C++20 module partition for configuration.
 *
 * This module partition exports configuration-related types:
 * - Feature flags and detection
 * - Configuration loader utilities
 * - CLI config parser
 *
 * Part of the kcenon.common module.
 */

module;

#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <variant>
#include <vector>

export module kcenon.common:config;

export namespace kcenon::common::config {

// ============================================================================
// Feature Flags
// ============================================================================

/**
 * @brief Compile-time feature detection flags.
 */
struct feature_flags {
    static constexpr bool has_source_location =
#if __has_include(<source_location>)
        true;
#else
        false;
#endif

    static constexpr bool has_concepts = true;  // Required for C++20 modules
    static constexpr bool has_ranges =
#if __has_include(<ranges>)
        true;
#else
        false;
#endif
};

// ============================================================================
// Configuration Types
// ============================================================================

/**
 * @brief Configuration value type.
 */
using config_value = std::variant<
    std::string,
    int64_t,
    double,
    bool,
    std::vector<std::string>,
    std::map<std::string, std::string>
>;

/**
 * @struct config_entry
 * @brief A single configuration entry.
 */
struct config_entry {
    std::string key;
    config_value value;
    std::optional<std::string> description;
    bool required = false;

    config_entry() = default;
    config_entry(std::string k, config_value v, std::string desc = "", bool req = false)
        : key(std::move(k)), value(std::move(v)), description(desc), required(req) {}
};

/**
 * @class IConfigLoader
 * @brief Interface for configuration loaders.
 */
class IConfigLoader {
public:
    virtual ~IConfigLoader() = default;

    /**
     * @brief Load configuration from source.
     * @return true if successful, false otherwise
     */
    virtual bool load() = 0;

    /**
     * @brief Get a configuration value.
     * @param key Configuration key
     * @return Configuration value or nullopt if not found
     */
    virtual std::optional<config_value> get(const std::string& key) const = 0;

    /**
     * @brief Check if a key exists.
     */
    virtual bool has(const std::string& key) const = 0;
};

// ============================================================================
// CLI Config Parser
// ============================================================================

/**
 * @struct cli_option
 * @brief Definition for a CLI option.
 */
struct cli_option {
    std::string name;
    std::string short_name;
    std::string description;
    bool required = false;
    bool takes_value = true;
    std::optional<std::string> default_value;
};

/**
 * @struct cli_parse_result
 * @brief Result of CLI parsing.
 */
struct cli_parse_result {
    std::map<std::string, std::string> options;
    std::vector<std::string> positional_args;
    std::vector<std::string> errors;

    bool has_errors() const { return !errors.empty(); }
    bool has_option(const std::string& name) const {
        return options.find(name) != options.end();
    }
    std::optional<std::string> get_option(const std::string& name) const {
        auto it = options.find(name);
        if (it != options.end()) return it->second;
        return std::nullopt;
    }
};

/**
 * @class CliConfigParser
 * @brief Simple command-line argument parser.
 */
class CliConfigParser {
public:
    CliConfigParser() = default;

    /**
     * @brief Add an option definition.
     */
    void add_option(const cli_option& opt) {
        options_.push_back(opt);
    }

    /**
     * @brief Parse command-line arguments.
     */
    cli_parse_result parse(int argc, const char* const* argv) const {
        cli_parse_result result;

        for (int i = 1; i < argc; ++i) {
            std::string arg = argv[i];

            if (arg.starts_with("--")) {
                std::string name = arg.substr(2);
                auto eq_pos = name.find('=');
                if (eq_pos != std::string::npos) {
                    result.options[name.substr(0, eq_pos)] = name.substr(eq_pos + 1);
                } else if (i + 1 < argc && argv[i + 1][0] != '-') {
                    result.options[name] = argv[++i];
                } else {
                    result.options[name] = "true";
                }
            } else if (arg.starts_with("-") && arg.size() == 2) {
                std::string short_name = arg.substr(1);
                for (const auto& opt : options_) {
                    if (opt.short_name == short_name) {
                        if (opt.takes_value && i + 1 < argc) {
                            result.options[opt.name] = argv[++i];
                        } else {
                            result.options[opt.name] = "true";
                        }
                        break;
                    }
                }
            } else {
                result.positional_args.push_back(arg);
            }
        }

        // Check required options
        for (const auto& opt : options_) {
            if (opt.required && !result.has_option(opt.name)) {
                result.errors.push_back("Missing required option: " + opt.name);
            }
        }

        return result;
    }

private:
    std::vector<cli_option> options_;
};

} // namespace kcenon::common::config
