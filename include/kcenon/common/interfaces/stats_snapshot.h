// BSD 3-Clause License
// Copyright (c) 2025, kcenon
// See the LICENSE file in the project root for full license information.

/**
 * @file stats_snapshot.h
 * @brief Value type for statistics snapshots.
 *
 * Provides a point-in-time snapshot of component statistics with
 * metadata (component name, timestamp) for monitoring and logging.
 *
 * @note Issue #316: Create IStats common interface for statistics
 */

#pragma once

#include <chrono>
#include <sstream>
#include <string>
#include <unordered_map>
#include <variant>
#include <iomanip>

namespace kcenon::common::interfaces {

// Forward declare to avoid circular dependency
using stats_value = std::variant<std::int64_t, double, std::string, bool>;

/**
 * @struct stats_snapshot
 * @brief Point-in-time snapshot of component statistics.
 *
 * Immutable value type that captures statistics with metadata.
 * Designed for serialization, logging, and transmission to monitoring systems.
 *
 * Usage Example:
 * @code
 * stats_snapshot snapshot{
 *     .component_name = "http_client",
 *     .timestamp = std::chrono::system_clock::now(),
 *     .values = {
 *         {"request_count", 1500},
 *         {"error_rate", 0.02},
 *         {"is_healthy", true}
 *     }
 * };
 *
 * // Serialize to JSON for logging
 * std::cout << snapshot.to_json() << std::endl;
 * @endcode
 */
struct stats_snapshot {
    std::string component_name;                             ///< Component identifier
    std::chrono::system_clock::time_point timestamp;        ///< Snapshot capture time
    std::unordered_map<std::string, stats_value> values;    ///< Metric key-value pairs

    /**
     * @brief Serialize snapshot to JSON string.
     *
     * Format:
     * {
     *   "component": "component_name",
     *   "timestamp": "2025-01-30T12:34:56Z",
     *   "metrics": {
     *     "metric1": value1,
     *     "metric2": value2
     *   }
     * }
     *
     * @return JSON string representation
     */
    [[nodiscard]] auto to_json() const -> std::string
    {
        std::ostringstream json;
        json << "{\n";
        json << "  \"component\": \"" << component_name << "\",\n";

        // Format timestamp as ISO 8601
        const auto time_t = std::chrono::system_clock::to_time_t(timestamp);
        std::tm tm{};
#ifdef _WIN32
        localtime_s(&tm, &time_t);
#else
        localtime_r(&time_t, &tm);
#endif
        json << "  \"timestamp\": \""
             << std::put_time(&tm, "%Y-%m-%dT%H:%M:%S")
             << "Z\",\n";

        // Serialize metrics
        json << "  \"metrics\": {\n";

        bool first = true;
        for (const auto& [key, value] : values) {
            if (!first) {
                json << ",\n";
            }
            first = false;

            json << "    \"" << key << "\": ";

            // Visit variant and format value
            std::visit([&json](const auto& v) {
                using T = std::decay_t<decltype(v)>;
                if constexpr (std::is_same_v<T, std::int64_t>) {
                    json << v;
                } else if constexpr (std::is_same_v<T, double>) {
                    json << std::fixed << std::setprecision(6) << v;
                } else if constexpr (std::is_same_v<T, std::string>) {
                    json << "\"" << v << "\"";
                } else if constexpr (std::is_same_v<T, bool>) {
                    json << (v ? "true" : "false");
                }
            }, value);
        }

        json << "\n  }\n";
        json << "}";

        return json.str();
    }
};

} // namespace kcenon::common::interfaces
