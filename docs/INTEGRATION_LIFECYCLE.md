---
doc_id: "COM-INTR-002c"
doc_title: "Cross-System Integration Guide - Lifecycle"
doc_version: "1.0.0"
doc_date: "2026-04-04"
doc_status: "Released"
project: "common_system"
category: "INTR"
---

# Cross-System Integration Guide - Lifecycle

> **Parent document**: [Cross-System Integration Guide](INTEGRATION_GUIDE.md)

This document covers initialization, shutdown, timeout/error recovery, and a complete example application for the kcenon ecosystem.

---

## 6. Initialization and Shutdown

### Initialization Sequence

Proper initialization order is critical for multi-system applications.

#### Phase-Based Initialization

```cpp
#include <kcenon/common/di/unified_bootstrapper.h>

int main() {
    using namespace kcenon;

    // Phase 1: Configuration
    auto config_result = common::config::read_yaml("app.yaml");
    if (config_result.is_err()) {
        std::cerr << "Config load failed: "
                  << config_result.error().message << "\n";
        return 1;
    }
    auto cfg = config_result.value();

    // Phase 2: Bootstrap Systems (automatic ordering)
    common::di::bootstrapper_options opts;
    opts.enable_logging = cfg.get_bool("logging.enabled", true);
    opts.enable_monitoring = cfg.get_bool("monitoring.enabled", true);
    opts.enable_database = cfg.get_bool("database.enabled", true);
    opts.enable_network = cfg.get_bool("network.enabled", true);
    opts.config_path = "app.yaml";

    auto init_result = common::di::unified_bootstrapper::initialize(opts);
    if (init_result.is_err()) {
        std::cerr << "System initialization failed at: "
                  << init_result.error().source << "\n"
                  << "Error: " << init_result.error().message << "\n";
        return 1;
    }

    // Phase 3: Resolve Services
    auto& services = common::di::unified_bootstrapper::services();
    auto logger = services.resolve<common::interfaces::ILogger>();
    auto executor = services.resolve<common::interfaces::IExecutor>();

    logger->log(common::log_level::info, "All systems initialized successfully");

    // Phase 4: Application Logic
    run_application(services);

    // Phase 5: Graceful Shutdown
    auto shutdown_result = common::di::unified_bootstrapper::shutdown(
        std::chrono::seconds(30)
    );

    if (shutdown_result.is_err()) {
        std::cerr << "Shutdown failed: "
                  << shutdown_result.error().message << "\n";
        return 1;
    }

    return 0;
}
```

#### Manual Initialization with Tier Ordering

For fine-grained control:

```cpp
auto initialize_systems() -> common::VoidResult {
    // Tier 0: Foundation (header-only, no init)
    // (common_system is always available)

    // Tier 1: Core Systems (parallel initialization)
    auto thread_result = thread::thread_pool::initialize(8);
    auto container_result = container::serializer::initialize();

    if (thread_result.is_err()) {
        return thread_result;
    }
    if (container_result.is_err()) {
        return container_result;
    }

    // Tier 2: Service Systems (require thread_system)
    auto logger_result = logger::logger::initialize(
        thread_result.value()  // Pass thread pool
    );

    auto db_result = database::database::initialize(
        thread_result.value(),
        "db_config.yaml"
    );

    if (logger_result.is_err()) {
        return logger_result;
    }
    if (db_result.is_err()) {
        return db_result;
    }

    // Tier 3: Integration Systems
    auto network_result = network::server::initialize(
        thread_result.value(),  // Executor
        logger_result.value()   // Logger
    );

    return network_result;
}
```

### Shutdown Sequence

**Golden Rule**: Shutdown in **reverse** initialization order.

#### Automatic Shutdown

```cpp
// unified_bootstrapper handles shutdown order automatically
auto shutdown_result = common::di::unified_bootstrapper::shutdown(
    std::chrono::seconds(30)  // Max timeout
);

if (shutdown_result.is_err()) {
    // Timeout or error during shutdown
    std::cerr << "Shutdown error: " << shutdown_result.error().message << "\n";

    // Force-kill may be necessary
    std::exit(1);
}
```

#### Manual Shutdown Sequence

