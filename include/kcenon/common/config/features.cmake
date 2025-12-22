# Central Feature Configuration for System Modules
# This file centralizes all build options and feature flags for better maintainability
#
# This file works alongside the C++ feature detection headers:
# - feature_flags.h       : Main entry point for C++ code
# - feature_flags_core.h  : Preprocessor helpers and compiler detection
# - feature_detection.h   : C++ standard library feature detection
# - feature_system_deps.h : System module integration flags
#
# CMake options defined here are propagated to C++ code via compile definitions.
# The C++ headers then use these definitions to set KCENON_* macros.

# Thread System Integration Options
option(ENABLE_THREAD_INTEGRATION
    "Enable integration with thread_system module"
    ON)

# Logger System Integration Options
option(ENABLE_LOGGER_INTEGRATION
    "Enable integration with logger_system module"
    ON)

# Monitoring System Integration Options
option(ENABLE_MONITORING_INTEGRATION
    "Enable integration with monitoring_system module"
    ON)

# Container System Integration Options
option(ENABLE_CONTAINER_INTEGRATION
    "Enable integration with container_system module"
    ON)

# Database System Options
option(ENABLE_DATABASE_MYSQL
    "Enable MySQL database support"
    OFF)

option(ENABLE_DATABASE_POSTGRESQL
    "Enable PostgreSQL database support"
    OFF)

option(ENABLE_DATABASE_SQLITE
    "Enable SQLite database support"
    ON)

option(ENABLE_DATABASE_MONGODB
    "Enable MongoDB database support"
    OFF)

option(ENABLE_DATABASE_REDIS
    "Enable Redis database support"
    OFF)

# Network System Options
option(ENABLE_NETWORK_SSL
    "Enable SSL/TLS support in network_system"
    ON)

option(ENABLE_NETWORK_COMPRESSION
    "Enable message compression in network_system"
    ON)

# Common Options
option(ENABLE_TESTING
    "Enable unit tests for all modules"
    ON)

option(ENABLE_BENCHMARKS
    "Enable performance benchmarks"
    OFF)

option(ENABLE_EXAMPLES
    "Build example applications"
    ON)

# Build Type Options
option(ENABLE_ASAN
    "Enable Address Sanitizer"
    OFF)

option(ENABLE_TSAN
    "Enable Thread Sanitizer"
    OFF)

option(ENABLE_UBSAN
    "Enable Undefined Behavior Sanitizer"
    OFF)

# Feature Flag Mappings for backward compatibility
# These map new standardized flags to existing module-specific flags

# Thread System Mappings
if(ENABLE_THREAD_INTEGRATION)
    set(USE_THREAD_SYSTEM ON CACHE BOOL "Legacy flag for thread system" FORCE)
    set(BUILD_WITH_THREAD_SYSTEM ON CACHE BOOL "Legacy build flag for thread system" FORCE)
endif()

# Logger System Mappings
if(ENABLE_LOGGER_INTEGRATION)
    set(USE_LOGGER_SYSTEM ON CACHE BOOL "Legacy flag for logger system" FORCE)
    set(BUILD_WITH_LOGGER ON CACHE BOOL "Legacy build flag for logger" FORCE)
endif()

# Monitoring System Mappings
if(ENABLE_MONITORING_INTEGRATION)
    set(USE_MONITORING_SYSTEM ON CACHE BOOL "Legacy flag for monitoring system" FORCE)
    set(BUILD_WITH_MONITORING ON CACHE BOOL "Legacy build flag for monitoring" FORCE)
endif()

# Container System Mappings
if(ENABLE_CONTAINER_INTEGRATION)
    set(USE_CONTAINER_SYSTEM ON CACHE BOOL "Legacy flag for container system" FORCE)
    set(BUILD_WITH_CONTAINER ON CACHE BOOL "Legacy build flag for container" FORCE)
endif()

