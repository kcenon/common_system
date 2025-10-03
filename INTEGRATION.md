# System Integration Guide

## Table of Contents

- [Overview](#overview)
- [Quick Start](#quick-start)
- [Integration Patterns](#integration-patterns)
- [Complete Examples](#complete-examples)
- [Troubleshooting](#troubleshooting)

## Overview

This guide demonstrates how to integrate the 7 core systems in your applications. Each section provides practical examples with explanations.

## Quick Start

### Minimal Integration

The simplest way to use the systems:

```cpp
#include <kcenon/logger/core/logger.h>
#include <kcenon/thread/core/thread_pool.h>

int main() {
    // Create logger
    auto logger = kcenon::logger::create_console_logger();

    // Create thread pool with 4 workers
    auto pool = kcenon::thread::create_thread_pool(4);

    // Submit a job
    pool->submit([&logger]() {
        logger->log(log_level::info, "Job executed");
    });

    return 0;
}
```

**CMakeLists.txt**:
```cmake
find_package(logger_system CONFIG REQUIRED)
find_package(thread_system CONFIG REQUIRED)

add_executable(minimal_example main.cpp)
target_link_libraries(minimal_example PRIVATE
    kcenon::logger_system
    kcenon::thread_system
)
```

## Integration Patterns

### Pattern 1: Logger + Monitoring

Monitor your application's logging activity:

```cpp
#include <kcenon/logger/core/logger.h>
#include <kcenon/monitoring/core/performance_monitor.h>
#include <kcenon/common/interfaces/monitoring_interface.h>

int main() {
    // Create monitoring system
    auto monitor = kcenon::monitoring::create_performance_monitor();

    // Create logger with monitoring
    auto logger = kcenon::logger::create_console_logger();
    logger->set_monitor(monitor.get());

    // Log messages (automatically records metrics)
    logger->log(log_level::info, "Application started");
    logger->log(log_level::warning, "Low memory");
    logger->log(log_level::error, "Connection failed");

    // Check metrics
    auto metrics = monitor->collect_metrics();
    std::cout << "Total logs: " << metrics["log_count"].value << std::endl;
    std::cout << "Error logs: " << metrics["error_count"].value << std::endl;

    return 0;
}
```

**Key Points**:
- Monitoring is automatic once `set_monitor()` is called
- Metrics include: `log_count`, `error_count`, `warning_count`
- Zero performance overhead when monitoring is disabled

### Pattern 2: Network + Thread + Logger

Build a concurrent network server:

```cpp
#include <network_system/core/messaging_server.h>
#include <kcenon/logger/core/logger.h>
#include <kcenon/thread/core/thread_pool.h>
#include "container.h"

int main() {
    // Create infrastructure
    auto logger = kcenon::logger::create_file_logger("server.log");
    auto thread_pool = kcenon::thread::create_thread_pool(8);

    // Create server
    auto server = network_system::create_messaging_server(8080);
    server->set_logger(logger.get());
    server->set_executor(thread_pool.get());

    // Handle incoming messages
    server->on_message([&logger, &thread_pool](const container& msg) {
        // Process message in thread pool
        thread_pool->submit([msg, &logger]() {
            std::string data = msg.get_string("data");
            logger->log(log_level::info,
                std::format("Received: {}", data));
        });
    });

    // Start server
    server->start();

    std::cout << "Server running on port 8080..." << std::endl;
    std::this_thread::sleep_for(std::chrono::hours(1));

    return 0;
}
```

**Key Points**:
- Server uses thread pool for concurrent message handling
- Logger records all server events
- Container provides type-safe message parsing

### Pattern 3: Database + Container + Result<T>

Type-safe database operations:

```cpp
#include <kcenon/database/core/database_manager.h>
#include <kcenon/common/patterns/result.h>
#include "container.h"

using namespace common;

Result<container> fetch_user(const std::string& user_id) {
    auto db = create_database_manager("postgresql://localhost/mydb");

    // Execute query with Result<T> error handling
    auto query_result = db->execute_query(
        "SELECT * FROM users WHERE id = $1",
        {user_id}
    );

    if (is_error(query_result)) {
        return get_error(query_result);  // Propagate error
    }

    auto rows = get_value(query_result);
    if (rows.empty()) {
        return make_error(error_code::not_found, "User not found");
    }

    // Convert database row to container
    container user;
    user.set_value("id", rows[0]["id"]);
    user.set_value("name", rows[0]["name"]);
    user.set_value("email", rows[0]["email"]);

    return ok(std::move(user));
}

int main() {
    auto result = fetch_user("user123");

    if (is_ok(result)) {
        auto user = get_value(result);
        std::cout << "User: " << user.get_string("name") << std::endl;
    } else {
        auto error = get_error(result);
        std::cerr << "Error: " << error.message << std::endl;
    }

    return 0;
}
```

**Key Points**:
- `Result<T>` provides type-safe error handling
- No exceptions needed
- Errors include detailed error codes and messages

### Pattern 4: Full Stack Integration

Combine all systems for a complete application:

```cpp
#include <network_system/core/messaging_server.h>
#include <kcenon/logger/core/logger.h>
#include <kcenon/thread/core/thread_pool.h>
#include <kcenon/monitoring/core/performance_monitor.h>
#include <kcenon/database/core/database_manager.h>
#include "container.h"

class Application {
private:
    std::shared_ptr<kcenon::logger::logger> logger_;
    std::shared_ptr<kcenon::thread::thread_pool> thread_pool_;
    std::shared_ptr<common::interfaces::IMonitor> monitor_;
    std::shared_ptr<database_manager> database_;
    std::shared_ptr<network_system::messaging_server> server_;

public:
    Application() {
        // Initialize infrastructure
        monitor_ = kcenon::monitoring::create_performance_monitor();
        logger_ = kcenon::logger::create_file_logger("app.log");
        logger_->set_monitor(monitor_.get());

        thread_pool_ = kcenon::thread::create_thread_pool(16);

        database_ = create_database_manager(
            "postgresql://localhost/app_db"
        );

        // Initialize server
        server_ = network_system::create_messaging_server(9000);
        server_->set_logger(logger_.get());
        server_->set_executor(thread_pool_.get());
        server_->set_monitor(monitor_.get());

        // Setup message handlers
        server_->on_message([this](const container& msg) {
            handle_message(msg);
        });
    }

    void start() {
        logger_->log(log_level::info, "Application starting...");
        server_->start();
        logger_->log(log_level::info, "Server listening on port 9000");
    }

    void stop() {
        logger_->log(log_level::info, "Application stopping...");
        server_->stop();
        print_statistics();
    }

private:
    void handle_message(const container& msg) {
        // Process in thread pool
        thread_pool_->submit([this, msg]() {
            std::string type = msg.get_string("type");

            if (type == "create_user") {
                auto result = create_user(msg);
                if (is_error(result)) {
                    logger_->log(log_level::error,
                        std::format("Failed to create user: {}",
                                    get_error(result).message));
                }
            }
            // Handle other message types...
        });
    }

    common::VoidResult create_user(const container& msg) {
        std::string name = msg.get_string("name");
        std::string email = msg.get_string("email");

        // Save to database
        auto result = database_->execute(
            "INSERT INTO users (name, email) VALUES ($1, $2)",
            {name, email}
        );

        if (is_ok(result)) {
            logger_->log(log_level::info,
                std::format("User created: {}", name));
            monitor_->record_metric("users_created", 1);
        }

        return result;
    }

    void print_statistics() {
        auto metrics = monitor_->collect_metrics();

        std::cout << "\\n=== Application Statistics ===" << std::endl;
        std::cout << "Total logs: " << metrics["log_count"].value << std::endl;
        std::cout << "Users created: " << metrics["users_created"].value << std::endl;
        std::cout << "Messages processed: "
                  << metrics["messages_processed"].value << std::endl;
    }
};

int main() {
    Application app;
    app.start();

    // Run for some time or wait for signal
    std::this_thread::sleep_for(std::chrono::hours(1));

    app.stop();
    return 0;
}
```

## Complete Examples

### Example 1: Asynchronous Message Queue

Build a high-performance message queue with monitoring:

```cpp
#include <kcenon/thread/core/thread_pool.h>
#include <kcenon/logger/core/logger.h>
#include <kcenon/monitoring/core/performance_monitor.h>
#include "container.h"
#include <queue>
#include <mutex>

class AsyncMessageQueue {
private:
    std::queue<container> queue_;
    std::mutex mutex_;
    std::shared_ptr<kcenon::thread::thread_pool> pool_;
    std::shared_ptr<kcenon::logger::logger> logger_;
    std::shared_ptr<common::interfaces::IMonitor> monitor_;

public:
    AsyncMessageQueue()
        : pool_(kcenon::thread::create_thread_pool(4))
        , logger_(kcenon::logger::create_console_logger())
        , monitor_(kcenon::monitoring::create_performance_monitor())
    {
        logger_->set_monitor(monitor_.get());
    }

    void enqueue(container&& msg) {
        {
            std::lock_guard lock(mutex_);
            queue_.push(std::move(msg));
        }

        monitor_->record_metric("queue_size", queue_.size());

        // Process asynchronously
        pool_->submit([this]() {
            process_next();
        });
    }

private:
    void process_next() {
        container msg;
        {
            std::lock_guard lock(mutex_);
            if (queue_.empty()) return;

            msg = std::move(queue_.front());
            queue_.pop();
        }

        // Simulate processing
        std::string data = msg.get_string("data");
        logger_->log(log_level::info,
            std::format("Processing: {}", data));

        monitor_->record_metric("messages_processed", 1);
    }
};

int main() {
    AsyncMessageQueue queue;

    // Enqueue messages
    for (int i = 0; i < 100; ++i) {
        container msg;
        msg.set_value("data", std::format("Message {}", i));
        queue.enqueue(std::move(msg));
    }

    std::this_thread::sleep_for(std::chrono::seconds(5));
    return 0;
}
```

### Example 2: Microservice with REST API

Combine network_system with other systems:

```cpp
#include <network_system/core/messaging_server.h>
#include <kcenon/logger/core/logger.h>
#include <kcenon/database/core/database_manager.h>
#include "container.h"

class UserService {
private:
    std::shared_ptr<network_system::messaging_server> server_;
    std::shared_ptr<database_manager> db_;
    std::shared_ptr<kcenon::logger::logger> logger_;

public:
    UserService(int port, const std::string& db_url)
        : server_(network_system::create_messaging_server(port))
        , db_(create_database_manager(db_url))
        , logger_(kcenon::logger::create_file_logger("service.log"))
    {
        server_->set_logger(logger_.get());
        setup_routes();
    }

    void start() {
        server_->start();
        logger_->log(log_level::info, "User service started");
    }

private:
    void setup_routes() {
        server_->on_message([this](const container& request) {
            std::string method = request.get_string("method");
            std::string path = request.get_string("path");

            container response;

            if (method == "GET" && path == "/users") {
                response = handle_get_users();
            } else if (method == "POST" && path == "/users") {
                response = handle_create_user(request);
            } else {
                response.set_value("status", 404);
                response.set_value("error", "Not found");
            }

            // Send response (simplified)
            return response;
        });
    }

    container handle_get_users() {
        auto result = db_->execute_query("SELECT * FROM users");

        container response;
        if (is_ok(result)) {
            response.set_value("status", 200);
            response.set_value("data", get_value(result));
        } else {
            response.set_value("status", 500);
            response.set_value("error", get_error(result).message);
        }

        return response;
    }

    container handle_create_user(const container& request) {
        std::string name = request.get_string("name");
        std::string email = request.get_string("email");

        auto result = db_->execute(
            "INSERT INTO users (name, email) VALUES ($1, $2)",
            {name, email}
        );

        container response;
        if (is_ok(result)) {
            logger_->log(log_level::info,
                std::format("User created: {}", name));
            response.set_value("status", 201);
        } else {
            response.set_value("status", 400);
            response.set_value("error", get_error(result).message);
        }

        return response;
    }
};

int main() {
    UserService service(8080, "postgresql://localhost/users_db");
    service.start();

    std::this_thread::sleep_for(std::chrono::hours(24));
    return 0;
}
```

## Troubleshooting

### CMake Cannot Find System

**Problem**: `find_package(xxx_system CONFIG REQUIRED)` fails

**Solution**:
```bash
# Option 1: Install system to standard location
cd xxx_system
cmake -B build -S . -DCMAKE_INSTALL_PREFIX=/usr/local
cmake --build build
sudo cmake --install build

# Option 2: Use CMAKE_PREFIX_PATH
cmake -B build -S . -DCMAKE_PREFIX_PATH=/path/to/systems
```

### Build Fails with "common_system not found"

**Problem**: System requires common_system but cannot find it

**Solution**:
```bash
# Build with common_system integration disabled (Tier 2 only)
cmake -B build -S . -DBUILD_WITH_COMMON_SYSTEM=OFF

# Or ensure common_system is in a sibling directory
/path/to/systems/
├── common_system/
├── thread_system/
└── your_project/
```

### Linker Errors: Undefined References

**Problem**: Linking fails with undefined symbol errors

**Solution**:
```cmake
# Ensure proper link order (dependencies first)
target_link_libraries(MyApp PRIVATE
    kcenon::common_system      # First (foundation)
    kcenon::thread_system      # Then core systems
    kcenon::container_system
    kcenon::logger_system      # Then service systems
    kcenon::network_system     # Finally integration systems
)
```

### Runtime: Logger Not Outputting

**Problem**: Logger created but no output visible

**Solution**:
```cpp
// Ensure logger is properly configured
auto logger = kcenon::logger::create_console_logger();

// Set minimum log level
logger->set_level(log_level::debug);  // Show all logs

// Flush before exit
logger->flush();
```

### Performance: High CPU Usage

**Problem**: Application uses excessive CPU

**Solutions**:
1. **Reduce thread pool size**: `create_thread_pool(4)` instead of `create_thread_pool(100)`
2. **Use batch logging**: Logger automatically batches, but ensure `flush()` isn't called too often
3. **Disable monitoring in production**: Set `BUILD_WITH_MONITORING=OFF` if not needed

## Best Practices

### 1. Resource Management

Use RAII and smart pointers:

```cpp
// Good: Automatic cleanup
{
    auto logger = create_logger();
    auto pool = create_thread_pool(4);
    // Resources cleaned up automatically
}

// Avoid: Manual memory management
logger* log = new logger();  // Don't do this
```

### 2. Error Handling

Always check `Result<T>`:

```cpp
// Good: Check result
auto result = db->execute(query);
if (is_error(result)) {
    handle_error(get_error(result));
    return;
}
process(get_value(result));

// Bad: Ignore result
db->execute(query);  // What if it fails?
```

### 3. Logging Levels

Use appropriate log levels:

```cpp
logger->log(log_level::debug, "Detailed debugging info");    // Development
logger->log(log_level::info, "Application started");         // Normal operation
logger->log(log_level::warning, "Deprecated API used");      // Potential issues
logger->log(log_level::error, "Failed to connect");          // Errors
logger->log(log_level::fatal, "Out of memory");              // Critical errors
```

### 4. Thread Pool Sizing

Choose appropriate worker count:

```cpp
// CPU-bound tasks: Use CPU core count
auto pool = create_thread_pool(std::thread::hardware_concurrency());

// I/O-bound tasks: Use higher count
auto pool = create_thread_pool(std::thread::hardware_concurrency() * 2);

// Mixed workload: Start conservative
auto pool = create_thread_pool(4);
```

## References

- [INTEGRATION_POLICY.md](./INTEGRATION_POLICY.md) - Integration policy
- [ARCHITECTURE.md](./ARCHITECTURE.md) - Architecture overview
- Individual system READMEs for detailed API documentation
