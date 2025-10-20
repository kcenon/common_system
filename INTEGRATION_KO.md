> **Language:** [English](INTEGRATION.md) | **한국어**

# 시스템 통합 가이드

## 목차

- [개요](#개요)
- [빠른 시작](#빠른-시작)
- [통합 패턴](#통합-패턴)
- [완전한 예제](#완전한-예제)
- [문제 해결](#문제-해결)

## 개요

이 가이드는 애플리케이션에서 7개의 핵심 시스템을 통합하는 방법을 설명합니다. 각 섹션은 설명과 함께 실용적인 예제를 제공합니다.

## 빠른 시작

### 최소 통합

시스템을 사용하는 가장 간단한 방법:

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

## 통합 패턴

### 패턴 1: Logger + Monitoring

애플리케이션의 로깅 활동 모니터링:

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

**핵심 포인트**:
- `set_monitor()` 호출 이후 자동으로 모니터링됩니다
- Metrics에는 `log_count`, `error_count`, `warning_count`가 포함됩니다
- 모니터링이 비활성화되면 성능 오버헤드가 없습니다

### 패턴 2: Network + Thread + Logger

동시성을 지원하는 네트워크 서버 구축:

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

**핵심 포인트**:
- Server는 동시 메시지 처리를 위해 thread pool을 사용합니다
- Logger는 모든 서버 이벤트를 기록합니다
- Container는 타입 안전 메시지 파싱을 제공합니다

### 패턴 3: Database + Container + Result<T>

타입 안전 데이터베이스 작업:

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

**핵심 포인트**:
- `Result<T>`는 타입 안전 에러 처리를 제공합니다
- 예외가 필요 없습니다
- 에러에는 상세한 에러 코드와 메시지가 포함됩니다

### 패턴 4: 풀스택 통합

완전한 애플리케이션을 위한 모든 시스템 결합:

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

## 완전한 예제

### 예제 1: 비동기 메시지 큐

모니터링을 사용한 고성능 메시지 큐 구축:

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

### 예제 2: REST API를 사용한 마이크로서비스

다른 시스템과 network_system 결합:

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

## 문제 해결

### CMake가 시스템을 찾을 수 없음

**문제**: `find_package(xxx_system CONFIG REQUIRED)` 실패

**해결책**:
```bash
# Option 1: Install system to standard location
cd xxx_system
cmake -B build -S . -DCMAKE_INSTALL_PREFIX=/usr/local
cmake --build build
sudo cmake --install build

# Option 2: Use CMAKE_PREFIX_PATH
cmake -B build -S . -DCMAKE_PREFIX_PATH=/path/to/systems
```

### "common_system not found"로 빌드 실패

**문제**: 시스템에 common_system이 필요하지만 찾을 수 없음

**해결책**:
```bash
# Build with common_system integration disabled (Tier 2 only)
cmake -B build -S . -DBUILD_WITH_COMMON_SYSTEM=OFF

# Or ensure common_system is in a sibling directory
/path/to/systems/
├── common_system/
├── thread_system/
└── your_project/
```

### 링커 오류: 정의되지 않은 참조

**문제**: 정의되지 않은 심볼 오류로 링크 실패

**해결책**:
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

### 런타임: Logger 출력이 안 됨

**문제**: Logger가 생성되었지만 출력이 보이지 않음

**해결책**:
```cpp
// Ensure logger is properly configured
auto logger = kcenon::logger::create_console_logger();

// Set minimum log level
logger->set_level(log_level::debug);  // Show all logs

// Flush before exit
logger->flush();
```

### 성능: 높은 CPU 사용량

**문제**: 애플리케이션이 과도한 CPU를 사용함

**해결책**:
1. **Thread pool 크기 줄이기**: `create_thread_pool(100)` 대신 `create_thread_pool(4)` 사용
2. **배치 로깅 사용**: Logger는 자동으로 배치 처리하지만, `flush()`를 너무 자주 호출하지 않도록 합니다
3. **프로덕션에서 모니터링 비활성화**: 필요 없으면 `BUILD_WITH_MONITORING=OFF` 설정

## 모범 사례

### 1. 리소스 관리

RAII와 스마트 포인터 사용:

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

### 2. 에러 처리

항상 `Result<T>` 확인:

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

### 3. 로깅 레벨

적절한 로그 레벨 사용:

```cpp
logger->log(log_level::debug, "Detailed debugging info");    // Development
logger->log(log_level::info, "Application started");         // Normal operation
logger->log(log_level::warning, "Deprecated API used");      // Potential issues
logger->log(log_level::error, "Failed to connect");          // Errors
logger->log(log_level::fatal, "Out of memory");              // Critical errors
```

### 4. Thread Pool 크기 조정

적절한 워커 수 선택:

```cpp
// CPU-bound tasks: Use CPU core count
auto pool = create_thread_pool(std::thread::hardware_concurrency());

// I/O-bound tasks: Use higher count
auto pool = create_thread_pool(std::thread::hardware_concurrency() * 2);

// Mixed workload: Start conservative
auto pool = create_thread_pool(4);
```

## 참고 자료

- [INTEGRATION_POLICY.md](./INTEGRATION_POLICY.md) - 통합 정책
- [ARCHITECTURE.md](./ARCHITECTURE.md) - 아키텍처 개요
- 개별 시스템 README - 상세 API 문서