# Function to print feature configuration summary
function(print_feature_summary)
    message(STATUS "")
    message(STATUS "=== Feature Configuration Summary ===")
    message(STATUS "Thread Integration:     ${ENABLE_THREAD_INTEGRATION}")
    message(STATUS "Logger Integration:     ${ENABLE_LOGGER_INTEGRATION}")
    message(STATUS "Monitoring Integration: ${ENABLE_MONITORING_INTEGRATION}")
    message(STATUS "Container Integration:  ${ENABLE_CONTAINER_INTEGRATION}")
    message(STATUS "")
    message(STATUS "Database Backends:")
    message(STATUS "  MySQL:      ${ENABLE_DATABASE_MYSQL}")
    message(STATUS "  PostgreSQL: ${ENABLE_DATABASE_POSTGRESQL}")
    message(STATUS "  SQLite:     ${ENABLE_DATABASE_SQLITE}")
    message(STATUS "  MongoDB:    ${ENABLE_DATABASE_MONGODB}")
    message(STATUS "  Redis:      ${ENABLE_DATABASE_REDIS}")
    message(STATUS "")
    message(STATUS "Network Features:")
    message(STATUS "  SSL/TLS:     ${ENABLE_NETWORK_SSL}")
    message(STATUS "  Compression: ${ENABLE_NETWORK_COMPRESSION}")
    message(STATUS "")
    message(STATUS "Build Options:")
    message(STATUS "  Testing:    ${ENABLE_TESTING}")
    message(STATUS "  Benchmarks: ${ENABLE_BENCHMARKS}")
    message(STATUS "  Examples:   ${ENABLE_EXAMPLES}")
    message(STATUS "")
    message(STATUS "Sanitizers:")
    message(STATUS "  AddressSanitizer:   ${ENABLE_ASAN}")
    message(STATUS "  ThreadSanitizer:    ${ENABLE_TSAN}")
    message(STATUS "  UBSanitizer:        ${ENABLE_UBSAN}")
    message(STATUS "=====================================")
    message(STATUS "")
endfunction()

# Helper macro to configure module integration
macro(configure_module_integration MODULE_NAME)
    string(TOUPPER ${MODULE_NAME} MODULE_NAME_UPPER)
    if(ENABLE_${MODULE_NAME_UPPER}_INTEGRATION)
        add_compile_definitions(WITH_${MODULE_NAME_UPPER}_SYSTEM)
        message(STATUS "Enabled ${MODULE_NAME} integration")
    endif()
endmacro()

# Helper function to setup sanitizers
function(setup_sanitizers TARGET_NAME)
    if(ENABLE_ASAN)
        target_compile_options(${TARGET_NAME} PRIVATE -fsanitize=address)
        target_link_options(${TARGET_NAME} PRIVATE -fsanitize=address)
    endif()

    if(ENABLE_TSAN)
        target_compile_options(${TARGET_NAME} PRIVATE -fsanitize=thread)
        target_link_options(${TARGET_NAME} PRIVATE -fsanitize=thread)
    endif()

    if(ENABLE_UBSAN)
        target_compile_options(${TARGET_NAME} PRIVATE -fsanitize=undefined)
        target_link_options(${TARGET_NAME} PRIVATE -fsanitize=undefined)
    endif()
endfunction()

#==============================================================================
# KCENON_* Feature Flag Export Functions
#==============================================================================

