// BSD 3-Clause License
// Copyright (c) 2025, kcenon
// See the LICENSE file in the project root for full license information.

/**
 * @file source_location.h
 * @brief C++17-compatible source_location implementation
 *
 * Provides a source_location type that works with both C++17 and C++20.
 * Uses std::source_location when available (C++20), otherwise falls back
 * to compiler builtins for C++17 compatibility.
 */

#pragma once

#if __cplusplus >= 202002L && __has_include(<source_location>)
    #include <source_location>
    #if defined(__cpp_lib_source_location) && __cpp_lib_source_location >= 201907L
        #define KCENON_HAS_STD_SOURCE_LOCATION 1
    #endif
#endif

#ifndef KCENON_HAS_STD_SOURCE_LOCATION
    #define KCENON_HAS_STD_SOURCE_LOCATION 0
#endif

namespace kcenon::common {

#if KCENON_HAS_STD_SOURCE_LOCATION
    // Use std::source_location when available (C++20)
    using source_location = std::source_location;
    #define COMMON_HAS_SOURCE_LOCATION 1
#else
    // Custom implementation for C++17 using compiler builtins
    struct source_location {
    public:
        // Prevent accidental copying of the default argument
        constexpr source_location(
            const char* file = __builtin_FILE(),
            const char* function = __builtin_FUNCTION(),
            int line = __builtin_LINE()
        ) noexcept
            : file_(file), function_(function), line_(line), column_(0) {}

        constexpr const char* file_name() const noexcept { return file_; }
        constexpr const char* function_name() const noexcept { return function_; }
        constexpr int line() const noexcept { return line_; }
        constexpr int column() const noexcept { return column_; }

        // Factory method to create a source_location at the call site
        static constexpr source_location current(
            const char* file = __builtin_FILE(),
            const char* function = __builtin_FUNCTION(),
            int line = __builtin_LINE()
        ) noexcept {
            return source_location(file, function, line);
        }

    private:
        const char* file_;
        const char* function_;
        int line_;
        int column_;
    };
    #define COMMON_HAS_SOURCE_LOCATION 1
#endif

} // namespace kcenon::common
