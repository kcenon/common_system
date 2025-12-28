// BSD 3-Clause License
// Copyright (c) 2025, kcenon
// See the LICENSE file in the project root for full license information.

/**
 * @file logger.h
 * @brief C++20 concepts for logger interfaces.
 *
 * This header provides concepts for validating logger types used in
 * logging operations. These concepts replace abstract class-based constraints
 * with compile-time validation and clearer error messages.
 *
 * Requirements:
 * - C++20 compiler with concepts support
 * - GCC 10+, Clang 10+, MSVC 2022+
 *
 * Thread Safety:
 * - Concepts are evaluated at compile-time only.
 * - No runtime thread-safety considerations apply.
 *
 * @see logger_interface.h for ILogger definition
 * @see https://en.cppreference.com/w/cpp/language/constraints
 */

#pragma once

#include <concepts>
#include <string>
#include <string_view>
#include <type_traits>

namespace kcenon::common {

// Forward declarations
namespace interfaces {
enum class log_level;
struct log_entry;
}  // namespace interfaces

namespace concepts {

/**
 * @concept LogLevelEnumLike
 * @brief A type that represents log levels.
 *
 * Types satisfying this concept are enum types that can be used
 * to specify logging severity levels.
 *
 * Example usage:
 * @code
 * template<LogLevelEnumLike L>
 * void set_level(L level) {
 *     // Configure logging level
 * }
 * @endcode
 */
template <typename T>
concept LogLevelEnumLike = std::is_enum_v<T>;

/**
 * @concept BasicLogger
 * @brief A type that provides basic logging functionality.
 *
 * Types satisfying this concept can log messages with a specified level.
 * This is the minimum requirement for a logging implementation.
 *
 * Example usage:
 * @code
 * template<BasicLogger L>
 * void log_info(L& logger, const std::string& message) {
 *     logger.log(interfaces::log_level::info, message);
 * }
 * @endcode
 */
template <typename T>
concept BasicLogger = requires(T t,
                               interfaces::log_level level,
                               const std::string& message) {
    { t.log(level, message) };
};

/**
 * @concept LevelAwareLogger
 * @brief A type that supports log level filtering.
 *
 * Types satisfying this concept can check if a log level is enabled
 * and can set/get the minimum log level. This enables efficient
 * logging by avoiding unnecessary message formatting.
 *
 * Example usage:
 * @code
 * template<LevelAwareLogger L>
 * void log_debug(L& logger, const std::string& message) {
 *     if (logger.is_enabled(interfaces::log_level::debug)) {
 *         logger.log(interfaces::log_level::debug, message);
 *     }
 * }
 * @endcode
 */
template <typename T>
concept LevelAwareLogger = BasicLogger<T> &&
    requires(T t, const T ct, interfaces::log_level level) {
        { ct.is_enabled(level) } -> std::convertible_to<bool>;
        { t.set_level(level) };
        { ct.get_level() } -> std::convertible_to<interfaces::log_level>;
    };

/**
 * @concept FlushableLogger
 * @brief A type that supports flushing buffered log messages.
 *
 * Types satisfying this concept can flush any internally buffered
 * log messages to their destination (file, console, etc.).
 *
 * Example usage:
 * @code
 * template<FlushableLogger L>
 * void ensure_logged(L& logger) {
 *     logger.flush();
 * }
 * @endcode
 */
template <typename T>
concept FlushableLogger = BasicLogger<T> && requires(T t) {
    { t.flush() };
};

/**
 * @concept StructuredLogger
 * @brief A type that supports structured log entries.
 *
 * Types satisfying this concept can log structured entries that
 * contain additional metadata beyond just level and message.
 *
 * Example usage:
 * @code
 * template<StructuredLogger L>
 * void log_entry(L& logger, const interfaces::log_entry& entry) {
 *     logger.log(entry);
 * }
 * @endcode
 */
template <typename T>
concept StructuredLogger = BasicLogger<T> &&
    requires(T t, const interfaces::log_entry& entry) {
        { t.log(entry) };
    };

/**
 * @concept LoggerLike
 * @brief A complete logger type satisfying ILogger interface requirements.
 *
 * Types satisfying this concept provide full logging functionality
 * including level awareness, flushing, and structured logging.
 * This concept matches the ILogger interface contract.
 *
 * Example usage:
 * @code
 * template<LoggerLike L>
 * void setup_logging(std::shared_ptr<L> logger) {
 *     logger->set_level(interfaces::log_level::debug);
 *     logger->log(interfaces::log_level::info, "Logging initialized");
 *     logger->flush();
 * }
 * @endcode
 */
template <typename T>
concept LoggerLike = LevelAwareLogger<T> &&
                     FlushableLogger<T> &&
                     StructuredLogger<T>;

/**
 * @concept LoggerProviderLike
 * @brief A type that can provide logger instances.
 *
 * Types satisfying this concept can create and retrieve logger instances,
 * enabling dependency injection for logging.
 *
 * Example usage:
 * @code
 * template<LoggerProviderLike P>
 * auto get_logger(P& provider, const std::string& name) {
 *     return provider.create_logger(name);
 * }
 * @endcode
 */
template <typename T>
concept LoggerProviderLike = requires(T t, const std::string& name) {
    { t.get_logger() };
    { t.create_logger(name) };
};

/**
 * @concept LoggerRegistryLike
 * @brief A type that manages named logger instances.
 *
 * Types satisfying this concept can register, retrieve, and manage
 * named logger instances in a registry pattern.
 *
 * Example usage:
 * @code
 * template<LoggerRegistryLike R, LoggerLike L>
 * void register_logger(R& registry, const std::string& name,
 *                      std::shared_ptr<L> logger) {
 *     registry.register_logger(name, logger);
 * }
 * @endcode
 */
template <typename T>
concept LoggerRegistryLike = requires(T t, const std::string& name) {
    { t.register_logger(name, std::declval<std::shared_ptr<void>>()) };
    { t.get_logger(name) };
    { t.unregister_logger(name) };
    { t.get_default_logger() };
    { t.set_default_logger(std::declval<std::shared_ptr<void>>()) };
};

}  // namespace concepts
}  // namespace kcenon::common