# Export KCENON_* compile definitions to a target
# This function propagates all enabled integration flags as KCENON_* macros
# for use in C++ code alongside the feature detection headers.
#
# Usage:
#   export_kcenon_features(my_target)
#   export_kcenon_features(my_target PUBLIC)  # For interface libraries
#
function(export_kcenon_features TARGET_NAME)
    set(VISIBILITY PRIVATE)
    if(ARGC GREATER 1)
        set(VISIBILITY ${ARGV1})
    endif()

    # System integration flags
    if(ENABLE_THREAD_INTEGRATION)
        target_compile_definitions(${TARGET_NAME} ${VISIBILITY}
            KCENON_WITH_THREAD_SYSTEM=1
            ENABLE_THREAD_INTEGRATION=1)
    endif()

    if(ENABLE_LOGGER_INTEGRATION)
        target_compile_definitions(${TARGET_NAME} ${VISIBILITY}
            KCENON_WITH_LOGGER_SYSTEM=1
            ENABLE_LOGGER_INTEGRATION=1)
    endif()

    if(ENABLE_MONITORING_INTEGRATION)
        target_compile_definitions(${TARGET_NAME} ${VISIBILITY}
            KCENON_WITH_MONITORING_SYSTEM=1
            ENABLE_MONITORING_INTEGRATION=1)
    endif()

    if(ENABLE_CONTAINER_INTEGRATION)
        target_compile_definitions(${TARGET_NAME} ${VISIBILITY}
            KCENON_WITH_CONTAINER_SYSTEM=1
            ENABLE_CONTAINER_INTEGRATION=1)
    endif()

    # Database backend flags
    if(ENABLE_DATABASE_MYSQL)
        target_compile_definitions(${TARGET_NAME} ${VISIBILITY}
            KCENON_DATABASE_MYSQL=1
            ENABLE_DATABASE_MYSQL=1)
    endif()

    if(ENABLE_DATABASE_POSTGRESQL)
        target_compile_definitions(${TARGET_NAME} ${VISIBILITY}
            KCENON_DATABASE_POSTGRESQL=1
            ENABLE_DATABASE_POSTGRESQL=1)
    endif()

    if(ENABLE_DATABASE_SQLITE)
        target_compile_definitions(${TARGET_NAME} ${VISIBILITY}
            KCENON_DATABASE_SQLITE=1
            ENABLE_DATABASE_SQLITE=1)
    endif()

    if(ENABLE_DATABASE_MONGODB)
        target_compile_definitions(${TARGET_NAME} ${VISIBILITY}
            KCENON_DATABASE_MONGODB=1
            ENABLE_DATABASE_MONGODB=1)
    endif()

    if(ENABLE_DATABASE_REDIS)
        target_compile_definitions(${TARGET_NAME} ${VISIBILITY}
            KCENON_DATABASE_REDIS=1
            ENABLE_DATABASE_REDIS=1)
    endif()

    # Network flags
    if(ENABLE_NETWORK_SSL)
        target_compile_definitions(${TARGET_NAME} ${VISIBILITY}
            KCENON_NETWORK_SSL=1
            ENABLE_NETWORK_SSL=1)
    endif()

    if(ENABLE_NETWORK_COMPRESSION)
        target_compile_definitions(${TARGET_NAME} ${VISIBILITY}
            KCENON_NETWORK_COMPRESSION=1
            ENABLE_NETWORK_COMPRESSION=1)
    endif()

    # Build configuration flags
    if(ENABLE_TESTING)
        target_compile_definitions(${TARGET_NAME} ${VISIBILITY}
            KCENON_ENABLE_TESTING=1)
    endif()

    if(ENABLE_BENCHMARKS)
        target_compile_definitions(${TARGET_NAME} ${VISIBILITY}
            KCENON_ENABLE_BENCHMARKS=1)
    endif()

    if(ENABLE_EXAMPLES)
        target_compile_definitions(${TARGET_NAME} ${VISIBILITY}
            KCENON_ENABLE_EXAMPLES=1)
    endif()

    # Sanitizer flags
    if(ENABLE_ASAN)
        target_compile_definitions(${TARGET_NAME} ${VISIBILITY}
            KCENON_ENABLE_ASAN=1)
    endif()

    if(ENABLE_TSAN)
        target_compile_definitions(${TARGET_NAME} ${VISIBILITY}
            KCENON_ENABLE_TSAN=1)
    endif()

    if(ENABLE_UBSAN)
        target_compile_definitions(${TARGET_NAME} ${VISIBILITY}
            KCENON_ENABLE_UBSAN=1)
    endif()
endfunction()

# Convenience function to print which KCENON_* flags would be exported
function(print_kcenon_features)
    message(STATUS "")
    message(STATUS "=== KCENON Feature Flags ===")

    # System integration
    message(STATUS "System Integration:")
    if(ENABLE_THREAD_INTEGRATION)
        message(STATUS "  KCENON_WITH_THREAD_SYSTEM=1")
    endif()
    if(ENABLE_LOGGER_INTEGRATION)
        message(STATUS "  KCENON_WITH_LOGGER_SYSTEM=1")
    endif()
    if(ENABLE_MONITORING_INTEGRATION)
        message(STATUS "  KCENON_WITH_MONITORING_SYSTEM=1")
    endif()
    if(ENABLE_CONTAINER_INTEGRATION)
        message(STATUS "  KCENON_WITH_CONTAINER_SYSTEM=1")
    endif()

    # Database
    message(STATUS "Database Backends:")
    if(ENABLE_DATABASE_MYSQL)
        message(STATUS "  KCENON_DATABASE_MYSQL=1")
    endif()
    if(ENABLE_DATABASE_POSTGRESQL)
        message(STATUS "  KCENON_DATABASE_POSTGRESQL=1")
    endif()
    if(ENABLE_DATABASE_SQLITE)
        message(STATUS "  KCENON_DATABASE_SQLITE=1")
    endif()
    if(ENABLE_DATABASE_MONGODB)
        message(STATUS "  KCENON_DATABASE_MONGODB=1")
    endif()
    if(ENABLE_DATABASE_REDIS)
        message(STATUS "  KCENON_DATABASE_REDIS=1")
    endif()

    # Network
    message(STATUS "Network Features:")
    if(ENABLE_NETWORK_SSL)
        message(STATUS "  KCENON_NETWORK_SSL=1")
    endif()
    if(ENABLE_NETWORK_COMPRESSION)
        message(STATUS "  KCENON_NETWORK_COMPRESSION=1")
    endif()

    message(STATUS "============================")
    message(STATUS "")
endfunction()