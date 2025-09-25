# Central Feature Configuration for System Modules
# This file centralizes all build options and feature flags for better maintainability

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