```cpp
auto graceful_shutdown(std::chrono::seconds timeout) -> common::VoidResult {
    auto start_time = std::chrono::steady_clock::now();

    // Tier 3: network_system (stop accepting new connections)
    auto network_result = network_system->shutdown(
        std::chrono::seconds(5)
    );
    if (network_result.is_err()) {
        logger->log(common::log_level::error,
                   "Network shutdown failed: " + network_result.error().message);
    }

    // Tier 2: Service systems (parallel shutdown)
    auto db_future = std::async(std::launch::async, [&]() {
        return database_system->shutdown(std::chrono::seconds(5));
    });

    auto logger_future = std::async(std::launch::async, [&]() {
        return logger_system->shutdown(std::chrono::seconds(5));
    });

    auto db_result = db_future.get();
    auto logger_result = logger_future.get();

    // Tier 1: thread_system (wait for all tasks to complete)
    auto elapsed = std::chrono::steady_clock::now() - start_time;
    auto remaining = timeout - std::chrono::duration_cast<std::chrono::seconds>(elapsed);

    auto thread_result = thread_system->shutdown(remaining);
    if (thread_result.is_err()) {
        return common::make_error(
            common::error_codes::TIMEOUT,
            "Thread pool shutdown timeout",
            "shutdown_manager"
        );
    }

    return common::ok();
}
```

### Timeout and Error Recovery

#### Shutdown with Timeout Handling

```cpp
auto shutdown_with_fallback(std::chrono::seconds timeout) -> common::VoidResult {
    auto deadline = std::chrono::steady_clock::now() + timeout;

    // Try graceful shutdown
    auto result = graceful_shutdown(timeout);

    if (result.is_ok()) {
        return result;
    }

    // Graceful shutdown failed, check time remaining
    auto now = std::chrono::steady_clock::now();
    if (now >= deadline) {
        logger->log(common::log_level::error,
                   "Graceful shutdown timeout, forcing termination");

        // Force-stop all systems
        network_system->force_stop();
        database_system->force_stop();
        thread_system->force_stop();

        return common::make_error(
            common::error_codes::TIMEOUT,
            "Forced shutdown after timeout",
            "shutdown_manager"
        );
    }

    // Retry with remaining time
    auto remaining = std::chrono::duration_cast<std::chrono::seconds>(deadline - now);
    logger->log(common::log_level::warn,
               "Retrying shutdown with " + std::to_string(remaining.count()) + "s remaining");

    return graceful_shutdown(remaining);
}
```

#### Initialization Rollback

```cpp
auto initialize_with_rollback() -> common::VoidResult {
    std::vector<std::function<void()>> cleanup_stack;

    // Initialize thread_system
    auto thread_result = thread::initialize();
    if (thread_result.is_err()) {
        return thread_result;
    }
    cleanup_stack.push_back([&]() { thread::shutdown(); });

    // Initialize logger_system
    auto logger_result = logger::initialize(thread_result.value());
    if (logger_result.is_err()) {
        // Rollback: shutdown thread_system
        for (auto& cleanup : cleanup_stack) {
            cleanup();
        }
        return logger_result;
    }
    cleanup_stack.push_back([&]() { logger::shutdown(); });

    // Initialize database_system
    auto db_result = database::initialize();
    if (db_result.is_err()) {
        // Rollback: shutdown logger and thread
        for (auto& cleanup : cleanup_stack) {
            cleanup();
        }
        return db_result;
    }
    cleanup_stack.push_back([&]() { database::shutdown(); });

    // Success: clear cleanup stack (no rollback needed)
    cleanup_stack.clear();
    return common::ok();
}
```

---

## 7. Complete Example Application

### Application Overview

A production-ready multi-system application demonstrating:
- Integration of 4 systems (common, thread, logger, database)
- Error handling across system boundaries
- Configuration management
- Graceful initialization and shutdown
- Build system setup

**Systems Used**:
- `common_system`: Foundation (Result<T>, interfaces)
- `thread_system`: Async request processing
- `logger_system`: Structured logging
- `database_system`: Data persistence

### Source Code

**File**: `examples/multi_system_app/main.cpp`

