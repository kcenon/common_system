// BSD 3-Clause License
// Copyright (c) 2025, kcenon
// See the LICENSE file in the project root for full license information.

/**
 * @file logging.cppm
 * @brief C++20 module partition for logging utilities.
 *
 * This module partition exports logging utility functions and macros.
 *
 * Part of the kcenon.common module.
 */

module;

#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <string_view>

// Feature detection for source_location
#if __has_include(<source_location>)
    #include <source_location>
    #define KCENON_MODULE_HAS_SOURCE_LOCATION 1
#else
    #define KCENON_MODULE_HAS_SOURCE_LOCATION 0
#endif

export module kcenon.common:logging;

import :interfaces.core;

export namespace kcenon::common::logging {

// ============================================================================
// Convenience Log Functions
// ============================================================================

/**
 * @brief Log a trace message.
 */
inline void log_trace(std::shared_ptr<interfaces::ILogger> logger, std::string_view message) {
    if (logger && logger->is_enabled(interfaces::log_level::trace)) {
        logger->log(interfaces::log_level::trace, message);
    }
}

/**
 * @brief Log a debug message.
 */
inline void log_debug(std::shared_ptr<interfaces::ILogger> logger, std::string_view message) {
    if (logger && logger->is_enabled(interfaces::log_level::debug)) {
        logger->log(interfaces::log_level::debug, message);
    }
}

/**
 * @brief Log an info message.
 */
inline void log_info(std::shared_ptr<interfaces::ILogger> logger, std::string_view message) {
    if (logger && logger->is_enabled(interfaces::log_level::info)) {
        logger->log(interfaces::log_level::info, message);
    }
}

/**
 * @brief Log a warning message.
 */
inline void log_warning(std::shared_ptr<interfaces::ILogger> logger, std::string_view message) {
    if (logger && logger->is_enabled(interfaces::log_level::warning)) {
        logger->log(interfaces::log_level::warning, message);
    }
}

/**
 * @brief Log an error message.
 */
inline void log_error(std::shared_ptr<interfaces::ILogger> logger, std::string_view message) {
    if (logger && logger->is_enabled(interfaces::log_level::error)) {
        logger->log(interfaces::log_level::error, message);
    }
}

/**
 * @brief Log a critical message.
 */
inline void log_critical(std::shared_ptr<interfaces::ILogger> logger, std::string_view message) {
    if (logger && logger->is_enabled(interfaces::log_level::critical)) {
        logger->log(interfaces::log_level::critical, message);
    }
}

// ============================================================================
// Console Logger (Simple Implementation)
// ============================================================================

/**
 * @class ConsoleLogger
 * @brief Simple console logger implementation.
 */
class ConsoleLogger : public interfaces::ILogger {
public:
    ConsoleLogger(interfaces::log_level level = interfaces::log_level::info)
        : level_(level) {}

    VoidResult log(interfaces::log_level level, const std::string& message) override {
        if (!is_enabled(level)) {
            return ok();
        }
        std::cout << "[" << interfaces::to_string(level) << "] " << message << std::endl;
        return ok();
    }

    VoidResult log(const interfaces::log_entry& entry) override {
        if (!is_enabled(entry.level)) {
            return ok();
        }
        std::cout << "[" << interfaces::to_string(entry.level) << "] "
                  << entry.message;
        if (!entry.file.empty()) {
            std::cout << " (" << entry.file << ":" << entry.line << ")";
        }
        std::cout << std::endl;
        return ok();
    }

    bool is_enabled(interfaces::log_level level) const override {
        return static_cast<int>(level) >= static_cast<int>(level_);
    }

    VoidResult set_level(interfaces::log_level level) override {
        level_ = level;
        return ok();
    }

    interfaces::log_level get_level() const override {
        return level_;
    }

    VoidResult flush() override {
        std::cout.flush();
        return ok();
    }

private:
    interfaces::log_level level_;
};

/**
 * @brief Create a console logger.
 */
inline std::shared_ptr<interfaces::ILogger> make_console_logger(
    interfaces::log_level level = interfaces::log_level::info) {
    return std::make_shared<ConsoleLogger>(level);
}

} // namespace kcenon::common::logging
