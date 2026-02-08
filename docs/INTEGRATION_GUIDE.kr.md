# 크로스 시스템 통합 가이드

> **언어:** [English](INTEGRATION_GUIDE.md) | **한국어**

**완전한 가이드**: 의존성 맵, 통합 패턴, 시나리오, 에러 처리, 예제

**상태**: ✅ **완료 (Parts 1-3)**

이 가이드는 kcenon 생태계의 7개 시스템이 어떻게 함께 작동하는지에 대한 포괄적인 개요를 제공합니다. 여러 시스템을 활용하는 애플리케이션을 구축하기 위한 의존성 관리, 초기화 시퀀스, 통합 패턴을 다룹니다.

---

## 목차

- [개요](#개요)
- [생태계 의존성 맵](#1-생태계-의존성-맵)
  - [시각적 의존성 트리](#시각적-의존성-트리)
  - [계층 기반 초기화 순서](#계층-기반-초기화-순서)
  - [의존성 설명](#의존성-설명)
- [통합 패턴](#2-통합-패턴)
  - [Unified Bootstrapper 사용](#unified-bootstrapper-사용)
  - [수동 Adapter 연결](#수동-adapter-연결)
  - [Service Container 통합](#service-container-통합)
  - [라이프사이클 관리](#라이프사이클-관리)
- [일반적인 통합 시나리오](#3-일반적인-통합-시나리오)
  - [시나리오 1: Web API 서버](#시나리오-1-web-api-서버)
  - [시나리오 2: 데이터 파이프라인](#시나리오-2-데이터-파이프라인)
  - [시나리오 3: 모니터링 스택](#시나리오-3-모니터링-스택)
  - [시나리오 4: Full Stack 애플리케이션](#시나리오-4-full-stack-애플리케이션)
- [시스템 간 설정](#4-시스템-간-설정)
  - [통합 설정 전략](#통합-설정-전략)
  - [설정 구조](#설정-구조)
  - [시나리오별 설정 예제](#시나리오별-설정-예제)
- [시스템 간 에러 처리](#5-시스템-간-에러-처리)
  - [에러 코드 범위](#에러-코드-범위)
  - [크로스 시스템 에러 전파](#크로스-시스템-에러-전파)
  - [Result 조합 패턴](#result-조합-패턴)
- [초기화 및 종료](#6-초기화-및-종료)
  - [초기화 시퀀스](#초기화-시퀀스)
  - [종료 시퀀스](#종료-시퀀스)
  - [타임아웃 및 에러 복구](#타임아웃-및-에러-복구)
- [완전한 예제 애플리케이션](#7-완전한-예제-애플리케이션)
  - [애플리케이션 개요](#애플리케이션-개요)
  - [소스 코드](#소스-코드)
  - [빌드 설정](#빌드-설정)
  - [예제 실행](#예제-실행)
- [관련 문서](#관련-문서)

---

## 개요

kcenon 생태계는 **7개의 핵심 C++ 시스템**(및 Rust 포트)으로 구성되며, 각각은 인터페이스 기반 설계를 통해 느슨한 결합을 유지하면서 특화된 기능을 제공합니다. 이 가이드는 여러 시스템을 통합하여 응집력 있는 애플리케이션을 만드는 방법을 설명합니다.

### 7개 핵심 시스템

| 시스템 | 계층 | 설명 | 주요 인터페이스 |
|--------|------|------|----------------|
| **common_system** | 0 | 인터페이스와 패턴을 제공하는 기반 계층 | `ILogger`, `IExecutor`, `Result<T>` |
| **thread_system** | 1 | 실행 및 작업 스케줄링 | `IExecutor` 구현 |
| **container_system** | 1 | 데이터 저장 및 직렬화 | `Result<T>` 사용 |
| **logger_system** | 2 | 구조화된 로깅 및 진단 | `ILogger` 구현 |
| **monitoring_system** | 2 | 메트릭, 상태 확인, 관찰 가능성 (Rust) | 이벤트 버스 사용, `IMonitor` |
| **database_system** | 2 | 영구 데이터 저장 | `Result<T>`, `IExecutor` 사용 |
| **network_system** | 3 | TCP/IP 통신 및 메시징 | `IExecutor`, `ILogger` 사용 |

### 해결하는 통합 과제

여러 시스템을 사용하는 애플리케이션을 구축할 때 개발자가 직면하는 문제:
- **초기화 순서 복잡성**: 어떤 시스템을 먼저 초기화해야 할까?
- **의존성 해결**: 서비스를 어떻게 연결해야 할까?
- **설정 관리**: 여러 시스템을 일관되게 구성하는 방법은?
- **라이프사이클 조정**: 상호 연결된 시스템을 우아하게 종료하는 방법은?

이 가이드는 이러한 과제를 해결하기 위한 검증된 패턴을 제공합니다.

---

## 1. 생태계 의존성 맵

### 시각적 의존성 트리

생태계는 순환 의존성을 방지하고 깔끔한 아키텍처를 보장하기 위해 엄격한 **계층 기반 계층 구조**를 따릅니다:

```
계층 0: 기반 계층 (의존성 없음)
┌─────────────────────────────────────────────────┐
│         common_system                           │
│  • 인터페이스: ILogger, IExecutor, IMonitor     │
│  • 패턴: Result<T>, VoidResult                  │
│  • 에러 처리: ErrorInfo, error_codes            │
│  • 어댑터: typed_adapter, smart_adapter         │
└─────────────────────────────────────────────────┘
                    ▲
                    │ 인터페이스 제공
        ┌───────────┴───────────┐
        │                       │
계층 1: 핵심 시스템 (계층 0에만 의존)
┌─────────────────┐     ┌─────────────────┐
│  thread_system  │     │container_system │
│                 │     │                 │
│ • IExecutor     │     │ • Result<T>     │
│   구현          │     │   사용          │
│ • 작업 큐잉     │     │ • JSON/Binary   │
│                 │     │   직렬화        │
└─────────────────┘     └─────────────────┘
        ▲                       ▲
        │                       │
        └───────────┬───────────┘
                    │ 기반으로 구축
        ┌───────────┴───────────┬───────────────┐
        │                       │               │
계층 2: 서비스 시스템 (계층 0-1에 의존)
┌─────────────────┐ ┌─────────────────┐ ┌─────────────────┐
│  logger_system  │ │monitoring_system│ │ database_system │
│                 │ │     (Rust)      │ │                 │
│ • ILogger       │ │ • 이벤트 버스   │ │ • thread_       │
│   구현          │ │ • 메트릭        │ │   system을      │
│ • 비동기 로깅   │ │ • 상태 확인     │ │   사용하여      │
│ • 스레드 안전   │ │ • Circuit       │ │   비동기 쿼리   │
│   싱크          │ │   breakers      │ │ • 에러 처리를   │
│                 │ │                 │ │   위한 Result<T>│
└─────────────────┘ └─────────────────┘ └─────────────────┘
                    ▲
                    │ 기반으로 구축
                    │
계층 3: 통합 시스템 (계층 0-2에 의존)
        ┌───────────────────────┐
        │   network_system      │
        │                       │
        │ • TCP/IP 통신         │
        │ • thread_system의     │
        │   IExecutor 사용      │
        │ • logger_system의     │
        │   ILogger 사용        │
        └───────────────────────┘
```

### 계층 기반 초기화 순서

**규칙**: **오름차순 계층 순서**로 시스템을 초기화하며, 각 계층 내의 의존성을 준수합니다.

#### Phase 1: 기반 (계층 0)
```
1. common_system (헤더 전용, 초기화 불필요)
```

#### Phase 2: 핵심 시스템 (계층 1)
```
2. thread_system    }
3. container_system } → 병렬 초기화 가능 (서로 간 의존성 없음)
```

#### Phase 3: 서비스 시스템 (계층 2)
```
4. logger_system      }
5. monitoring_system  } → thread_system 준비 후 병렬 초기화 가능
6. database_system    }
```

#### Phase 4: 통합 시스템 (계층 3)
```
7. network_system (thread_system + logger_system 필요)
```

**종료 순서**: 초기화의 **역순** (계층 3 → 계층 2 → 계층 1).

#### 역순 종료 이유

역순 의존성 순서로 종료하면 다음이 보장됩니다:
1. **활성 연결이 먼저 닫힘**: network_system이 thread_system보다 먼저 종료
2. **로깅이 계속 사용 가능**: logger_system은 상위 계층이 정리를 완료할 때까지 유지
3. **매달린 참조 없음**: 상위 계층 시스템이 할당 해제된 하위 계층 리소스에 접근하지 않음

### 의존성 설명

#### 왜 이런 계층 구조인가?

| 의존성 | 이유 | 고려된 대안 |
|--------|------|-------------|
| thread_system → common_system | `IExecutor` 인터페이스 필요 | ❌ executor 직접 내장 → 강한 결합 |
| logger_system → thread_system | 비동기 로깅에 작업 큐 필요 | ❌ 동기 로깅만 → 성능 저하 |
| database_system → thread_system | 블로킹 없는 비동기 쿼리 | ❌ 블로킹 쿼리 → 확장성 낮음 |
| network_system → logger_system | 네트워크 이벤트에 로깅 필요 | ❌ 커스텀 로깅 → 중복 노력 |
| monitoring_system → event bus | 분리된 메트릭 수집 | ❌ 직접 결합 → 확장 어려움 |

#### 순환 의존성 완화

**잠재적 순환**: common_system ↔ monitoring_system

**해결책**: `event_bus.h`의 조건부 컴파일:
```cpp
#if defined(ENABLE_MONITORING_INTEGRATION)
    #include <kcenon/monitoring/core/event_bus.h>
    using event_bus = monitoring_system::event_bus;
#else
    // 스텁 구현 제공
    class null_event_bus { /* no-op */ };
    using event_bus = null_event_bus;
#endif
```

**결과**: common_system은 monitoring_system을 하드 의존성 없이 빌드할 수 있습니다.

---

## 2. 통합 패턴

### Unified Bootstrapper 사용

**unified_bootstrapper**는 최소한의 보일러플레이트로 전체 생태계를 초기화하고 종료하는 고수준 API를 제공합니다.

#### 기본 예제

```cpp
#include <kcenon/common/di/unified_bootstrapper.h>
#include <iostream>

using namespace kcenon::common::di;

int main() {
    // 활성화할 시스템 구성
    bootstrapper_options opts;
    opts.enable_logging = true;
    opts.enable_monitoring = true;
    opts.enable_database = false;  // 이 앱에는 불필요
    opts.enable_network = false;
    opts.config_path = "config.yaml";
    opts.shutdown_timeout = std::chrono::seconds(30);

    // 활성화된 모든 시스템 초기화
    auto result = unified_bootstrapper::initialize(opts);
    if (result.is_err()) {
        std::cerr << "초기화 실패: "
                  << result.error().message << "\n";
        return 1;
    }

    // 서비스 컨테이너 가져오기
    auto& services = unified_bootstrapper::services();

    // 서비스 해결
    auto logger = services.resolve<interfaces::ILogger>();
    logger->log(log_level::info, "애플리케이션 시작");

    // 애플리케이션 로직...

    // 우아한 종료 (시그널 핸들러도 이를 트리거함)
    unified_bootstrapper::shutdown();
    return 0;
}
```

#### 초기화 시퀀스

부트스트래퍼는 내부적으로 다음 시퀀스를 따릅니다:

```
1. 설정 파일 파싱 (제공된 경우)
2. 핵심 서비스 등록 (항상 활성화됨)
   → service_container
   → ErrorInfo factory
3. 옵션에 따라 선택적 서비스 등록
   → logger_system (enable_logging = true인 경우)
   → monitoring_system (enable_monitoring = true인 경우)
   → database_system (enable_database = true인 경우)
   → network_system (enable_network = true인 경우)
4. 서비스 간 의존성 해결
   → logger를 thread pool에 연결
   → database를 executor에 연결
5. 시그널 핸들러 등록 (SIGTERM, SIGINT)
6. 초기화 훅 호출
7. VoidResult 반환 (ok 또는 error)
```

#### 고급: 종료 훅

종료 중 실행할 커스텀 정리 로직 등록:

```cpp
unified_bootstrapper::register_shutdown_hook("my_cleanup",
    [](std::chrono::milliseconds remaining_timeout) {
        // 커스텀 정리 로직
        std::cout << "남은 시간 "
                  << remaining_timeout.count() << "ms로 리소스 정리 중\n";

        // 타임아웃 전 정리 완료 보장
        my_resource_manager.close_all();
    });
```

**훅 실행 순서**: 등록의 역순 (LIFO).

#### 에러 처리

```cpp
auto result = unified_bootstrapper::initialize(opts);
if (result.is_err()) {
    switch (result.error().code) {
        case error_codes::ALREADY_EXISTS:
            // 다른 옵션으로 이미 초기화됨
            std::cerr << "이미 초기화됨\n";
            break;
        case error_codes::INTERNAL_ERROR:
            // 서비스 등록 실패
            std::cerr << "서비스 등록 에러: "
                      << result.error().message << "\n";
            break;
        default:
            std::cerr << "알 수 없는 에러\n";
    }
    return 1;
}
```

### 수동 Adapter 연결

세밀한 제어를 위해 **어댑터**와 **서비스 컨테이너**를 사용하여 시스템을 수동으로 연결할 수 있습니다.

#### 패턴 1: Interface Adapter

**사용 사례**: 구체적인 구현을 인터페이스에 적응시킵니다.

```cpp
#include <kcenon/common/adapters/adapter.h>
#include <kcenon/common/di/service_container.h>

using namespace kcenon::common;

// 커스텀 logger 구현
class MyCustomLogger : public interfaces::ILogger {
public:
    void log(log_level level, const std::string& message) override {
        // 커스텀 로깅 로직
    }
};

// 수동 연결
di::service_container container;

// 커스텀 logger 등록
auto logger_impl = std::make_shared<MyCustomLogger>();
auto logger_adapter = adapters::interface_adapter<interfaces::ILogger, MyCustomLogger>(logger_impl);

container.register_instance<interfaces::ILogger>(
    std::make_shared<decltype(logger_adapter)>(logger_adapter)
);

// 앱 어디서나 해결
auto logger = container.resolve<interfaces::ILogger>();
logger->log(log_level::info, "수동 어댑터 사용");
```

#### 패턴 2: Cross-System Adapter

**사용 사례**: 다른 인터페이스를 가진 두 시스템을 연결합니다.

```cpp
// 예제: database executor를 thread pool에 적응
#include <kcenon/thread/core/thread_pool.h>
#include <kcenon/database/core/async_executor.h>

class DatabaseExecutorAdapter : public interfaces::IExecutor {
    std::shared_ptr<database::async_executor> db_exec_;

public:
    explicit DatabaseExecutorAdapter(std::shared_ptr<database::async_executor> exec)
        : db_exec_(exec) {}

    void submit(std::function<void()> task) override {
        db_exec_->enqueue_query([task]() {
            task();
            return database::query_result{};  // 서명 적응
        });
    }
};

// 적응된 executor 등록
auto db_exec = std::make_shared<database::async_executor>(pool_size);
auto adapted = std::make_shared<DatabaseExecutorAdapter>(db_exec);
container.register_instance<interfaces::IExecutor>(adapted);
```

#### 패턴 3: Smart Adapter (Deprecated)

**레거시 코드**: `typed_adapter`와 `smart_adapter`는 **deprecated**입니다.

**마이그레이션 경로**:
```cpp
// 구버전 (deprecated)
auto old_adapter = typed_adapter<ILogger, MyLogger>(impl);

// 신버전 (권장)
auto new_adapter = interface_adapter<ILogger, MyLogger>(impl);
```

**Deprecation 이유**: `interface_adapter`는 더 나은 타입 안전성을 제공하고 RTTI 의존성을 제거합니다.

### Service Container 통합

**service_container**는 모든 시스템이 사용하는 중앙 DI 컨테이너입니다.

#### 라이프사이클

```cpp
#include <kcenon/common/di/service_container.h>

using namespace kcenon::common::di;

// 1. 컨테이너 생성
service_container container;

// 2. 서비스 등록
//    - Singleton: 전역으로 공유되는 하나의 인스턴스
container.register_singleton<ILogger>([]() {
    return std::make_shared<ConsoleLogger>();
});

//    - Transient: resolve마다 새 인스턴스
container.register_transient<IExecutor>([]() {
    return std::make_shared<ThreadPool>(4);
});

//    - Instance: 기존 인스턴스
auto existing_logger = std::make_shared<FileLogger>("app.log");
container.register_instance<ILogger>(existing_logger);

// 3. 서비스 해결
auto logger = container.resolve<ILogger>();
auto executor = container.resolve<IExecutor>();

// 4. 모든 등록 지우기 (종료 중)
container.clear();
```

#### Dependency Injection

**생성자 주입** (권장):

```cpp
class MyService {
    std::shared_ptr<interfaces::ILogger> logger_;
    std::shared_ptr<interfaces::IExecutor> executor_;

public:
    MyService(std::shared_ptr<interfaces::ILogger> logger,
              std::shared_ptr<interfaces::IExecutor> executor)
        : logger_(logger), executor_(executor) {}

    void do_work() {
        logger_->log(log_level::info, "작업 시작");
        executor_->submit([this]() {
            // 비동기 작업
        });
    }
};

// 팩토리 함수로 등록
container.register_singleton<MyService>([&container]() {
    return std::make_shared<MyService>(
        container.resolve<interfaces::ILogger>(),
        container.resolve<interfaces::IExecutor>()
    );
});
```

**Property 주입** (fallback):

```cpp
class LegacyService {
public:
    void set_logger(std::shared_ptr<interfaces::ILogger> logger) {
        logger_ = logger;
    }

private:
    std::shared_ptr<interfaces::ILogger> logger_;
};

// 생성 후 연결
auto service = std::make_shared<LegacyService>();
service->set_logger(container.resolve<interfaces::ILogger>());
```

### 라이프사이클 관리

#### 애플리케이션 라이프사이클 단계

```
┌─────────────────────────────────────────────────┐
│ 단계 1: 설정 로딩                              │
│  → YAML/JSON config 파싱                      │
│  → 설정 검증                                   │
│  → 환경 변수 설정                              │
└─────────────────────────────────────────────────┘
                    ↓
┌─────────────────────────────────────────────────┐
│ 단계 2: 부트스트래핑                           │
│  → unified_bootstrapper::initialize()          │
│  → 서비스 등록                                 │
│  → 의존성 해결                                 │
└─────────────────────────────────────────────────┘
                    ↓
┌─────────────────────────────────────────────────┐
│ 단계 3: 실행                                   │
│  → 애플리케이션 로직 실행                      │
│  → 서비스가 인터페이스를 통해 상호작용         │
│  → 이벤트 버스가 크로스 시스템 메시지 처리     │
└─────────────────────────────────────────────────┘
                    ↓
┌─────────────────────────────────────────────────┐
│ 단계 4: 우아한 종료                            │
│  → 시그널 수신 (SIGTERM/SIGINT)               │
│  → unified_bootstrapper::shutdown()            │
│  → 종료 훅 호출 (역순)                        │
│  → 컨테이너에서 서비스 지우기                  │
└─────────────────────────────────────────────────┘
```

#### 완전한 라이프사이클 예제

```cpp
#include <kcenon/common/di/unified_bootstrapper.h>
#include <kcenon/common/config/config_reader.h>
#include <csignal>
#include <iostream>

using namespace kcenon::common;

int main(int argc, char** argv) {
    // 단계 1: 설정 로딩
    auto config_result = config::read_yaml("config.yaml");
    if (config_result.is_err()) {
        std::cerr << "Config 에러: " << config_result.error().message << "\n";
        return 1;
    }
    auto cfg = config_result.value();

    // 단계 2: 부트스트랩
    di::bootstrapper_options opts;
    opts.enable_logging = cfg.get_bool("logging.enabled", true);
    opts.enable_monitoring = cfg.get_bool("monitoring.enabled", true);
    opts.enable_database = cfg.get_bool("database.enabled", false);
    opts.enable_network = cfg.get_bool("network.enabled", false);
    opts.shutdown_timeout = std::chrono::milliseconds(
        cfg.get_int("shutdown_timeout_ms", 30000)
    );

    auto init_result = di::unified_bootstrapper::initialize(opts);
    if (init_result.is_err()) {
        std::cerr << "초기화 에러: " << init_result.error().message << "\n";
        return 1;
    }

    // 커스텀 종료 훅 등록
    di::unified_bootstrapper::register_shutdown_hook("app_cleanup",
        [](std::chrono::milliseconds timeout) {
            std::cout << "앱 종료 중, 타임아웃 "
                      << timeout.count() << "ms\n";
            // 여기에 커스텀 정리 로직
        });

    // 단계 3: 애플리케이션 실행
    auto& services = di::unified_bootstrapper::services();
    auto logger = services.resolve<interfaces::ILogger>();
    logger->log(log_level::info, "애플리케이션 시작");

    // 애플리케이션 로직...
    std::cout << "Ctrl+C를 눌러 종료하세요\n";

    // 시그널 대기 (시그널 핸들러는 bootstrapper가 자동 등록)
    while (!di::unified_bootstrapper::is_shutdown_requested()) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    // 단계 4: 우아한 종료
    // (시그널 핸들러가 이미 shutdown을 트리거했지만 명시적으로도 호출 가능)
    auto shutdown_result = di::unified_bootstrapper::shutdown(
        std::chrono::seconds(30)
    );

    if (shutdown_result.is_err()) {
        std::cerr << "종료 에러: " << shutdown_result.error().message << "\n";
        return 1;
    }

    logger->log(log_level::info, "애플리케이션 중지");
    return 0;
}
```

---

## 3. 일반적인 통합 시나리오

이 섹션은 여러 kcenon 시스템을 함께 사용하는 실제 통합 패턴을 보여줍니다.

### 시나리오 1: Web API 서버

**사용 시스템**: common_system + thread_system + logger_system + network_system

**사용 사례**: 비동기 요청 처리와 구조화된 로깅을 갖춘 HTTP REST API 서버.

#### 필요한 시스템

| 시스템 | 역할 | 사용되는 주요 기능 |
|--------|------|-------------------|
| common_system | 기반 | Result<T>, ILogger, IExecutor 인터페이스 |
| thread_system | 요청 처리 | 비동기 요청 처리를 위한 스레드 풀 |
| logger_system | 관찰 가능성 | 구조화된 로깅, 비동기 파일 싱크 |
| network_system | 통신 | HTTP 서버, 라우팅, 미들웨어 |

#### 코드 예제

```cpp
#include <kcenon/common/di/unified_bootstrapper.h>
#include <kcenon/network/http/http_server.h>
#include <kcenon/thread/core/thread_pool.h>
#include <kcenon/logger/core/logger.h>

using namespace kcenon;

int main() {
    // 시스템 초기화
    common::di::bootstrapper_options opts;
    opts.enable_logging = true;
    opts.enable_network = true;
    opts.config_path = "api_server.yaml";

    auto init_result = common::di::unified_bootstrapper::initialize(opts);
    if (init_result.is_err()) {
        std::cerr << "초기화 실패\n";
        return 1;
    }

    auto& services = common::di::unified_bootstrapper::services();
    auto logger = services.resolve<common::interfaces::ILogger>();
    auto executor = services.resolve<common::interfaces::IExecutor>();

    // HTTP 서버 생성
    network::http::server_config config;
    config.port = 8080;
    config.thread_pool_size = 8;

    network::http::http_server server(config, logger, executor);

    // 라우트 등록
    server.route("GET", "/api/users", [logger](const auto& req) {
        logger->log(common::log_level::info, "GET /api/users");

        // 비동기 데이터베이스 쿼리가 여기에 들어감
        return network::http::response{
            .status = 200,
            .body = R"({"users": [{"id": 1, "name": "Alice"}]})",
            .content_type = "application/json"
        };
    });

    server.route("POST", "/api/users", [logger, executor](const auto& req) {
        logger->log(common::log_level::info, "POST /api/users");

        // 스레드 풀에서 비동기 처리
        executor->submit([logger, body = req.body]() {
            // 사용자 생성 처리
            logger->log(common::log_level::debug, "사용자 생성 처리 중");
        });

        return network::http::response{
            .status = 201,
            .body = R"({"id": 2, "name": "Bob"})",
            .content_type = "application/json"
        };
    });

    // 서버 시작
    logger->log(common::log_level::info, "포트 8080에서 서버 시작");
    server.start();

    // 종료 시그널 대기
    while (!common::di::unified_bootstrapper::is_shutdown_requested()) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    server.stop();
    common::di::unified_bootstrapper::shutdown();
    return 0;
}
```

#### 설정

```yaml
# api_server.yaml
common:
  log_level: info

logging:
  enabled: true
  async: true
  sinks:
    - type: console
      level: info
    - type: file
      path: logs/api_server.log
      level: debug
      rotation:
        max_size_mb: 100
        max_files: 10

thread:
  pool_size: 8
  queue_capacity: 1000

network:
  http:
    port: 8080
    max_connections: 1000
    request_timeout_sec: 30
    keep_alive: true
```

#### 일반적인 함정

| 함정 | 해결책 |
|------|--------|
| **요청 핸들러에서 블로킹 I/O** | 긴 작업에는 executor->submit() 사용 |
| **잠금 없는 공유 상태** | std::mutex 또는 atomic 타입 사용 |
| **람다 캡처에서 메모리 누수** | 순환 참조에 weak_ptr 사용 |
| **무제한 요청 큐** | 스레드 풀 config에서 queue_capacity 설정 |

---

### 시나리오 2: 데이터 파이프라인

**사용 시스템**: common_system + thread_system + database_system + container_system

**사용 사례**: 직렬화와 함께 데이터베이스에서 데이터를 처리하는 ETL 파이프라인.

#### 필요한 시스템

| 시스템 | 역할 | 사용되는 주요 기능 |
|--------|------|-------------------|
| common_system | 기반 | 에러 처리를 위한 Result<T> |
| thread_system | 병렬 처리 | 배치 처리를 위한 스레드 풀 |
| database_system | 데이터 소스 | 비동기 쿼리, 연결 풀링 |
| container_system | 직렬화 | JSON/binary 직렬화, 타입 컨테이너 |

#### 코드 예제

```cpp
#include <kcenon/common/di/unified_bootstrapper.h>
#include <kcenon/database/core/database.h>
#include <kcenon/thread/core/thread_pool.h>
#include <kcenon/container/serialization/json_serializer.h>

using namespace kcenon;

struct UserRecord {
    int id;
    std::string name;
    std::string email;
};

int main() {
    // 시스템 초기화
    common::di::bootstrapper_options opts;
    opts.enable_database = true;
    opts.config_path = "pipeline.yaml";

    auto init_result = common::di::unified_bootstrapper::initialize(opts);
    if (init_result.is_err()) {
        std::cerr << "초기화 실패: " << init_result.error().message << "\n";
        return 1;
    }

    auto& services = common::di::unified_bootstrapper::services();
    auto executor = services.resolve<common::interfaces::IExecutor>();

    // 데이터베이스 연결 설정
    database::connection_config db_config;
    db_config.type = database::db_type::sqlite;
    db_config.path = "data.db";
    db_config.pool_size = 4;

    auto db_result = database::database::connect(db_config);
    if (db_result.is_err()) {
        std::cerr << "DB 연결 실패\n";
        return 1;
    }
    auto db = db_result.value();

    // ETL 파이프라인: Extract -> Transform -> Load

    // 1. Extract: 배치로 데이터 쿼리
    std::string query = "SELECT id, name, email FROM users LIMIT 1000";
    auto rows_result = db->query(query);

    if (rows_result.is_err()) {
        std::cerr << "쿼리 실패\n";
        return 1;
    }
    auto rows = rows_result.value();

    // 2. Transform: 병렬 배치로 처리
    const size_t batch_size = 100;
    std::vector<std::future<std::vector<UserRecord>>> futures;

    for (size_t i = 0; i < rows.size(); i += batch_size) {
        size_t end = std::min(i + batch_size, rows.size());

        auto future = std::async(std::launch::async, [&rows, i, end]() {
            std::vector<UserRecord> batch;
            for (size_t j = i; j < end; ++j) {
                UserRecord record;
                record.id = rows[j].get<int>("id");
                record.name = rows[j].get<std::string>("name");
                record.email = rows[j].get<std::string>("email");

                // Transform: 이메일을 소문자로 정규화
                std::transform(record.email.begin(), record.email.end(),
                             record.email.begin(), ::tolower);

                batch.push_back(record);
            }
            return batch;
        });

        futures.push_back(std::move(future));
    }

    // 변환된 배치 수집
    std::vector<UserRecord> transformed_records;
    for (auto& future : futures) {
        auto batch = future.get();
        transformed_records.insert(transformed_records.end(),
                                  batch.begin(), batch.end());
    }

    // 3. Load: JSON으로 직렬화하고 저장
    container::json_serializer serializer;
    auto json_result = serializer.serialize(transformed_records);

    if (json_result.is_ok()) {
        std::ofstream outfile("output.json");
        outfile << json_result.value();
        std::cout << "파이프라인 완료: " << transformed_records.size()
                  << "개 레코드 처리됨\n";
    }

    common::di::unified_bootstrapper::shutdown();
    return 0;
}
```

#### 설정

```yaml
# pipeline.yaml
common:
  log_level: info

database:
  type: sqlite
  path: data.db
  pool_size: 4
  timeout_sec: 10

thread:
  pool_size: 4

container:
  json:
    pretty_print: true
    indent_size: 2
```

#### 일반적인 함정

| 함정 | 해결책 |
|------|--------|
| **트랜잭션 범위가 너무 큼** | 모두 한 번에가 아닌 배치로 커밋 |
| **대용량 데이터셋에서 메모리 고갈** | 모든 데이터 로딩 대신 스트림 처리 |
| **병렬 쓰기에서 데드락** | 단일 스레드 쓰기 또는 행 수준 잠금 사용 |
| **직렬화 병목** | 배치별로 직렬화 병렬화 |

---

### 시나리오 3: 모니터링 스택

**사용 시스템**: common_system + thread_system + monitoring_system + logger_system

**사용 사례**: 메트릭 수집 및 상태 확인을 갖춘 실시간 애플리케이션 모니터링.

#### 필요한 시스템

| 시스템 | 역할 | 사용되는 주요 기능 |
|--------|------|-------------------|
| common_system | 기반 | 이벤트 버스, IMonitor 인터페이스 |
| thread_system | 백그라운드 작업 | 예약된 메트릭 수집 |
| monitoring_system | 메트릭 | Prometheus 메트릭, 상태 확인, circuit breakers |
| logger_system | 알림 | 알림을 위한 구조화된 로깅 |

#### 코드 예제

```cpp
#include <kcenon/common/di/unified_bootstrapper.h>
#include <kcenon/monitoring/core/metrics.h>
#include <kcenon/monitoring/health/health_checker.h>
#include <kcenon/thread/core/thread_pool.h>

using namespace kcenon;

int main() {
    // 시스템 초기화
    common::di::bootstrapper_options opts;
    opts.enable_logging = true;
    opts.enable_monitoring = true;
    opts.config_path = "monitoring.yaml";

    auto init_result = common::di::unified_bootstrapper::initialize(opts);
    if (init_result.is_err()) {
        return 1;
    }

    auto& services = common::di::unified_bootstrapper::services();
    auto logger = services.resolve<common::interfaces::ILogger>();
    auto executor = services.resolve<common::interfaces::IExecutor>();

    // 메트릭 설정
    monitoring::metrics_registry metrics;

    auto request_counter = metrics.create_counter(
        "http_requests_total",
        "총 HTTP 요청 수",
        {"method", "endpoint", "status"}
    );

    auto request_duration = metrics.create_histogram(
        "http_request_duration_seconds",
        "HTTP 요청 지연 시간",
        {0.001, 0.01, 0.1, 0.5, 1.0, 5.0}  // 버킷
    );

    auto active_connections = metrics.create_gauge(
        "active_connections",
        "활성 연결 수"
    );

    // 상태 확인 설정
    monitoring::health_checker health;

    health.register_check("database", []() -> monitoring::health_status {
        // 데이터베이스 연결 확인
        // 반환: monitoring::health_status::healthy 또는 ::unhealthy
        return monitoring::health_status::healthy;
    });

    health.register_check("cache", []() -> monitoring::health_status {
        // 캐시 가용성 확인
        return monitoring::health_status::healthy;
    });

    // 주기적 메트릭 수집 (10초마다)
    executor->submit_periodic(std::chrono::seconds(10), [&metrics, logger]() {
        // 시스템 메트릭 수집
        auto cpu_usage = get_cpu_usage();  // 가상의 함수
        auto memory_usage = get_memory_usage();

        metrics.create_gauge("system_cpu_usage", "CPU 사용률")->set(cpu_usage);
        metrics.create_gauge("system_memory_usage", "메모리 사용률")->set(memory_usage);

        logger->log(common::log_level::debug,
                   "메트릭 수집됨 - CPU: " + std::to_string(cpu_usage) + "%");
    });

    // 애플리케이션 워크로드 시뮬레이션
    for (int i = 0; i < 100; ++i) {
        auto start = std::chrono::steady_clock::now();

        // HTTP 요청 처리 시뮬레이션
        active_connections->increment();

        std::this_thread::sleep_for(std::chrono::milliseconds(50));

        active_connections->decrement();

        auto duration = std::chrono::duration<double>(
            std::chrono::steady_clock::now() - start
        ).count();

        request_duration->observe(duration);
        request_counter->inc({"GET", "/api/users", "200"});
    }

    // Prometheus 형식으로 메트릭 내보내기
    std::cout << "=== 메트릭 내보내기 ===\n";
    std::cout << metrics.export_prometheus() << "\n";

    // 상태 확인 실행
    auto health_report = health.check_all();
    std::cout << "=== 상태 ===\n";
    std::cout << "전체: " << (health_report.is_healthy() ? "정상" : "비정상") << "\n";

    common::di::unified_bootstrapper::shutdown();
    return 0;
}
```

#### 설정

```yaml
# monitoring.yaml
common:
  log_level: info

monitoring:
  enabled: true
  metrics:
    export_port: 9090
    export_path: /metrics
  health:
    check_interval_sec: 30
  circuit_breaker:
    failure_threshold: 5
    timeout_sec: 60

logging:
  enabled: true
  sinks:
    - type: console
      level: info
```

#### 일반적인 함정

| 함정 | 해결책 |
|------|--------|
| **메트릭 카디널리티 폭발** | 레이블 값 제한, 레이블 가이드라인 사용 |
| **상태 확인 false positive** | 유예 기간, 재시도 로직 추가 |
| **블로킹 상태 확인** | 타임아웃과 함께 비동기로 확인 실행 |
| **에러 시 메트릭 누락** | finally 블록에서 항상 메트릭 기록 |

---

### 시나리오 4: Full Stack 애플리케이션

**사용 시스템**: 모든 7개 시스템 (common + thread + container + logger + monitoring + database + network)

**사용 사례**: 완전한 관찰 가능성과 영속성을 갖춘 프로덕션 준비 웹 애플리케이션.

#### 필요한 시스템

| 시스템 | 역할 |
|--------|------|
| common_system | 모든 인터페이스의 기반 |
| thread_system | 비동기 요청 처리, 백그라운드 작업 |
| container_system | 설정 파싱, 데이터 직렬화 |
| logger_system | 애플리케이션 로깅, 감사 추적 |
| monitoring_system | 메트릭, 상태 확인, circuit breakers |
| database_system | 사용자 데이터 영속성 |
| network_system | HTTP API, WebSocket 연결 |

#### 코드 예제 (간소화)

```cpp
#include <kcenon/common/di/unified_bootstrapper.h>
// 모든 시스템 헤더 포함...

using namespace kcenon;

class FullStackApp {
    std::shared_ptr<common::interfaces::ILogger> logger_;
    std::shared_ptr<network::http::http_server> server_;
    std::shared_ptr<database::database> db_;
    std::shared_ptr<monitoring::metrics_registry> metrics_;

public:
    FullStackApp(/* 의존성 주입 */) {
        // 모든 서비스의 생성자 주입
    }

    void start() {
        logger_->log(common::log_level::info, "풀스택 애플리케이션 시작");

        // 라우트 설정
        server_->route("GET", "/api/health", [this](const auto& req) {
            return this->handle_health_check(req);
        });

        server_->route("POST", "/api/data", [this](const auto& req) {
            return this->handle_data_submission(req);
        });

        server_->start();
    }

private:
    network::http::response handle_health_check(const auto& req) {
        // monitoring_system을 사용하여 상태 확인
        auto status = monitoring::health_checker::check_all();

        metrics_->create_counter("health_checks_total", "")->inc();

        return network::http::response{
            .status = status.is_healthy() ? 200 : 503,
            .body = status.to_json(),
            .content_type = "application/json"
        };
    }

    network::http::response handle_data_submission(const auto& req) {
        // container_system을 사용하여 요청 파싱
        auto data_result = container::json_serializer::parse(req.body);

        if (data_result.is_err()) {
            logger_->log(common::log_level::warn, "잘못된 JSON");
            metrics_->create_counter("invalid_requests_total", "")->inc();
            return network::http::response{.status = 400};
        }

        // database_system에 저장 (thread_system을 통한 비동기)
        auto insert_result = db_->execute(
            "INSERT INTO data (payload) VALUES (?)",
            {data_result.value()}
        );

        if (insert_result.is_ok()) {
            logger_->log(common::log_level::info, "데이터 저장 성공");
            metrics_->create_counter("data_stored_total", "")->inc();
            return network::http::response{.status = 201};
        } else {
            logger_->log(common::log_level::error,
                        "데이터베이스 에러: " + insert_result.error().message);
            return network::http::response{.status = 500};
        }
    }
};

int main() {
    // 모든 시스템 초기화
    common::di::bootstrapper_options opts;
    opts.enable_logging = true;
    opts.enable_monitoring = true;
    opts.enable_database = true;
    opts.enable_network = true;
    opts.config_path = "fullstack.yaml";

    auto init_result = common::di::unified_bootstrapper::initialize(opts);
    if (init_result.is_err()) {
        return 1;
    }

    auto& services = common::di::unified_bootstrapper::services();

    // 모든 서비스 해결하고 함께 연결
    FullStackApp app(/* 서비스 전달 */);
    app.start();

    // 종료 대기
    while (!common::di::unified_bootstrapper::is_shutdown_requested()) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    common::di::unified_bootstrapper::shutdown();
    return 0;
}
```

#### 설정

```yaml
# fullstack.yaml
common:
  log_level: info
  shutdown_timeout_ms: 30000

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
  pool_size: 16

database:
  type: sqlite
  path: app.db
  pool_size: 10

network:
  http:
    port: 8080
    max_connections: 1000

monitoring:
  enabled: true
  metrics:
    export_port: 9090
  health:
    check_interval_sec: 30
```

#### 일반적인 함정

| 함정 | 해결책 |
|------|--------|
| **초기화 순서 에러** | 올바른 순서를 위해 unified_bootstrapper 사용 |
| **서비스 수명 불일치** | 모든 서비스에 shared_ptr 사용, 원시 포인터 피함 |
| **설정 드리프트** | 시작 시 config 검증, 빠른 실패 |
| **우아한 종료 실패** | 각 시스템에 대한 종료 훅 등록 |

---

## 4. 시스템 간 설정

### 통합 설정 전략

여러 kcenon 시스템을 통합할 때 통합 설정 접근 방식은 중복을 방지하고 일관성을 보장합니다.

#### 설계 원칙

1. **단일 진실 공급원**: 배포 환경당 하나의 설정 파일
2. **계층적 구조**: 최상위 수준의 공통 설정, 시스템별 중첩
3. **환경 재정의**: dev/staging/production 변형 지원
4. **로드 시 검증**: 설정이 잘못된 경우 빠르게 실패
5. **합리적인 기본값**: 필요한 설정 최소화

### 설정 구조

#### 최상위 스키마

```yaml
# kcenon 생태계의 표준 설정 구조

common:                    # 모든 시스템에서 공유되는 설정
  log_level: info         # 전역 로그 레벨
  shutdown_timeout_ms: 30000
  environment: production  # dev | staging | production

logging:                   # logger_system 설정
  enabled: true
  async: true
  sinks:
    - type: console
      level: info
    - type: file
      path: logs/app.log
      level: debug
      rotation:
        max_size_mb: 100
        max_files: 10

thread:                    # thread_system 설정
  pool_size: 8             # 워커 스레드 수
  queue_capacity: 1000     # 최대 큐 작업
  autoscale:
    enabled: false
    min_threads: 4
    max_threads: 32

database:                  # database_system 설정
  type: sqlite             # sqlite | postgres | mysql
  host: localhost          # 원격 데이터베이스용
  port: 5432
  path: data.db            # SQLite용
  pool_size: 10
  timeout_sec: 10

network:                   # network_system 설정
  http:
    port: 8080
    host: 0.0.0.0
    max_connections: 1000
    request_timeout_sec: 30
    keep_alive: true

monitoring:                # monitoring_system 설정
  enabled: true
  metrics:
    export_port: 9090
    export_path: /metrics
  health:
    check_interval_sec: 30

container:                 # container_system 설정
  json:
    pretty_print: false
    indent_size: 2
  binary:
    compression: true
```

#### 공유 설정

여러 시스템에 적용되는 설정:

| 설정 | 영향받는 시스템 | 목적 |
|------|----------------|------|
| `common.log_level` | 모든 시스템 | 기본 로그 상세도 |
| `common.shutdown_timeout_ms` | 모든 시스템 | 우아한 종료를 위한 최대 시간 |
| `common.environment` | 모든 시스템 | 배포 환경 식별자 |
| `thread.pool_size` | thread, database, network | 워커 스레드 수 |

#### 시스템별 재정의

시스템은 공통 설정을 재정의할 수 있습니다:

```yaml
common:
  log_level: info  # 모든 시스템의 기본값

logging:
  level: debug     # logger_system만 재정의

database:
  log_level: warn  # database_system만 재정의
```

### 시나리오별 설정 예제

#### Web API 서버 설정

```yaml
common:
  log_level: info
  environment: production

logging:
  enabled: true
  async: true
  sinks:
    - type: console
      level: info
    - type: file
      path: logs/api.log
      level: debug

thread:
  pool_size: 16            # 동시 요청 처리
  queue_capacity: 5000

network:
  http:
    port: 8080
    max_connections: 2000  # 높은 동시성
    request_timeout_sec: 30

monitoring:
  enabled: true
  metrics:
    export_port: 9090
```

#### 데이터 파이프라인 설정

```yaml
common:
  log_level: debug         # 디버깅을 위한 상세 로깅

database:
  type: postgres
  host: db.example.com
  port: 5432
  pool_size: 20            # ETL을 위한 높은 병렬성
  timeout_sec: 60          # 장기 실행 쿼리

thread:
  pool_size: 32            # 병렬 배치 처리

container:
  json:
    pretty_print: true     # 사람이 읽을 수 있는 출력 파일
```

#### 모니터링 스택 설정

```yaml
common:
  log_level: info

logging:
  enabled: true
  sinks:
    - type: console
      level: info

monitoring:
  enabled: true
  metrics:
    export_port: 9090
    export_path: /metrics
  health:
    check_interval_sec: 10  # 빈번한 확인
  circuit_breaker:
    failure_threshold: 5
    timeout_sec: 60

thread:
  pool_size: 4             # 가벼운 워크로드
```

### 환경 변수 매핑

환경 변수로 설정 재정의:

| 환경 변수 | 설정 경로 | 예제 |
|-----------|----------|------|
| `KCENON_LOG_LEVEL` | `common.log_level` | `export KCENON_LOG_LEVEL=debug` |
| `KCENON_HTTP_PORT` | `network.http.port` | `export KCENON_HTTP_PORT=3000` |
| `KCENON_DB_HOST` | `database.host` | `export KCENON_DB_HOST=prod-db.internal` |
| `KCENON_THREAD_POOL_SIZE` | `thread.pool_size` | `export KCENON_THREAD_POOL_SIZE=32` |

**우선순위**: 환경 변수 > 설정 파일 > 기본값

### 설정 검증

```cpp
#include <kcenon/common/config/config_reader.h>

auto config_result = common::config::read_yaml("app.yaml");
if (config_result.is_err()) {
    std::cerr << "Config 에러: " << config_result.error().message << "\n";
    return 1;
}

auto cfg = config_result.value();

// 필수 설정 검증
if (!cfg.has("network.http.port")) {
    std::cerr << "필수 config 누락: network.http.port\n";
    return 1;
}

// 값 범위 검증
int port = cfg.get_int("network.http.port");
if (port < 1024 || port > 65535) {
    std::cerr << "잘못된 포트: " << port << " (1024-65535 사이여야 함)\n";
    return 1;
}
```

---

## 5. 시스템 간 에러 처리

### 에러 코드 범위

각 kcenon 시스템은 충돌을 방지하고 에러 소스 식별을 가능하게 하기 위해 전용 에러 코드 범위를 갖습니다.

| 시스템 | 에러 코드 범위 | 목적 |
|--------|---------------|------|
| common_system | 1000-1999 | 기반 에러 (잘못된 인수, 찾을 수 없음 등) |
| thread_system | 2000-2999 | 스레드 풀 에러 (큐 가득 참, 종료 등) |
| logger_system | 3000-3999 | 로깅 에러 (싱크 실패, 파일 I/O 등) |
| database_system | 4000-4999 | 데이터베이스 에러 (연결, 쿼리, 트랜잭션 등) |
| network_system | 5000-5999 | 네트워크 에러 (연결, 타임아웃, 프로토콜 등) |
| container_system | 6000-6999 | 컨테이너 에러 (직렬화, 검증 등) |
| monitoring_system | 7000-7999 | 모니터링 에러 (메트릭 수집, 상태 확인 등) |

#### 일반적인 에러 코드

**common_system (1000-1999)**:
```cpp
namespace kcenon::common::error_codes {
    constexpr int OK = 0;
    constexpr int UNKNOWN_ERROR = 1000;
    constexpr int INVALID_ARGUMENT = 1001;
    constexpr int NOT_FOUND = 1002;
    constexpr int ALREADY_EXISTS = 1003;
    constexpr int PERMISSION_DENIED = 1004;
    constexpr int TIMEOUT = 1005;
    constexpr int INTERNAL_ERROR = 1006;
    constexpr int NOT_INITIALIZED = 1007;
    constexpr int ALREADY_INITIALIZED = 1008;
}
```

**thread_system (2000-2999)**:
```cpp
namespace kcenon::thread::error_codes {
    constexpr int QUEUE_FULL = 2001;
    constexpr int EXECUTOR_SHUTDOWN = 2002;
    constexpr int TASK_REJECTED = 2003;
    constexpr int INVALID_THREAD_COUNT = 2004;
    constexpr int THREAD_CREATION_FAILED = 2005;
}
```

**database_system (4000-4999)**:
```cpp
namespace kcenon::database::error_codes {
    constexpr int CONNECTION_FAILED = 4001;
    constexpr int QUERY_FAILED = 4002;
    constexpr int TRANSACTION_FAILED = 4003;
    constexpr int INVALID_QUERY = 4004;
    constexpr int CONSTRAINT_VIOLATION = 4005;
    constexpr int DEADLOCK = 4006;
}
```

**network_system (5000-5999)**:
```cpp
namespace kcenon::network::error_codes {
    constexpr int CONNECTION_REFUSED = 5001;
    constexpr int CONNECTION_TIMEOUT = 5002;
    constexpr int SOCKET_ERROR = 5003;
    constexpr int INVALID_PROTOCOL = 5004;
    constexpr int REQUEST_TIMEOUT = 5005;
    constexpr int SERVER_ERROR = 5006;
}
```

### 크로스 시스템 에러 전파

에러가 시스템 경계를 넘을 때 상위 수준 시스템이 자체 에러 정보를 추가할 수 있도록 하면서 컨텍스트를 보존합니다.

#### 패턴 1: 에러 컨텍스트 보존

```cpp
#include <kcenon/common/patterns/result.h>
#include <kcenon/database/core/database.h>
#include <kcenon/network/http/http_server.h>

using namespace kcenon;

// 하위 수준 데이터베이스 에러
auto query_users() -> common::Result<std::vector<User>> {
    auto result = db->execute("SELECT * FROM users");
    if (result.is_err()) {
        // 데이터베이스 에러 (4000-4999 범위)
        return common::make_error<std::vector<User>>(
            result.error()  // 원본 에러 보존
        );
    }
    return common::ok(parse_users(result.value()));
}

// 중간 수준 서비스 에러
auto get_users_service() -> common::Result<UserList> {
    auto users_result = query_users();
    if (users_result.is_err()) {
        // DB 에러를 보존하면서 서비스 계층 컨텍스트 추가
        return common::make_error<UserList>(
            common::error_codes::INTERNAL_ERROR,
            "사용자 검색 실패: " + users_result.error().message,
            "user_service",
            users_result.error()  // 원본 에러 체인
        );
    }

    return common::ok(UserList{users_result.value()});
}

// 상위 수준 HTTP 핸들러
auto handle_get_users(const auto& req) -> network::http::response {
    auto result = get_users_service();
    if (result.is_err()) {
        const auto& err = result.error();

        // 내부 에러를 HTTP 상태 코드로 매핑
        int http_status = 500;
        if (err.code == common::error_codes::NOT_FOUND) {
            http_status = 404;
        } else if (err.code == common::error_codes::PERMISSION_DENIED) {
            http_status = 403;
        }

        // 전체 에러 체인 로깅
        logger->log(common::log_level::error,
                   "요청 실패: " + err.to_string());

        // 사용자 친화적 에러 반환
        return network::http::response{
            .status = http_status,
            .body = R"({"error": "사용자 검색 실패"})",
            .content_type = "application/json"
        };
    }

    return network::http::response{
        .status = 200,
        .body = serialize_users(result.value()),
        .content_type = "application/json"
    };
}
```

#### 패턴 2: 에러 변환

일부 시스템은 에러를 한 도메인에서 다른 도메인으로 변환해야 합니다:

```cpp
// 데이터베이스 에러를 네트워크 에러로 변환
auto save_user_http(const User& user) -> common::VoidResult {
    auto db_result = db->insert("users", user);

    if (db_result.is_err()) {
        switch (db_result.error().code) {
            case database::error_codes::CONSTRAINT_VIOLATION:
                // 공통 에러로 변환
                return common::make_error(
                    common::error_codes::ALREADY_EXISTS,
                    "사용자가 이미 존재함",
                    "http_handler"
                );

            case database::error_codes::CONNECTION_FAILED:
                // 네트워크 에러로 변환
                return common::make_error(
                    network::error_codes::SERVER_ERROR,
                    "데이터베이스 사용 불가",
                    "http_handler"
                );

            default:
                // 일반 fallback
                return common::make_error(
                    common::error_codes::INTERNAL_ERROR,
                    "사용자 저장 실패",
                    "http_handler",
                    db_result.error()  // 원본 보존
                );
        }
    }

    return common::ok();
}
```

### Result 조합 패턴

모나딕 연산을 사용하여 여러 시스템의 Result를 조합합니다.

#### 순차 조합

```cpp
auto process_order(const Order& order) -> common::Result<Receipt> {
    return validate_order(order)           // common_system
        .and_then([](const auto& validated) {
            return save_to_database(validated);  // database_system
        })
        .and_then([](const auto& saved) {
            return send_confirmation_email(saved);  // network_system
        })
        .and_then([](const auto& confirmed) {
            return generate_receipt(confirmed);
        });
}

// 사용
auto result = process_order(order);
if (result.is_err()) {
    logger->log(common::log_level::error,
               "주문 처리 실패 위치: " + result.error().source);
    return result.error();
}
```

#### 병렬 조합

```cpp
auto fetch_user_dashboard(int user_id) -> common::Result<Dashboard> {
    // 병렬 쿼리 실행
    auto user_future = std::async(std::launch::async, [=]() {
        return db->query_user(user_id);
    });

    auto orders_future = std::async(std::launch::async, [=]() {
        return db->query_orders(user_id);
    });

    auto notifications_future = std::async(std::launch::async, [=]() {
        return fetch_notifications(user_id);
    });

    // 결과 수집
    auto user_result = user_future.get();
    auto orders_result = orders_future.get();
    auto notifications_result = notifications_future.get();

    // 결과 조합 (모두 성공해야 함)
    if (user_result.is_err()) {
        return common::make_error<Dashboard>(user_result.error());
    }
    if (orders_result.is_err()) {
        return common::make_error<Dashboard>(orders_result.error());
    }
    if (notifications_result.is_err()) {
        return common::make_error<Dashboard>(notifications_result.error());
    }

    // 성공: 모든 데이터 조합
    return common::ok(Dashboard{
        .user = user_result.value(),
        .orders = orders_result.value(),
        .notifications = notifications_result.value()
    });
}
```

#### Fallback 조합

```cpp
auto get_config() -> common::Result<Config> {
    // 기본 config 소스 시도
    auto result = load_from_database();
    if (result.is_ok()) {
        return result;
    }

    logger->log(common::log_level::warn,
               "데이터베이스 config 실패, 파일 시도");

    // 파일로 fallback
    result = load_from_file("config.yaml");
    if (result.is_ok()) {
        return result;
    }

    logger->log(common::log_level::warn,
               "파일 config 실패, 기본값 사용");

    // 기본값으로 최종 fallback
    return common::ok(Config::defaults());
}
```

---

## 6. 초기화 및 종료

### 초기화 시퀀스

적절한 초기화 순서는 다중 시스템 애플리케이션에 중요합니다.

#### 단계 기반 초기화

```cpp
#include <kcenon/common/di/unified_bootstrapper.h>

int main() {
    using namespace kcenon;

    // 단계 1: 설정
    auto config_result = common::config::read_yaml("app.yaml");
    if (config_result.is_err()) {
        std::cerr << "Config 로드 실패: "
                  << config_result.error().message << "\n";
        return 1;
    }
    auto cfg = config_result.value();

    // 단계 2: 시스템 부트스트랩 (자동 순서 지정)
    common::di::bootstrapper_options opts;
    opts.enable_logging = cfg.get_bool("logging.enabled", true);
    opts.enable_monitoring = cfg.get_bool("monitoring.enabled", true);
    opts.enable_database = cfg.get_bool("database.enabled", true);
    opts.enable_network = cfg.get_bool("network.enabled", true);
    opts.config_path = "app.yaml";

    auto init_result = common::di::unified_bootstrapper::initialize(opts);
    if (init_result.is_err()) {
        std::cerr << "시스템 초기화 실패 위치: "
                  << init_result.error().source << "\n"
                  << "에러: " << init_result.error().message << "\n";
        return 1;
    }

    // 단계 3: 서비스 해결
    auto& services = common::di::unified_bootstrapper::services();
    auto logger = services.resolve<common::interfaces::ILogger>();
    auto executor = services.resolve<common::interfaces::IExecutor>();

    logger->log(common::log_level::info, "모든 시스템이 성공적으로 초기화됨");

    // 단계 4: 애플리케이션 로직
    run_application(services);

    // 단계 5: 우아한 종료
    auto shutdown_result = common::di::unified_bootstrapper::shutdown(
        std::chrono::seconds(30)
    );

    if (shutdown_result.is_err()) {
        std::cerr << "종료 실패: "
                  << shutdown_result.error().message << "\n";
        return 1;
    }

    return 0;
}
```

#### 계층 순서를 갖춘 수동 초기화

세밀한 제어를 위해:

```cpp
auto initialize_systems() -> common::VoidResult {
    // 계층 0: 기반 (헤더 전용, 초기화 없음)
    // (common_system은 항상 사용 가능)

    // 계층 1: 핵심 시스템 (병렬 초기화)
    auto thread_result = thread::thread_pool::initialize(8);
    auto container_result = container::serializer::initialize();

    if (thread_result.is_err()) {
        return thread_result;
    }
    if (container_result.is_err()) {
        return container_result;
    }

    // 계층 2: 서비스 시스템 (thread_system 필요)
    auto logger_result = logger::logger::initialize(
        thread_result.value()  // 스레드 풀 전달
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

    // 계층 3: 통합 시스템
    auto network_result = network::server::initialize(
        thread_result.value(),  // Executor
        logger_result.value()   // Logger
    );

    return network_result;
}
```

### 종료 시퀀스

**황금률**: 초기화의 **역순**으로 종료합니다.

#### 자동 종료

```cpp
// unified_bootstrapper가 종료 순서를 자동으로 처리
auto shutdown_result = common::di::unified_bootstrapper::shutdown(
    std::chrono::seconds(30)  // 최대 타임아웃
);

if (shutdown_result.is_err()) {
    // 종료 중 타임아웃 또는 에러
    std::cerr << "종료 에러: " << shutdown_result.error().message << "\n";

    // 강제 종료가 필요할 수 있음
    std::exit(1);
}
```

#### 수동 종료 시퀀스

```cpp
auto graceful_shutdown(std::chrono::seconds timeout) -> common::VoidResult {
    auto start_time = std::chrono::steady_clock::now();

    // 계층 3: network_system (새 연결 수락 중지)
    auto network_result = network_system->shutdown(
        std::chrono::seconds(5)
    );
    if (network_result.is_err()) {
        logger->log(common::log_level::error,
                   "네트워크 종료 실패: " + network_result.error().message);
    }

    // 계층 2: 서비스 시스템 (병렬 종료)
    auto db_future = std::async(std::launch::async, [&]() {
        return database_system->shutdown(std::chrono::seconds(5));
    });

    auto logger_future = std::async(std::launch::async, [&]() {
        return logger_system->shutdown(std::chrono::seconds(5));
    });

    auto db_result = db_future.get();
    auto logger_result = logger_future.get();

    // 계층 1: thread_system (모든 작업 완료 대기)
    auto elapsed = std::chrono::steady_clock::now() - start_time;
    auto remaining = timeout - std::chrono::duration_cast<std::chrono::seconds>(elapsed);

    auto thread_result = thread_system->shutdown(remaining);
    if (thread_result.is_err()) {
        return common::make_error(
            common::error_codes::TIMEOUT,
            "스레드 풀 종료 타임아웃",
            "shutdown_manager"
        );
    }

    return common::ok();
}
```

### 타임아웃 및 에러 복구

#### 타임아웃 처리를 갖춘 종료

```cpp
auto shutdown_with_fallback(std::chrono::seconds timeout) -> common::VoidResult {
    auto deadline = std::chrono::steady_clock::now() + timeout;

    // 우아한 종료 시도
    auto result = graceful_shutdown(timeout);

    if (result.is_ok()) {
        return result;
    }

    // 우아한 종료 실패, 남은 시간 확인
    auto now = std::chrono::steady_clock::now();
    if (now >= deadline) {
        logger->log(common::log_level::error,
                   "우아한 종료 타임아웃, 강제 종료");

        // 모든 시스템 강제 중지
        network_system->force_stop();
        database_system->force_stop();
        thread_system->force_stop();

        return common::make_error(
            common::error_codes::TIMEOUT,
            "타임아웃 후 강제 종료",
            "shutdown_manager"
        );
    }

    // 남은 시간으로 재시도
    auto remaining = std::chrono::duration_cast<std::chrono::seconds>(deadline - now);
    logger->log(common::log_level::warn,
               "남은 시간 " + std::to_string(remaining.count()) + "초로 종료 재시도");

    return graceful_shutdown(remaining);
}
```

#### 초기화 롤백

```cpp
auto initialize_with_rollback() -> common::VoidResult {
    std::vector<std::function<void()>> cleanup_stack;

    // thread_system 초기화
    auto thread_result = thread::initialize();
    if (thread_result.is_err()) {
        return thread_result;
    }
    cleanup_stack.push_back([&]() { thread::shutdown(); });

    // logger_system 초기화
    auto logger_result = logger::initialize(thread_result.value());
    if (logger_result.is_err()) {
        // 롤백: thread_system 종료
        for (auto& cleanup : cleanup_stack) {
            cleanup();
        }
        return logger_result;
    }
    cleanup_stack.push_back([&]() { logger::shutdown(); });

    // database_system 초기화
    auto db_result = database::initialize();
    if (db_result.is_err()) {
        // 롤백: logger와 thread 종료
        for (auto& cleanup : cleanup_stack) {
            cleanup();
        }
        return db_result;
    }
    cleanup_stack.push_back([&]() { database::shutdown(); });

    // 성공: cleanup 스택 지우기 (롤백 불필요)
    cleanup_stack.clear();
    return common::ok();
}
```

---

## 7. 완전한 예제 애플리케이션

### 애플리케이션 개요

다음을 시연하는 프로덕션 준비 다중 시스템 애플리케이션:
- 4개 시스템 통합 (common, thread, logger, database)
- 시스템 경계를 넘는 에러 처리
- 설정 관리
- 우아한 초기화 및 종료
- 빌드 시스템 설정

**사용 시스템**:
- `common_system`: 기반 (Result<T>, 인터페이스)
- `thread_system`: 비동기 요청 처리
- `logger_system`: 구조화된 로깅
- `database_system`: 데이터 영속성

### 소스 코드

**파일**: `examples/multi_system_app/main.cpp`

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

// 애플리케이션 상태
std::atomic<bool> g_running{true};

// 우아한 종료를 위한 시그널 핸들러
void signal_handler(int signal) {
    std::cout << "\n시그널 " << signal << " 수신, 종료 중...\n";
    g_running = false;
}

// 사용자 데이터 모델
struct User {
    int id;
    std::string name;
    std::string email;
};

// 비즈니스 로직: 사용자 등록 처리
auto register_user(
    std::shared_ptr<database::database> db,
    std::shared_ptr<common::interfaces::ILogger> logger,
    const std::string& name,
    const std::string& email
) -> common::Result<User> {
    // 입력 검증
    if (name.empty() || email.empty()) {
        return common::make_error<User>(
            common::error_codes::INVALID_ARGUMENT,
            "이름과 이메일은 필수입니다",
            "register_user"
        );
    }

    logger->log(common::log_level::info,
               "사용자 등록: " + name + " <" + email + ">");

    // 사용자 존재 여부 확인
    auto check_query = "SELECT COUNT(*) FROM users WHERE email = '" + email + "'";
    auto check_result = db->query(check_query);

    if (check_result.is_err()) {
        return common::make_error<User>(check_result.error());
    }

    if (check_result.value().rows[0].get<int>(0) > 0) {
        return common::make_error<User>(
            common::error_codes::ALREADY_EXISTS,
            "이메일을 가진 사용자가 이미 존재함",
            "register_user"
        );
    }

    // 새 사용자 삽입
    auto insert_query = "INSERT INTO users (name, email) VALUES ('"
                       + name + "', '" + email + "')";
    auto insert_result = db->execute(insert_query);

    if (insert_result.is_err()) {
        logger->log(common::log_level::error,
                   "사용자 삽입 실패: " + insert_result.error().message);
        return common::make_error<User>(insert_result.error());
    }

    // 삽입된 사용자 검색
    auto user_id = db->last_insert_id();

    logger->log(common::log_level::info,
               "사용자 등록 성공, ID: " + std::to_string(user_id));

    return common::ok(User{
        .id = static_cast<int>(user_id),
        .name = name,
        .email = email
    });
}

int main(int argc, char** argv) {
    // 시그널 핸들러 설치
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);

    std::cout << "=== 다중 시스템 애플리케이션 데모 ===\n\n";

    // 단계 1: 설정 로딩
    std::cout << "1. 설정 로딩 중...\n";
    auto config_result = common::config::read_yaml("app.yaml");
    if (config_result.is_err()) {
        std::cerr << "Config 에러: " << config_result.error().message << "\n";
        std::cerr << "기본 설정 사용\n";
    }
    auto cfg = config_result.is_ok() ? config_result.value()
                                      : common::config::Config::defaults();

    // 단계 2: 시스템 초기화
    std::cout << "2. 시스템 초기화 중...\n";
    common::di::bootstrapper_options opts;
    opts.enable_logging = true;
    opts.enable_database = true;
    opts.config_path = "app.yaml";

    auto init_result = common::di::unified_bootstrapper::initialize(opts);
    if (init_result.is_err()) {
        std::cerr << "초기화 실패: "
                  << init_result.error().message << "\n";
        return 1;
    }

    std::cout << "   ✓ 시스템 초기화 성공\n\n";

    // 단계 3: 서비스 해결
    auto& services = common::di::unified_bootstrapper::services();
    auto logger = services.resolve<common::interfaces::ILogger>();
    auto executor = services.resolve<common::interfaces::IExecutor>();

    // 단계 4: 데이터베이스 설정
    std::cout << "3. 데이터베이스 설정 중...\n";
    database::connection_config db_config;
    db_config.type = database::db_type::sqlite;
    db_config.path = "users.db";

    auto db_result = database::database::connect(db_config);
    if (db_result.is_err()) {
        std::cerr << "데이터베이스 연결 실패: "
                  << db_result.error().message << "\n";
        common::di::unified_bootstrapper::shutdown();
        return 1;
    }
    auto db = db_result.value();

    // users 테이블 생성
    auto create_table_result = db->execute(
        "CREATE TABLE IF NOT EXISTS users ("
        "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  name TEXT NOT NULL,"
        "  email TEXT UNIQUE NOT NULL"
        ")"
    );

    if (create_table_result.is_err()) {
        std::cerr << "테이블 생성 실패\n";
        common::di::unified_bootstrapper::shutdown();
        return 1;
    }

    std::cout << "   ✓ 데이터베이스 준비 완료\n\n";

    // 단계 5: 애플리케이션 로직
    std::cout << "4. 애플리케이션 실행 중...\n";
    logger->log(common::log_level::info, "애플리케이션 시작");

    // 샘플 사용자 등록
    std::vector<std::pair<std::string, std::string>> sample_users = {
        {"Alice Smith", "alice@example.com"},
        {"Bob Johnson", "bob@example.com"},
        {"Carol Williams", "carol@example.com"}
    };

    for (const auto& [name, email] : sample_users) {
        auto result = register_user(db, logger, name, email);

        if (result.is_ok()) {
            const auto& user = result.value();
            std::cout << "   ✓ 등록됨: " << user.name
                      << " (ID: " << user.id << ")\n";
        } else {
            std::cout << "   ✗ 실패: " << name
                      << " - " << result.error().message << "\n";
        }

        // 처리 시간 시뮬레이션
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        if (!g_running) {
            break;
        }
    }

    // 모든 사용자 쿼리
    std::cout << "\n5. 모든 사용자 쿼리 중...\n";
    auto users_result = db->query("SELECT id, name, email FROM users");

    if (users_result.is_ok()) {
        const auto& rows = users_result.value().rows;
        std::cout << "   총 사용자: " << rows.size() << "\n";

        for (const auto& row : rows) {
            std::cout << "   - " << row.get<std::string>(1)
                      << " <" << row.get<std::string>(2) << ">\n";
        }
    }

    logger->log(common::log_level::info, "애플리케이션 완료");

    // 단계 6: 우아한 종료
    std::cout << "\n6. 종료 중...\n";
    auto shutdown_result = common::di::unified_bootstrapper::shutdown(
        std::chrono::seconds(5)
    );

    if (shutdown_result.is_err()) {
        std::cerr << "종료 에러: "
                  << shutdown_result.error().message << "\n";
        return 1;
    }

    std::cout << "   ✓ 종료 완료\n";
    std::cout << "\n=== 애플리케이션 종료 ===\n";

    return 0;
}
```

### 빌드 설정

**파일**: `examples/multi_system_app/CMakeLists.txt`

```cmake
cmake_minimum_required(VERSION 3.20)
project(multi_system_app VERSION 1.0.0 LANGUAGES CXX)

# C++20 필요
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# kcenon 시스템 찾기
find_package(common_system CONFIG REQUIRED)
find_package(thread_system CONFIG REQUIRED)
find_package(logger_system CONFIG REQUIRED)
find_package(database_system CONFIG REQUIRED)

# 실행 파일
add_executable(multi_system_app main.cpp)

# 라이브러리 링크
target_link_libraries(multi_system_app PRIVATE
    kcenon::common_system
    kcenon::thread_system
    kcenon::logger_system
    kcenon::database_system
)

# 빌드 디렉토리에 config 파일 복사
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/app.yaml
    ${CMAKE_CURRENT_BINARY_DIR}/app.yaml
    COPYONLY
)

# 설치
install(TARGETS multi_system_app
    RUNTIME DESTINATION bin
)
```

**파일**: `examples/multi_system_app/app.yaml`

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

### 예제 실행

```bash
# 빌드
mkdir build && cd build
cmake ..
cmake --build .

# 실행
./multi_system_app
```

**예상 출력**:
```
=== 다중 시스템 애플리케이션 데모 ===

1. 설정 로딩 중...
2. 시스템 초기화 중...
   ✓ 시스템 초기화 성공

3. 데이터베이스 설정 중...
   ✓ 데이터베이스 준비 완료

4. 애플리케이션 실행 중...
   ✓ 등록됨: Alice Smith (ID: 1)
   ✓ 등록됨: Bob Johnson (ID: 2)
   ✓ 등록됨: Carol Williams (ID: 3)

5. 모든 사용자 쿼리 중...
   총 사용자: 3
   - Alice Smith <alice@example.com>
   - Bob Johnson <bob@example.com>
   - Carol Williams <carol@example.com>

6. 종료 중...
   ✓ 종료 완료

=== 애플리케이션 종료 ===
```

---

## 관련 문서

- [빠른 시작 가이드](guides/QUICK_START.md) - 단일 시스템 시작하기
- [간단한 통합 예제](guides/INTEGRATION.md) - 기본 2-3 시스템 통합
- [아키텍처 개요](ARCHITECTURE.md) - 시스템 설계 철학
- [의존성 매트릭스](advanced/DEPENDENCY_MATRIX.md) - 상세한 의존성 분석
- [API 참조](API_REFERENCE.md) - 인터페이스 사양
- [Rust/C++ Parity Matrix](RUST_PARITY.md) - C++와 Rust 포트 간 기능 비교
- [에러 코드 레지스트리](ERROR_CODE_REGISTRY.md) - 완전한 에러 코드 참조

---

**문서 버전**: 3.0.0 (완전한 가이드)
**최종 업데이트**: 2026-02-08
**작성자**: kcenon team
**관련 이슈**: kcenon/common_system#329, #334, #335, #336

**영문 번역**: 이 완전한 가이드의 영문 버전은 별도 문서 (`docs/INTEGRATION_GUIDE.md`)에 제공됩니다.