```cpp
// examples/multi_system_app/main.cpp
#include <kcenon/common/di/unified_bootstrapper.h>
#include <kcenon/common/config/config_reader.h>
#include <kcenon/database/core/database.h>
#include <kcenon/thread/core/thread_pool.h>
#include <kcenon/logger/core/logger.h>

#include <iostream>
#include <csignal>
#include <atomic>

using namespace kcenon;

// Application state
std::atomic<bool> g_running{true};

// Signal handler for graceful shutdown
void signal_handler(int signal) {
    std::cout << "\nReceived signal " << signal << ", shutting down...\n";
    g_running = false;
}

// User data model
struct User {
    int id;
    std::string name;
    std::string email;
};

// Business logic: Process user registration
auto register_user(
    std::shared_ptr<database::database> db,
    std::shared_ptr<common::interfaces::ILogger> logger,
    const std::string& name,
    const std::string& email
) -> common::Result<User> {
    // Validate input
    if (name.empty() || email.empty()) {
        return common::make_error<User>(
            common::error_codes::INVALID_ARGUMENT,
            "Name and email are required",
            "register_user"
        );
    }

    logger->log(common::log_level::info,
               "Registering user: " + name + " <" + email + ">");

    // Check if user already exists
    auto check_query = "SELECT COUNT(*) FROM users WHERE email = '" + email + "'";
    auto check_result = db->query(check_query);

    if (check_result.is_err()) {
        return common::make_error<User>(check_result.error());
    }

    if (check_result.value().rows[0].get<int>(0) > 0) {
        return common::make_error<User>(
            common::error_codes::ALREADY_EXISTS,
            "User with email already exists",
            "register_user"
        );
    }

    // Insert new user
    auto insert_query = "INSERT INTO users (name, email) VALUES ('"
                       + name + "', '" + email + "')";
    auto insert_result = db->execute(insert_query);

    if (insert_result.is_err()) {
        logger->log(common::log_level::error,
                   "Failed to insert user: " + insert_result.error().message);
        return common::make_error<User>(insert_result.error());
    }

    // Retrieve inserted user
    auto user_id = db->last_insert_id();

    logger->log(common::log_level::info,
               "User registered successfully with ID: " + std::to_string(user_id));

    return common::ok(User{
        .id = static_cast<int>(user_id),
        .name = name,
        .email = email
    });
}

int main(int argc, char** argv) {
    // Install signal handlers
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);

    std::cout << "=== Multi-System Application Demo ===\n\n";

    // Step 1: Load Configuration
    std::cout << "1. Loading configuration...\n";
    auto config_result = common::config::read_yaml("app.yaml");
    if (config_result.is_err()) {
        std::cerr << "Config error: " << config_result.error().message << "\n";
        std::cerr << "Using default configuration\n";
    }
    auto cfg = config_result.is_ok() ? config_result.value()
                                      : common::config::Config::defaults();

    // Step 2: Initialize Systems
    std::cout << "2. Initializing systems...\n";
    common::di::bootstrapper_options opts;
    opts.enable_logging = true;
    opts.enable_database = true;
    opts.config_path = "app.yaml";

    auto init_result = common::di::unified_bootstrapper::initialize(opts);
    if (init_result.is_err()) {
        std::cerr << "Initialization failed: "
                  << init_result.error().message << "\n";
        return 1;
    }

    std::cout << "   ✓ Systems initialized successfully\n\n";

    // Step 3: Resolve Services
    auto& services = common::di::unified_bootstrapper::services();
    auto logger = services.resolve<common::interfaces::ILogger>();
    auto executor = services.resolve<common::interfaces::IExecutor>();

    // Step 4: Setup Database
    std::cout << "3. Setting up database...\n";
    database::connection_config db_config;
    db_config.type = database::db_type::sqlite;
    db_config.path = "users.db";

    auto db_result = database::database::connect(db_config);
    if (db_result.is_err()) {
        std::cerr << "Database connection failed: "
                  << db_result.error().message << "\n";
        common::di::unified_bootstrapper::shutdown();
        return 1;
    }
    auto db = db_result.value();

    // Create users table
    auto create_table_result = db->execute(
        "CREATE TABLE IF NOT EXISTS users ("
        "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  name TEXT NOT NULL,"
        "  email TEXT UNIQUE NOT NULL"
        ")"
    );

    if (create_table_result.is_err()) {
        std::cerr << "Table creation failed\n";
        common::di::unified_bootstrapper::shutdown();
        return 1;
    }

    std::cout << "   ✓ Database ready\n\n";

    // Step 5: Application Logic
    std::cout << "4. Running application...\n";
    logger->log(common::log_level::info, "Application started");

    // Register sample users
    std::vector<std::pair<std::string, std::string>> sample_users = {
        {"Alice Smith", "alice@example.com"},
        {"Bob Johnson", "bob@example.com"},
        {"Carol Williams", "carol@example.com"}
    };

    for (const auto& [name, email] : sample_users) {
        auto result = register_user(db, logger, name, email);

        if (result.is_ok()) {
            const auto& user = result.value();
            std::cout << "   ✓ Registered: " << user.name
                      << " (ID: " << user.id << ")\n";
        } else {
            std::cout << "   ✗ Failed: " << name
                      << " - " << result.error().message << "\n";
        }

        // Simulate some processing time
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        if (!g_running) {
            break;
        }
    }

    // Query all users
    std::cout << "\n5. Querying all users...\n";
    auto users_result = db->query("SELECT id, name, email FROM users");

    if (users_result.is_ok()) {
        const auto& rows = users_result.value().rows;
        std::cout << "   Total users: " << rows.size() << "\n";

        for (const auto& row : rows) {
            std::cout << "   - " << row.get<std::string>(1)
                      << " <" << row.get<std::string>(2) << ">\n";
        }
    }

    logger->log(common::log_level::info, "Application completed");

    // Step 6: Graceful Shutdown
    std::cout << "\n6. Shutting down...\n";
    auto shutdown_result = common::di::unified_bootstrapper::shutdown(
        std::chrono::seconds(5)
    );

    if (shutdown_result.is_err()) {
        std::cerr << "Shutdown error: "
                  << shutdown_result.error().message << "\n";
        return 1;
    }

    std::cout << "   ✓ Shutdown complete\n";
    std::cout << "\n=== Application Finished ===\n";

    return 0;
}
```

