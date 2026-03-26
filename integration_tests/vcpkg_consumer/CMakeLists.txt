cmake_minimum_required(VERSION 3.20)
project(vcpkg_chain_consumer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Track which ports are found
set(FOUND_PORTS "")
set(MISSING_PORTS "")

# ─── Tier 0: Foundation ──────────────────────────────────────────────────────

find_package(common_system CONFIG REQUIRED)
message(STATUS "Found common_system: ${common_system_DIR}")
list(APPEND FOUND_PORTS "common_system")

# ─── Tier 1: Threading & Containers ──────────────────────────────────────────

find_package(thread_system CONFIG)
if(thread_system_FOUND)
    message(STATUS "Found thread_system: ${thread_system_DIR}")
    list(APPEND FOUND_PORTS "thread_system")
else()
    message(WARNING "thread_system not found - skipping")
    list(APPEND MISSING_PORTS "thread_system")
endif()

find_package(container_system CONFIG)
if(container_system_FOUND)
    message(STATUS "Found container_system: ${container_system_DIR}")
    list(APPEND FOUND_PORTS "container_system")
else()
    message(WARNING "container_system not found - skipping")
    list(APPEND MISSING_PORTS "container_system")
endif()

# ─── Tier 2: Logging ─────────────────────────────────────────────────────────

find_package(logger_system CONFIG)
if(logger_system_FOUND)
    message(STATUS "Found logger_system: ${logger_system_DIR}")
    list(APPEND FOUND_PORTS "logger_system")
else()
    message(WARNING "logger_system not found - skipping")
    list(APPEND MISSING_PORTS "logger_system")
endif()

# ─── Tier 3: Monitoring & Database ───────────────────────────────────────────

find_package(monitoring_system CONFIG)
if(monitoring_system_FOUND)
    message(STATUS "Found monitoring_system: ${monitoring_system_DIR}")
    list(APPEND FOUND_PORTS "monitoring_system")
else()
    message(WARNING "monitoring_system not found - skipping")
    list(APPEND MISSING_PORTS "monitoring_system")
endif()

find_package(database_system CONFIG)
if(database_system_FOUND)
    message(STATUS "Found database_system: ${database_system_DIR}")
    list(APPEND FOUND_PORTS "database_system")
else()
    message(WARNING "database_system not found - skipping")
    list(APPEND MISSING_PORTS "database_system")
endif()

# ─── Tier 4: Networking ──────────────────────────────────────────────────────

# OpenSSL is a transitive dependency of network_system
find_package(OpenSSL QUIET)

find_package(network_system CONFIG)
if(network_system_FOUND)
    message(STATUS "Found network_system: ${network_system_DIR}")
    list(APPEND FOUND_PORTS "network_system")
else()
    message(WARNING "network_system not found - skipping")
    list(APPEND MISSING_PORTS "network_system")
endif()

# ─── Tier 5: PACS ────────────────────────────────────────────────────────────

find_package(pacs_system CONFIG)
if(pacs_system_FOUND)
    message(STATUS "Found pacs_system: ${pacs_system_DIR}")
    list(APPEND FOUND_PORTS "pacs_system")
else()
    message(WARNING "pacs_system not found - skipping")
    list(APPEND MISSING_PORTS "pacs_system")
endif()

# ─── Test Consumer Binary ────────────────────────────────────────────────────

add_executable(vcpkg_chain_consumer main.cpp)

# Always link common_system (required)
target_link_libraries(vcpkg_chain_consumer PRIVATE kcenon::common_system)
target_compile_definitions(vcpkg_chain_consumer PRIVATE HAS_COMMON_SYSTEM)

# Conditionally link available ports (using actual vcpkg exported target names)
if(TARGET thread_system::ThreadSystem)
    target_link_libraries(vcpkg_chain_consumer PRIVATE thread_system::ThreadSystem)
    target_compile_definitions(vcpkg_chain_consumer PRIVATE HAS_THREAD_SYSTEM)
endif()

if(TARGET ContainerSystem::container)
    target_link_libraries(vcpkg_chain_consumer PRIVATE ContainerSystem::container)
    target_compile_definitions(vcpkg_chain_consumer PRIVATE HAS_CONTAINER_SYSTEM)
endif()

if(TARGET LoggerSystem::LoggerSystem)
    target_link_libraries(vcpkg_chain_consumer PRIVATE LoggerSystem::LoggerSystem)
    target_compile_definitions(vcpkg_chain_consumer PRIVATE HAS_LOGGER_SYSTEM)
endif()

if(TARGET monitoring_system::monitoring_system)
    target_link_libraries(vcpkg_chain_consumer PRIVATE monitoring_system::monitoring_system)
    target_compile_definitions(vcpkg_chain_consumer PRIVATE HAS_MONITORING_SYSTEM)
endif()

if(TARGET DatabaseSystem::database)
    target_link_libraries(vcpkg_chain_consumer PRIVATE DatabaseSystem::database)
    target_compile_definitions(vcpkg_chain_consumer PRIVATE HAS_DATABASE_SYSTEM)
endif()

if(TARGET NetworkSystem::NetworkSystem)
    target_link_libraries(vcpkg_chain_consumer PRIVATE NetworkSystem::NetworkSystem)
    target_compile_definitions(vcpkg_chain_consumer PRIVATE HAS_NETWORK_SYSTEM)
endif()

if(TARGET kcenon::pacs::core)
    target_link_libraries(vcpkg_chain_consumer PRIVATE kcenon::pacs::core)
    target_compile_definitions(vcpkg_chain_consumer PRIVATE HAS_PACS_SYSTEM)
endif()

# ─── Summary ─────────────────────────────────────────────────────────────────

list(LENGTH FOUND_PORTS _found_count)
list(LENGTH MISSING_PORTS _missing_count)

message(STATUS "")
message(STATUS "vcpkg chain consumer configuration:")
message(STATUS "  Found:   ${_found_count} ports (${FOUND_PORTS})")
if(_missing_count GREATER 0)
    message(STATUS "  Missing: ${_missing_count} ports (${MISSING_PORTS})")
endif()
message(STATUS "")
