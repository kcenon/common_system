// BSD 3-Clause License
// Copyright (c) 2025, kcenon
// See the LICENSE file in the project root for full license information.

/**
 * @file error_codes_compat.h
 * @brief Backward compatibility aliases for error codes.
 *
 * This header provides uppercase aliases for error codes to maintain
 * backward compatibility with existing code that uses the old naming
 * convention (e.g., INVALID_ARGUMENT instead of invalid_argument).
 */

#pragma once

// Import error codes from centralized location
#include <kcenon/common/error/error_codes.h>

namespace kcenon::common {

// Import common error codes from centralized error_codes.h
// Provide backward compatibility aliases for existing code
namespace error_codes {
    using namespace error::codes::common_errors;

    // Uppercase aliases for backward compatibility
    constexpr int SUCCESS = error::codes::common_errors::success;
    constexpr int INVALID_ARGUMENT = error::codes::common_errors::invalid_argument;
    constexpr int NOT_FOUND = error::codes::common_errors::not_found;
    constexpr int PERMISSION_DENIED = error::codes::common_errors::permission_denied;
    constexpr int TIMEOUT = error::codes::common_errors::timeout;
    constexpr int CANCELLED = error::codes::common_errors::cancelled;
    constexpr int NOT_INITIALIZED = error::codes::common_errors::not_initialized;
    constexpr int ALREADY_EXISTS = error::codes::common_errors::already_exists;
    constexpr int OUT_OF_MEMORY = error::codes::common_errors::out_of_memory;
    constexpr int IO_ERROR = error::codes::common_errors::io_error;
    constexpr int NETWORK_ERROR = error::codes::common_errors::network_error;
    constexpr int REGISTRY_FROZEN = error::codes::common_errors::registry_frozen;
    constexpr int INTERNAL_ERROR = error::codes::common_errors::internal_error;

    // Module-specific base ranges
    constexpr int THREAD_ERROR_BASE = static_cast<int>(error::category::thread_system);
    constexpr int LOGGER_ERROR_BASE = static_cast<int>(error::category::logger_system);
    constexpr int MONITORING_ERROR_BASE = static_cast<int>(error::category::monitoring_system);
    constexpr int CONTAINER_ERROR_BASE = static_cast<int>(error::category::container_system);
    constexpr int DATABASE_ERROR_BASE = static_cast<int>(error::category::database_system);
    constexpr int NETWORK_ERROR_BASE = static_cast<int>(error::category::network_system);
}

} // namespace kcenon::common