### Build Configuration

**File**: `examples/multi_system_app/CMakeLists.txt`

```cmake
cmake_minimum_required(VERSION 3.20)
project(multi_system_app VERSION 1.0.0 LANGUAGES CXX)

# C++20 required
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find kcenon systems
find_package(common_system CONFIG REQUIRED)
find_package(thread_system CONFIG REQUIRED)
find_package(logger_system CONFIG REQUIRED)
find_package(database_system CONFIG REQUIRED)

# Executable
add_executable(multi_system_app main.cpp)

# Link libraries
target_link_libraries(multi_system_app PRIVATE
    kcenon::common_system
    kcenon::thread_system
    kcenon::logger_system
    kcenon::database_system
)

# Copy config file to build directory
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/app.yaml
    ${CMAKE_CURRENT_BINARY_DIR}/app.yaml
    COPYONLY
)

# Install
install(TARGETS multi_system_app
    RUNTIME DESTINATION bin
)
```

**File**: `examples/multi_system_app/app.yaml`

```yaml
common:
  log_level: info
  shutdown_timeout_ms: 5000

logging:
  enabled: true
  async: true
  sinks:
    - type: console
      level: info
    - type: file
      path: logs/app.log
      level: debug

thread:
  pool_size: 4

database:
  type: sqlite
  path: users.db
  timeout_sec: 5
```

### Running the Example

```bash
# Build
mkdir build && cd build
cmake ..
cmake --build .

# Run
./multi_system_app
```

**Expected Output**:
```
=== Multi-System Application Demo ===

1. Loading configuration...
2. Initializing systems...
   ✓ Systems initialized successfully

3. Setting up database...
   ✓ Database ready

4. Running application...
   ✓ Registered: Alice Smith (ID: 1)
   ✓ Registered: Bob Johnson (ID: 2)
   ✓ Registered: Carol Williams (ID: 3)

5. Querying all users...
   Total users: 3
   - Alice Smith <alice@example.com>
   - Bob Johnson <bob@example.com>
   - Carol Williams <carol@example.com>

6. Shutting down...
   ✓ Shutdown complete

=== Application Finished ===
```

---

## Related Documentation

- [Quick Start Guide](guides/QUICK_START.md) - Getting started with a single system
- [Simple Integration Examples](guides/INTEGRATION.md) - Basic 2-3 system integrations
- [Architecture Overview](ARCHITECTURE.md) - System design philosophy
- [Dependency Matrix](advanced/DEPENDENCY_MATRIX.md) - Detailed dependency analysis
- [API Reference](API_REFERENCE.md) - Interface specifications
- [Rust/C++ Parity Matrix](RUST_PARITY.md) - Feature comparison between C++ and Rust ports
- [Error Code Registry](ERROR_CODE_REGISTRY.md) - Complete error code reference

---

**Document Version**: 3.0.0 (Complete Guide)
**Last Updated**: 2026-02-08
**Authors**: kcenon team
**Related Issues**: kcenon/common_system#329, #334, #335, #336

**Korean Translation**: A Korean translation of this complete guide will be provided in a separate document (`docs/ko/INTEGRATION_GUIDE.md`).
