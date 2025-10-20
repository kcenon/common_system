> **Language:** [English](MIGRATION.md) | **한국어**

# 마이그레이션 가이드

## 목차

- [개요](#개요)
- [common_system 통합으로 마이그레이션](#common_system-통합으로-마이그레이션)
- [Result<T> 패턴으로 마이그레이션](#resultt-패턴으로-마이그레이션)
- [표준 인터페이스로 마이그레이션](#표준-인터페이스로-마이그레이션)
- [버전 마이그레이션 가이드](#버전-마이그레이션-가이드)
- [문제 해결](#문제-해결)

## 개요

이 가이드는 기존 코드를 common_system과 통합된 시스템 스위트로 마이그레이션하는 방법을 설명합니다. 각 섹션은 전/후 예제와 함께 단계별 지침을 제공합니다.

## common_system 통합으로 마이그레이션

### 단계 1: CMakeLists.txt 업데이트

프로젝트에 common_system 의존성 추가:

**이전** (독립형):
```cmake
cmake_minimum_required(VERSION 3.20)
project(MyProject)

add_executable(MyApp main.cpp)
```

**이후** (통합):
```cmake
cmake_minimum_required(VERSION 3.20)
project(MyProject)

# Add option for integration
option(BUILD_WITH_COMMON_SYSTEM "Enable common_system integration" ON)

if(BUILD_WITH_COMMON_SYSTEM)
    find_package(common_system CONFIG REQUIRED)
    target_link_libraries(MyApp PRIVATE kcenon::common_system)
    target_compile_definitions(MyApp PRIVATE BUILD_WITH_COMMON_SYSTEM)
endif()

add_executable(MyApp main.cpp)
```

### 단계 2: 빌드 프로세스 업데이트

**이전**:
```bash
cmake -B build -S .
cmake --build build
```

**이후**:
```bash
# With common_system integration (default)
cmake -B build -S .
cmake --build build

# Or explicitly disable if needed
cmake -B build -S . -DBUILD_WITH_COMMON_SYSTEM=OFF
cmake --build build
```

### 단계 3: 소스 코드 업데이트

하위 호환성을 위한 조건부 컴파일 사용:

```cpp
#ifdef BUILD_WITH_COMMON_SYSTEM
#include <kcenon/common/patterns/result.h>
#include <kcenon/common/interfaces/logger_interface.h>
using namespace common;
#endif

class MyClass {
public:
    #ifdef BUILD_WITH_COMMON_SYSTEM
    VoidResult initialize() {
        if (!validate()) {
            return make_error(error_code::invalid_state, "Invalid state");
        }
        return ok();
    }
    #else
    bool initialize() {
        return validate();
    }
    #endif

private:
    bool validate();
};
```

## Result<T> 패턴으로 마이그레이션

### bool 반환 값에서 마이그레이션

**이전**:
```cpp
bool process_data(const std::string& data) {
    if (data.empty()) {
        return false;  // Why did it fail?
    }

    if (!validate(data)) {
        return false;  // What's wrong with the data?
    }

    return do_processing(data);
}

// Usage
if (!process_data(input)) {
    std::cerr << "Processing failed" << std::endl;  // No context!
}
```

**이후**:
```cpp
#include <kcenon/common/patterns/result.h>
using namespace common;

VoidResult process_data(const std::string& data) {
    if (data.empty()) {
        return make_error(error_code::invalid_argument,
                         "Data cannot be empty");
    }

    if (!validate(data)) {
        return make_error(error_code::validation_failed,
                         std::format("Invalid data format: {}", data));
    }

    return do_processing(data);
}

// Usage
auto result = process_data(input);
if (is_error(result)) {
    auto err = get_error(result);
    std::cerr << "Processing failed: " << err.message
              << " (code: " << static_cast<int>(err.code) << ")" << std::endl;

    // Can handle different error types
    if (err.code == error_code::invalid_argument) {
        // Handle invalid input
    }
}
```

### 예외에서 마이그레이션

**이전**:
```cpp
User load_user(const std::string& id) {
    auto conn = database->connect();
    if (!conn) {
        throw std::runtime_error("Database connection failed");
    }

    auto user = conn->query("SELECT * FROM users WHERE id = ?", id);
    if (!user) {
        throw std::runtime_error("User not found");
    }

    return *user;
}

// Usage
try {
    auto user = load_user("123");
    process(user);
} catch (const std::exception& e) {
    std::cerr << "Error: " << e.what() << std::endl;
}
```

**이후**:
```cpp
#include <kcenon/common/patterns/result.h>
using namespace common;

Result<User> load_user(const std::string& id) {
    auto conn_result = database->connect();
    if (is_error(conn_result)) {
        return get_error(conn_result);  // Propagate error
    }

    auto conn = get_value(conn_result);
    auto user_result = conn->query("SELECT * FROM users WHERE id = ?", id);

    if (is_error(user_result)) {
        return make_error(error_code::not_found,
                         std::format("User {} not found", id));
    }

    return get_value(user_result);
}

// Usage (no exceptions)
auto result = load_user("123");
if (is_ok(result)) {
    auto user = get_value(result);
    process(user);
} else {
    auto err = get_error(result);
    std::cerr << "Error: " << err.message << std::endl;

    // Type-safe error handling
    switch (err.code) {
        case error_code::connection_failed:
            retry_connection();
            break;
        case error_code::not_found:
            create_default_user();
            break;
        default:
            log_error(err);
    }
}
```

### Optional<T>에서 마이그레이션

**이전**:
```cpp
std::optional<Config> load_config(const std::string& path) {
    std::ifstream file(path);
    if (!file) {
        return std::nullopt;  // Why did it fail?
    }

    Config config;
    if (!parse(file, config)) {
        return std::nullopt;  // Parse error? File error?
    }

    return config;
}

// Usage
auto config = load_config("config.json");
if (!config) {
    std::cerr << "Failed to load config" << std::endl;  // No details!
}
```

**이후**:
```cpp
Result<Config> load_config(const std::string& path) {
    std::ifstream file(path);
    if (!file) {
        return make_error(error_code::file_not_found,
                         std::format("Config file not found: {}", path));
    }

    Config config;
    if (!parse(file, config)) {
        return make_error(error_code::parse_error,
                         std::format("Failed to parse config file: {}", path));
    }

    return ok(std::move(config));
}

// Usage
auto result = load_config("config.json");
if (is_ok(result)) {
    auto config = get_value(result);
    apply(config);
} else {
    auto err = get_error(result);
    std::cerr << "Config error: " << err.message << std::endl;

    // Can provide fallback based on error type
    if (err.code == error_code::file_not_found) {
        use_default_config();
    } else if (err.code == error_code::parse_error) {
        log_parse_error(err);
        use_default_config();
    }
}
```

## 표준 인터페이스로 마이그레이션

### ILogger 인터페이스로 마이그레이션

**이전** (직접 의존성):
```cpp
#include <spdlog/spdlog.h>

class MyService {
private:
    std::shared_ptr<spdlog::logger> logger_;

public:
    MyService() {
        logger_ = spdlog::stdout_color_mt("service");
    }

    void process() {
        logger_->info("Processing started");
        // ... processing
        logger_->info("Processing completed");
    }
};
```

**이후** (인터페이스 추상화):
```cpp
#include <kcenon/common/interfaces/logger_interface.h>

class MyService {
private:
    std::shared_ptr<common::interfaces::ILogger> logger_;

public:
    MyService(std::shared_ptr<common::interfaces::ILogger> logger)
        : logger_(std::move(logger)) {}

    void process() {
        logger_->log(log_level::info, "Processing started");
        // ... processing
        logger_->log(log_level::info, "Processing completed");
    }
};

// Usage - can use ANY logger implementation
#include <kcenon/logger/core/logger.h>

auto logger = kcenon::logger::create_console_logger();
MyService service(logger);
service.process();
```

**장점**:
- 특정 logger 구현에 직접 의존하지 않음
- Logger 구현을 쉽게 교체 가능
- 테스트가 더 쉬움 (mock logger 사용 가능)
- 모든 시스템에서 일관된 로깅 인터페이스

### IExecutor 인터페이스로 마이그레이션

**이전** (직접 thread 관리):
```cpp
#include <thread>
#include <queue>

class TaskProcessor {
private:
    std::vector<std::thread> workers_;
    std::queue<std::function<void()>> tasks_;
    std::mutex mutex_;

public:
    TaskProcessor(size_t num_threads) {
        for (size_t i = 0; i < num_threads; ++i) {
            workers_.emplace_back([this]() {
                worker_loop();
            });
        }
    }

    void submit(std::function<void()> task) {
        std::lock_guard lock(mutex_);
        tasks_.push(std::move(task));
    }

private:
    void worker_loop() {
        // Complex worker implementation
    }
};
```

**이후** (인터페이스 추상화):
```cpp
#include <kcenon/common/interfaces/executor_interface.h>

class TaskProcessor {
private:
    std::shared_ptr<common::interfaces::IExecutor> executor_;

public:
    TaskProcessor(std::shared_ptr<common::interfaces::IExecutor> executor)
        : executor_(std::move(executor)) {}

    void submit(std::function<void()> task) {
        executor_->submit(std::move(task));
    }
};

// Usage - thread_system handles the complexity
#include <kcenon/thread/core/thread_pool.h>

auto executor = kcenon::thread::create_thread_pool(4);
TaskProcessor processor(executor);
processor.submit([]() {
    std::cout << "Task executed" << std::endl;
});
```

### IMonitor 인터페이스로 마이그레이션

**이전** (커스텀 metrics):
```cpp
class ServiceMetrics {
private:
    std::atomic<int64_t> request_count_{0};
    std::atomic<int64_t> error_count_{0};
    std::atomic<double> avg_latency_{0.0};

public:
    void record_request() { ++request_count_; }
    void record_error() { ++error_count_; }
    void record_latency(double ms) { avg_latency_ = ms; }

    void print_stats() {
        std::cout << "Requests: " << request_count_ << std::endl;
        std::cout << "Errors: " << error_count_ << std::endl;
        std::cout << "Avg latency: " << avg_latency_ << "ms" << std::endl;
    }
};
```

**이후** (표준 인터페이스):
```cpp
#include <kcenon/common/interfaces/monitoring_interface.h>

class Service {
private:
    std::shared_ptr<common::interfaces::IMonitor> monitor_;

public:
    Service(std::shared_ptr<common::interfaces::IMonitor> monitor)
        : monitor_(std::move(monitor)) {}

    void handle_request() {
        auto start = std::chrono::steady_clock::now();

        monitor_->record_metric("requests_total", 1);

        try {
            // Process request
        } catch (...) {
            monitor_->record_metric("errors_total", 1);
            throw;
        }

        auto duration = std::chrono::steady_clock::now() - start;
        auto ms = std::chrono::duration<double, std::milli>(duration).count();
        monitor_->record_metric("latency_ms", ms);
    }

    void print_stats() {
        auto metrics = monitor_->collect_metrics();
        for (const auto& [name, metric] : metrics) {
            std::cout << name << ": " << metric.value << std::endl;
        }
    }
};

// Usage
#include <kcenon/monitoring/core/performance_monitor.h>

auto monitor = kcenon::monitoring::create_performance_monitor();
Service service(monitor);
```

## 버전 마이그레이션 가이드

### 0.x에서 1.0으로 마이그레이션

#### 호환성 깨는 변경사항

1. **Result<T> API 변경**
   ```cpp
   // Old (0.x)
   if (result.is_ok()) {
       auto value = result.value();
   }

   // New (1.0)
   if (is_ok(result)) {
       auto value = get_value(result);
   }
   ```

2. **Logger 인터페이스 변경**
   ```cpp
   // Old (0.x)
   logger->log(LogLevel::INFO, "message");

   // New (1.0)
   logger->log(log_level::info, "message");
   ```

3. **네임스페이스 변경**
   ```cpp
   // Old (0.x)
   using namespace kcenon::common;

   // New (1.0)
   using namespace common;
   using namespace common::interfaces;
   ```

#### 마이그레이션 단계

1. **의존성 업데이트**
   ```cmake
   # Update version requirement
   find_package(common_system 1.0 REQUIRED)
   ```

2. **코드 업데이트**
   - `result.is_ok()`를 `is_ok(result)`로 교체
   - `result.value()`를 `get_value(result)`로 교체
   - `result.error()`를 `get_error(result)`로 교체
   - enum 이름을 소문자로 업데이트 (예: `LogLevel::INFO` → `log_level::info`)

3. **철저한 테스트**
   ```bash
   # Run all tests
   ctest --output-on-failure
   ```

### 마이너 버전 간 마이그레이션

마이너 버전 업데이트는 하위 호환됩니다. 코드 변경이 필요 없지만 다음을 검토하세요:

- 릴리스 노트의 새로운 기능
- 사용 중단된(deprecated) API (컴파일 시 경고)
- 성능 개선 사항

## 문제 해결

### 일반적인 마이그레이션 문제

#### 문제 1: 마이그레이션 후 링커 오류

**문제**:
```
undefined reference to `common::make_error(...)`
```

**해결책**:
```cmake
# Ensure common_system is linked
target_link_libraries(MyApp PRIVATE kcenon::common_system)
```

#### 문제 2: 함수 호출이 모호함

**문제**:
```cpp
error: call to 'ok' is ambiguous
```

**해결책**:
```cpp
// Explicitly use namespace
auto result = common::ok(value);

// Or use namespace directive
using namespace common;
auto result = ok(value);
```

#### 문제 3: 성능 저하

**문제**: Result<T>로 마이그레이션 후 코드가 느려짐

**해결책**:
```cpp
// Enable optimizations
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release

// Use move semantics
return ok(std::move(large_object));

// Avoid unnecessary copies
auto result = process();  // Don't: auto result = Result(process());
```

#### 문제 4: common_system을 찾을 수 없음

**문제**:
```
CMake Error: Could not find a package configuration file provided by "common_system"
```

**해결책**:
```bash
# Option 1: Install common_system
cd common_system
cmake -B build -S . -DCMAKE_INSTALL_PREFIX=/usr/local
sudo cmake --install build

# Option 2: Use CMAKE_PREFIX_PATH
cmake -B build -S . -DCMAKE_PREFIX_PATH=/path/to/systems

# Option 3: Use sibling directory structure
/path/to/systems/
├── common_system/
├── thread_system/
└── my_project/
```

### 마이그레이션 체크리스트

마이그레이션 시작 전:

- [ ] 기존 코드 백업
- [ ] ARCHITECTURE.md 검토
- [ ] INTEGRATION_POLICY.md 읽기
- [ ] 시스템 호환성 확인 (C++20 필요)
- [ ] 단계별 마이그레이션 계획 (모든 것을 한 번에 마이그레이션하지 않기)

마이그레이션 중:

- [ ] CMakeLists.txt 업데이트
- [ ] 하위 호환성을 위한 조건부 컴파일 추가
- [ ] 한 번에 하나의 모듈만 마이그레이션
- [ ] 각 모듈 마이그레이션 후 테스트
- [ ] 문서 업데이트

마이그레이션 후:

- [ ] 전체 테스트 스위트 실행
- [ ] 성능 테스트
- [ ] 코드 리뷰
- [ ] 프로젝트 문서 업데이트
- [ ] CI/CD 파이프라인 업데이트

### 도움 받기

문제가 발생하면:

1. [INTEGRATION.md](./INTEGRATION.md)에서 예제 확인
2. [ARCHITECTURE.md](./ARCHITECTURE.md)에서 시스템 디자인 검토
3. GitHub에서 기존 이슈 확인
4. 다음 정보와 함께 새 이슈 생성:
   - 시스템 버전
   - 에러 메시지
   - 최소 재현 예제

## 모범 사례

### 점진적 마이그레이션

모든 것을 한 번에 마이그레이션하지 마세요:

```cpp
// Phase 1: Add integration support
#ifdef BUILD_WITH_COMMON_SYSTEM
    Result<Data> new_load_data() { /* ... */ }
#endif

    bool legacy_load_data() { /* keep existing */ }

// Phase 2: Migrate callers gradually
void process() {
    #ifdef BUILD_WITH_COMMON_SYSTEM
    auto result = new_load_data();
    if (is_error(result)) { /* ... */ }
    #else
    if (!legacy_load_data()) { /* ... */ }
    #endif
}

// Phase 3: Remove legacy code (in next release)
Result<Data> load_data() { /* ... */ }
```

### 하위 호환성 유지

기능 플래그 사용:

```cpp
namespace my_app {
    #ifdef BUILD_WITH_COMMON_SYSTEM
    using error_type = common::Error;
    using result_type = common::VoidResult;
    #else
    struct error_type { std::string message; };
    using result_type = bool;
    #endif
}
```

### 변경사항 문서화

README 업데이트:

```markdown
## Dependencies

### Required
- CMake 3.20+
- C++20 compiler

### Optional
- common_system 1.0+ (recommended, enables Result<T> pattern)
- thread_system 1.0+ (for concurrent execution)

## Building

### With Integration (Recommended)
\`\`\`bash
cmake -B build -S . -DBUILD_WITH_COMMON_SYSTEM=ON
cmake --build build
\`\`\`

### Standalone
\`\`\`bash
cmake -B build -S . -DBUILD_WITH_COMMON_SYSTEM=OFF
cmake --build build
\`\`\`
```

## 참고 자료

- [INTEGRATION.md](./INTEGRATION.md) - 예제를 포함한 통합 가이드
- [ARCHITECTURE.md](./ARCHITECTURE.md) - 시스템 아키텍처
- [INTEGRATION_POLICY.md](./INTEGRATION_POLICY.md) - 통합 tier 및 정책
- [NEED_TO_FIX.md](./NEED_TO_FIX.md) - 프로젝트 개선 추적
