// BSD 3-Clause License
// Copyright (c) 2025, kcenon
// See the LICENSE file in the project root for full license information.

/**
 * @file enum_serialization.h
 * @brief Generic enum serialization utilities using C++20 concepts.
 *
 * This header provides a type-safe, compile-time approach to enum
 * serialization and deserialization. It eliminates duplicated switch/case
 * patterns across multiple enum types.
 *
 * Usage:
 * @code
 * // 1. Define enum_traits specialization for your enum
 * template<>
 * struct enum_traits<my_enum> {
 *     static constexpr auto values = std::array{
 *         std::pair{my_enum::value1, std::string_view{"VALUE1"}},
 *         std::pair{my_enum::value2, std::string_view{"VALUE2"}},
 *     };
 *     static constexpr std::string_view module_name = "my_module";
 * };
 *
 * // 2. Use generic functions
 * auto str = enum_to_string(my_enum::value1);  // "VALUE1"
 * auto result = enum_from_string<my_enum>("VALUE1");  // Result<my_enum>
 * @endcode
 *
 * @note Issue #293: Created to eliminate duplicated enum serialization patterns.
 */

#pragma once

#include <algorithm>
#include <array>
#include <cctype>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

#include "../patterns/result.h"

namespace kcenon::common {

/**
 * @brief Primary template for enum traits (must be specialized for each enum)
 *
 * Specializations must provide:
 * - static constexpr auto values: array of {enum_value, string_view} pairs
 * - static constexpr std::string_view module_name: module name for error reporting
 */
template<typename Enum>
struct enum_traits;

/**
 * @brief Concept to check if an enum has valid traits defined
 */
template<typename Enum>
concept EnumSerializable = std::is_enum_v<Enum> && requires {
    { enum_traits<Enum>::values } -> std::convertible_to<
        std::span<const std::pair<Enum, std::string_view>>>;
    { enum_traits<Enum>::module_name } -> std::convertible_to<std::string_view>;
};

/**
 * @brief Convert an enum value to its string representation
 *
 * @tparam Enum The enum type (must satisfy EnumSerializable concept)
 * @param value The enum value to convert
 * @return String representation, or "UNKNOWN" if not found
 */
template<EnumSerializable Enum>
[[nodiscard]] inline std::string enum_to_string(Enum value) {
    for (const auto& [e, s] : enum_traits<Enum>::values) {
        if (e == value) {
            return std::string(s);
        }
    }
    return "UNKNOWN";
}

/**
 * @brief Convert a string to its enum value (case-insensitive)
 *
 * @tparam Enum The enum type (must satisfy EnumSerializable concept)
 * @param str The string to convert
 * @return Result containing the enum value or error
 */
template<EnumSerializable Enum>
[[nodiscard]] inline Result<Enum> enum_from_string(std::string_view str) {
    // Convert input to uppercase for case-insensitive comparison
    std::string upper(str.size(), '\0');
    std::transform(str.begin(), str.end(), upper.begin(),
                   [](unsigned char c) { return static_cast<char>(std::toupper(c)); });

    for (const auto& [e, s] : enum_traits<Enum>::values) {
        if (s == upper) {
            return Result<Enum>::ok(e);
        }
    }

    return Result<Enum>::err(
        error_info{1, "Invalid enum value: " + std::string(str),
                   std::string(enum_traits<Enum>::module_name)});
}

}  // namespace kcenon::common
