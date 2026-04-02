// BSD 3-Clause License
// Copyright (c) 2025, 🍀☀🌕🌥 🌊
// See the LICENSE file in the project root for full license information.

#pragma once

#include <cstdint>
#include <map>
#include <string>
#include <variant>
#include <vector>

#include "../patterns/result.h"

namespace kcenon::common {

/**
 * @struct database_null
 * @brief Represents NULL values in database results
 */
struct database_null {};

/**
 * @typedef database_value
 * @brief Variant type for database column values
 *
 * Supports NULL, string, 64-bit integer, double, and boolean types.
 */
using database_value = std::variant<database_null, std::string, std::int64_t, double, bool>;

/**
 * @typedef database_row
 * @brief Map of column names to values representing a database row
 */
using database_row = std::map<std::string, database_value>;

/**
 * @typedef database_result
 * @brief Vector of rows representing a complete query result set
 */
using database_result = std::vector<database_row>;

namespace interfaces {

/**
 * @class IDatabase
 * @brief Standard interface for database operations
 *
 * This interface provides a common abstraction for database connectivity
 * and query execution, supporting transactions and connection management.
 *
 * Thread Safety:
 * - Implementation-specific; check concrete implementations
 * - Generally, each connection should be used from a single thread
 *   or protected with appropriate synchronization
 * - execute_query() and execute_command() should not be called concurrently
 *   on the same connection during a transaction
 *
 * Example:
 * @code
 * auto db = create_database();
 * auto conn_result = db->connect("host=localhost dbname=test");
 * if (conn_result.is_ok()) {
 *     auto result = db->execute_query("SELECT * FROM users");
 *     if (result.is_ok()) {
 *         for (const auto& row : result.value()) {
 *             // Process row
 *         }
 *     }
 *     db->disconnect();
 * }
 * @endcode
 */
class IDatabase {
public:
    virtual ~IDatabase() = default;

    /**
     * @brief Connect to database using connection string
     * @param connection_string Database-specific connection string
     * @return VoidResult indicating success or error
     */
    virtual VoidResult connect(const std::string& connection_string) = 0;

    /**
     * @brief Disconnect from database
     * @return VoidResult indicating success or error
     */
    virtual VoidResult disconnect() = 0;

    /**
     * @brief Execute a query and return results
     * @param query SQL query string
     * @return Result containing query results or error
     */
    virtual Result<database_result> execute_query(const std::string& query) = 0;

    /**
     * @brief Execute a command without returning results
     * @param command SQL command string (INSERT, UPDATE, DELETE, etc.)
     * @return VoidResult indicating success or error
     */
    virtual VoidResult execute_command(const std::string& command) = 0;

    /**
     * @brief Begin a database transaction
     * @return VoidResult indicating success or error
     */
    virtual VoidResult begin_transaction() = 0;

    /**
     * @brief Commit the current transaction
     * @return VoidResult indicating success or error
     */
    virtual VoidResult commit() = 0;

    /**
     * @brief Rollback the current transaction
     * @return VoidResult indicating success or error
     */
    virtual VoidResult rollback() = 0;

    /**
     * @brief Check if database is currently connected
     * @return true if connected, false otherwise
     */
    virtual bool is_connected() const = 0;
};

} // namespace interfaces
} // namespace kcenon::common
