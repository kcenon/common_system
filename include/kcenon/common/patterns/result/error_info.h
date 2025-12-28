// BSD 3-Clause License
// Copyright (c) 2025, kcenon
// See the LICENSE file in the project root for full license information.

/**
 * @file error_info.h
 * @brief Error information struct for Result<T> type.
 *
 * This header defines the error_info struct used by Result<T> to represent
 * error states with code, message, module, and optional details.
 */

#pragma once

#include <string>
#include <optional>
#include <type_traits>
#include <utility>

namespace kcenon::common {

/**
 * @struct error_info
 * @brief Standard error information used by Result<T>.
 */
struct error_info {
    int code;
    std::string message;
    std::string module;
    std::optional<std::string> details;

    error_info() : code(0) {}

    /** @brief Construct with message only. */
    error_info(const std::string& msg)
        : code(-1), message(msg), module("") {}

    /** @brief Construct with code, message and optional module. */
    error_info(int c, const std::string& msg, const std::string& mod = "")
        : code(c), message(msg), module(mod) {}

    /** @brief Construct with code, message, module and details. */
    error_info(int c, const std::string& msg, const std::string& mod,
               const std::string& det)
        : code(c), message(msg), module(mod), details(det) {}

    /**
     * @brief Construct from strongly-typed enum error codes.
     *
     * Enables database_system/network_system enums to be passed directly
     * without manual static_cast noise.
     */
    template<typename Enum,
             typename std::enable_if_t<std::is_enum_v<Enum>, int> = 0>
    error_info(Enum c, std::string msg, std::string mod = "",
               std::optional<std::string> det = std::nullopt)
        : code(static_cast<int>(c)),
          message(std::move(msg)),
          module(std::move(mod)),
          details(std::move(det)) {}

    bool operator==(const error_info& other) const {
        return code == other.code && message == other.message &&
               module == other.module && details == other.details;
    }

    bool operator!=(const error_info& other) const {
        return !(*this == other);
    }
};

/**
 * @brief Alias for backward compatibility
 *
 * Some code may use error_code instead of error_info.
 * This alias ensures compatibility.
 */
using error_code = error_info;

} // namespace kcenon::common
