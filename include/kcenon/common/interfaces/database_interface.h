/*
 * BSD 3-Clause License
 * Copyright (c) 2025, kcenon
 */

#pragma once

#include <cstdint>
#include <map>
#include <string>
#include <variant>
#include <vector>

#include "../patterns/result.h"

namespace common {

struct database_null {};

using database_value = std::variant<database_null, std::string, std::int64_t, double, bool>;
using database_row = std::map<std::string, database_value>;
using database_result = std::vector<database_row>;

namespace interfaces {

class IDatabase {
public:
    virtual ~IDatabase() = default;

    virtual VoidResult connect(const std::string& connection_string) = 0;
    virtual VoidResult disconnect() = 0;
    virtual Result<database_result> execute_query(const std::string& query) = 0;
    virtual VoidResult execute_command(const std::string& command) = 0;
    virtual VoidResult begin_transaction() = 0;
    virtual VoidResult commit() = 0;
    virtual VoidResult rollback() = 0;
    virtual bool is_connected() const = 0;
};

} // namespace interfaces
} // namespace common

