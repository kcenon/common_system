# Common System을 위한 RAII 패턴 가이드라인

> **Language:** [English](RAII_GUIDELINES.md) | **한국어**


**문서 버전**: 1.0
**생성일**: 2025-10-08
**대상**: common_system을 사용하는 모든 시스템

---

## 목차

1. [개요](#개요)
2. [RAII 원칙](#raii-원칙)
3. [리소스 카테고리](#리소스-카테고리)
4. [스마트 포인터 가이드라인](#스마트-포인터-가이드라인)
5. [구현 패턴](#구현-패턴)
6. [일반적인 함정](#일반적인-함정)
7. [Result<T>와의 통합](#resultt와의-통합)
8. [예제](#예제)
9. [체크리스트](#체크리스트)

---

## 개요

RAII(Resource Acquisition Is Initialization)는 리소스 생명주기 관리를 위한 기본적인 C++ 관용구입니다. 이 문서는 모든 시스템에 걸친 일관된 RAII 사용을 위한 가이드라인을 제시합니다.

### 핵심 RAII 원칙

> **리소스는 생성자에서 획득되고 소멸자에서 자동으로 해제됩니다.**

**이점**:
- ✅ 예외 안전성 보장
- ✅ 수동 정리 불필요
- ✅ 명확한 소유권 시맨틱
- ✅ 결정적 리소스 해제
- ✅ 스레드 안전 리소스 관리

---

## RAII 원칙

### 원칙 1: 생성자에서 리소스 획득

```cpp
class file_writer {
    FILE* handle_;
public:
    explicit file_writer(const std::string& path) {
        handle_ = fopen(path.c_str(), "w");
        if (!handle_) {
            throw std::runtime_error("Failed to open file");
        }
    }

    ~file_writer() {
        if (handle_) {
            fclose(handle_);
        }
    }

    // 복사 삭제, 이동 활성화
    file_writer(const file_writer&) = delete;
    file_writer& operator=(const file_writer&) = delete;
    file_writer(file_writer&& other) noexcept
        : handle_(std::exchange(other.handle_, nullptr)) {}
    file_writer& operator=(file_writer&& other) noexcept {
        if (this != &other) {
            if (handle_) fclose(handle_);
            handle_ = std::exchange(other.handle_, nullptr);
        }
        return *this;
    }
};
```

### 원칙 2: 소멸자에서 리소스 해제

**소멸자는 반드시**:
- 예외를 throw하지 않음 (C++11+에서 기본적으로 `noexcept`)
- 멱등성 보장 (여러 번 호출해도 안전)
- 모든 정리 경로 처리

```cpp
~resource_guard() noexcept {
    // 안전한 정리 - 절대 throw하지 않음
    if (resource_) {
        try {
            cleanup(resource_);
        } catch (...) {
            // 에러를 로깅하되 전파하지 않음
            // 소멸자는 throw하면 안 됨
        }
    }
}
```

### 원칙 3: 복사 삭제, 이동 활성화

**Rule of Five/Zero**:
- **다섯 가지** 특수 멤버 함수를 모두 정의하거나,
- **아무것도** 정의하지 않음 (`= default` 또는 `= delete` 사용)

```cpp
class resource_wrapper {
public:
    // 명시적으로 복사 연산 삭제
    resource_wrapper(const resource_wrapper&) = delete;
    resource_wrapper& operator=(const resource_wrapper&) = delete;

    // 명시적으로 이동 연산 정의
    resource_wrapper(resource_wrapper&&) noexcept = default;
    resource_wrapper& operator=(resource_wrapper&&) noexcept = default;

    // 소멸자
    ~resource_wrapper() = default;
};
```

---

## 리소스 카테고리

### 카테고리 1: 시스템 리소스

**예시**: 파일, 소켓, mutex, 스레드, 타이머

**가이드라인**:
- 항상 RAII 래퍼 사용
- 네이키드 핸들 사용 금지
- 획득 시 타임아웃 제공

```cpp
// 파일 핸들
class file_handle {
    FILE* handle_ = nullptr;
public:
    explicit file_handle(const char* path, const char* mode);
    ~file_handle() { if (handle_) fclose(handle_); }
    FILE* get() const { return handle_; }
};

// 소켓 핸들
class socket_handle {
    int fd_ = -1;
public:
    explicit socket_handle(int domain, int type, int protocol);
    ~socket_handle() { if (fd_ >= 0) close(fd_); }
    int get() const { return fd_; }
};

// Mutex 락
// std::lock_guard, std::unique_lock, std::scoped_lock 사용
```

### 카테고리 2: 메모리 리소스

**예시**: 힙 할당, 메모리 풀, 버퍼

**가이드라인**:
- `std::unique_ptr` 및 `std::shared_ptr` 선호
- 필요 시 커스텀 deleter 사용
- 네이키드 `new`/`delete` 피하기

```cpp
// 독점 소유권
auto buffer = std::make_unique<char[]>(size);

// 공유 소유권
auto shared_buffer = std::make_shared<buffer_type>(args);

// 커스텀 deleter
auto resource = std::unique_ptr<Resource, void(*)(Resource*)>(
    acquire_resource(),
    [](Resource* r) { release_resource(r); }
);
```

### 카테고리 3: 논리적 리소스

**예시**: 락, 트랜잭션, 스코프

**가이드라인**:
- 정리 작업에 스코프 가드 사용
- 논리적 리소스를 위한 RAII 래퍼 구현

```cpp
// 트랜잭션 가드
class transaction_guard {
    database& db_;
    bool committed_ = false;
public:
    explicit transaction_guard(database& db) : db_(db) {
        db_.begin_transaction();
    }

    ~transaction_guard() {
        if (!committed_) {
            db_.rollback();
        }
    }

    void commit() {
        db_.commit();
        committed_ = true;
    }
};
```

---

## 스마트 포인터 가이드라인

### std::unique_ptr<T>

**사용 시기**: 독점 소유권

```cpp
class logger {
    std::unique_ptr<writer> writer_;
public:
    explicit logger(std::unique_ptr<writer> w)
        : writer_(std::move(w)) {}
};

// 생성
auto logger = std::make_unique<file_logger>("app.log");
```

**규칙**:
- ✅ 생성 시 `std::make_unique` 사용
- ✅ 소유권 이전을 위해 `std::unique_ptr<T>`로 전달
- ✅ 비소유 접근을 위해 `T*` 또는 `T&`로 전달
- ❌ `new`를 직접 사용 금지

### std::shared_ptr<T>

**사용 시기**: 공유 소유권이 필요할 때

```cpp
class session : public std::enable_shared_from_this<session> {
    void start_async_operation() {
        auto self = shared_from_this();
        async_op([self, this]() {
            // 'self'가 콜백 중 session을 유지
            process();
        });
    }
};
```

**규칙**:
- ✅ 생성 시 `std::make_shared` 사용
- ✅ 순환 참조를 끊기 위해 `std::weak_ptr` 사용
- ✅ 필요 시 `std::enable_shared_from_this` 상속
- ⚠️ 순환 참조 주의

### std::weak_ptr<T>

**사용 시기**: 공유 리소스에 대한 비소유 참조

```cpp
class connection_manager {
    std::vector<std::weak_ptr<connection>> connections_;

    void cleanup_dead_connections() {
        connections_.erase(
            std::remove_if(connections_.begin(), connections_.end(),
                [](const auto& weak) { return weak.expired(); }),
            connections_.end()
        );
    }
};
```

### Raw 포인터

**사용 시기**: 비소유, 유효성이 보장된 참조

```cpp
// ✅ 비소유 파라미터
void process(const logger* logger) {
    // logger가 함수 스코프 동안 유효함 보장
}

// ✅ 선택적 비소유 파라미터
void log_if_available(logger* logger) {
    if (logger) {
        logger->info("message");
    }
}

// ❌ 소유권에는 절대 사용 금지
void bad_example(Thing* thing) {
    delete thing;  // 누가 소유하는가? 불명확!
}
```

---

## 구현 패턴

### 패턴 1: 기본 RAII 래퍼

```cpp
template<typename Resource, typename Deleter>
class resource_guard {
    Resource resource_;
    Deleter deleter_;
    bool valid_ = true;

public:
    explicit resource_guard(Resource r, Deleter d)
        : resource_(r), deleter_(d) {}

    ~resource_guard() {
        if (valid_) {
            deleter_(resource_);
        }
    }

    // 복사 삭제
    resource_guard(const resource_guard&) = delete;
    resource_guard& operator=(const resource_guard&) = delete;

    // 이동 활성화
    resource_guard(resource_guard&& other) noexcept
        : resource_(std::exchange(other.resource_, Resource{}))
        , deleter_(std::move(other.deleter_))
        , valid_(std::exchange(other.valid_, false)) {}

    Resource get() const { return resource_; }
    void release() { valid_ = false; }
};
```

### 패턴 2: 스코프 가드

```cpp
template<typename Func>
class scope_exit {
    Func cleanup_;
    bool dismissed_ = false;

public:
    explicit scope_exit(Func f) : cleanup_(std::move(f)) {}

    ~scope_exit() {
        if (!dismissed_) {
            cleanup_();
        }
    }

    void dismiss() { dismissed_ = true; }

    scope_exit(const scope_exit&) = delete;
    scope_exit& operator=(const scope_exit&) = delete;
};

// 헬퍼
template<typename F>
auto make_scope_exit(F&& f) {
    return scope_exit<std::decay_t<F>>(std::forward<F>(f));
}

// 사용법
void example() {
    auto guard = make_scope_exit([&] {
        cleanup_resources();
    });

    // 작업 수행
    // 정리가 자동으로 발생
}
```

### 패턴 3: 연결 가드 (NEED_TO_FIX.md에서)

```cpp
class connection_guard {
    connection_pool* pool_;
    connection* conn_;

public:
    explicit connection_guard(connection_pool* pool)
        : pool_(pool), conn_(pool->acquire()) {
        if (!conn_) {
            throw std::runtime_error("Failed to acquire connection");
        }
    }

    ~connection_guard() {
        if (conn_) {
            pool_->release(conn_);
        }
    }

    connection* operator->() { return conn_; }
    const connection* operator->() const { return conn_; }

    connection_guard(const connection_guard&) = delete;
    connection_guard& operator=(const connection_guard&) = delete;
};
```

---

## 일반적인 함정

### 함정 1: 복사 생성자 삭제를 잊음

```cpp
// ❌ 나쁨 - 복사를 허용하여 이중 삭제 가능
class resource {
    void* handle_;
public:
    ~resource() { free(handle_); }
};

// ✅ 좋음 - 명시적으로 복사 삭제
class resource {
    void* handle_;
public:
    ~resource() { free(handle_); }
    resource(const resource&) = delete;
    resource& operator=(const resource&) = delete;
};
```

### 함정 2: 소멸자에서 throw

```cpp
// ❌ 나쁨 - 소멸자가 throw 가능
~bad_resource() {
    cleanup();  // throw 가능!
}

// ✅ 좋음 - 소멸자는 절대 throw하지 않음
~good_resource() noexcept {
    try {
        cleanup();
    } catch (...) {
        // 로깅하되 전파하지 않음
    }
}
```

### 함정 3: 순환 shared_ptr 참조

```cpp
// ❌ 나쁨 - 순환 참조로 인한 누수
class Node {
    std::shared_ptr<Node> parent_;  // 순환!
    std::shared_ptr<Node> child_;
};

// ✅ 좋음 - weak_ptr로 순환 끊기
class Node {
    std::weak_ptr<Node> parent_;     // 비소유
    std::shared_ptr<Node> child_;    // 소유
};
```

### 함정 4: 네이키드 new/delete

```cpp
// ❌ 나쁨 - 수동 메모리 관리
Widget* w = new Widget();
// ... w 사용 ...
delete w;  // 잊기 쉬움!

// ✅ 좋음 - 자동 정리
auto w = std::make_unique<Widget>();
// ... w 사용 ...
// 자동 정리
```

---

## Result<T>와의 통합

RAII는 예외 없는 에러 처리를 위해 `common::Result<T>`와 완벽하게 작동합니다.

### 패턴: RAII 팩토리 함수

```cpp
Result<std::unique_ptr<file_writer>> create_file_writer(const std::string& path) {
    FILE* handle = fopen(path.c_str(), "w");
    if (!handle) {
        return error_info{errno, std::strerror(errno), "file_writer"};
    }

    return std::make_unique<file_writer>(handle);  // RAII 래퍼
}

// 사용법
auto result = create_file_writer("output.txt");
if (is_error(result)) {
    // 에러 처리
    return;
}

auto writer = std::move(std::get<std::unique_ptr<file_writer>>(result));
// writer 사용 - 스코프 종료 시 자동 정리
```

### 패턴: 에러 전파가 있는 RAII

```cpp
Result<void> process_file(const std::string& path) {
    // RAII로 리소스 획득
    auto file_result = create_file_writer(path);
    if (is_error(file_result)) {
        return std::get<error_info>(file_result);
    }

    auto writer = std::move(std::get<std::unique_ptr<file_writer>>(file_result));

    // 리소스 사용
    auto write_result = writer->write("data");
    if (is_error(write_result)) {
        return std::get<error_info>(write_result);
    }

    return std::monostate{};
    // writer는 여기서 자동으로 정리됨
}
```

---

## 예제

### 예제 1: RAII가 있는 파일 writer

```cpp
class file_writer {
    FILE* handle_ = nullptr;

public:
    static Result<std::unique_ptr<file_writer>> create(const std::string& path) {
        FILE* handle = fopen(path.c_str(), "w");
        if (!handle) {
            return error_info{errno, std::strerror(errno), "file_writer::create"};
        }

        return std::unique_ptr<file_writer>(new file_writer(handle));
    }

    ~file_writer() {
        if (handle_) {
            fclose(handle_);
        }
    }

    Result<void> write(const std::string& data) {
        size_t written = fwrite(data.data(), 1, data.size(), handle_);
        if (written != data.size()) {
            return error_info{EIO, "Write failed", "file_writer::write"};
        }
        return std::monostate{};
    }

private:
    explicit file_writer(FILE* handle) : handle_(handle) {}

    file_writer(const file_writer&) = delete;
    file_writer& operator=(const file_writer&) = delete;
};
```

### 예제 2: RAII가 있는 연결 풀

```cpp
class connection_pool {
    std::vector<std::unique_ptr<connection>> connections_;
    std::queue<connection*> available_;
    std::mutex mutex_;

public:
    class connection_guard {
        connection_pool* pool_;
        connection* conn_;

    public:
        connection_guard(connection_pool* pool, connection* conn)
            : pool_(pool), conn_(conn) {}

        ~connection_guard() {
            if (conn_) {
                pool_->release(conn_);
            }
        }

        connection* operator->() { return conn_; }

        connection_guard(const connection_guard&) = delete;
        connection_guard& operator=(const connection_guard&) = delete;

        connection_guard(connection_guard&& other) noexcept
            : pool_(other.pool_)
            , conn_(std::exchange(other.conn_, nullptr)) {}
    };

    Result<connection_guard> acquire() {
        std::lock_guard<std::mutex> lock(mutex_);

        if (available_.empty()) {
            return error_info{EAGAIN, "Pool exhausted", "connection_pool::acquire"};
        }

        auto* conn = available_.front();
        available_.pop();

        return connection_guard{this, conn};
    }

private:
    void release(connection* conn) {
        std::lock_guard<std::mutex> lock(mutex_);
        available_.push(conn);
    }
};
```

---

## 체크리스트

RAII 구현 시 이 체크리스트를 사용하세요:

### 설계 단계
- [ ] 관리가 필요한 모든 리소스 식별
- [ ] 소유권 모델 결정 (unique vs shared)
- [ ] 예외 안전한 생성자 설계
- [ ] 에러 처리 전략 계획

### 구현 단계
- [ ] 생성자에서 리소스 획득
- [ ] 소멸자에서 리소스 해제
- [ ] 소멸자가 `noexcept`
- [ ] 복사 생성자 삭제 (복사 불가능한 경우)
- [ ] 복사 할당 삭제 (복사 불가능한 경우)
- [ ] 이동 생성자 구현 (이동 가능한 경우)
- [ ] 이동 할당 구현 (이동 가능한 경우)
- [ ] 힙 할당에 스마트 포인터 사용
- [ ] 네이키드 `new` 또는 `delete` 없음

### 통합 단계
- [ ] 팩토리 함수가 `Result<std::unique_ptr<T>>` 반환
- [ ] 에러 경로 적절히 처리됨
- [ ] 리소스 소유권 문서화됨
- [ ] 스레드 안전성 문서화됨

### 테스팅 단계
- [ ] 예외 안전성 테스트됨
- [ ] 리소스 누수 테스트됨 (AddressSanitizer)
- [ ] 이중 삭제 방지됨
- [ ] 이동 시맨틱 테스트됨
- [ ] 동시 접근 테스트됨 (해당되는 경우)

---

## 참고 자료

- [C++ Core Guidelines: Resource Management](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#r-resource-management)
- [common_system Result<T> Documentation](./ERRORS.md)

---

*Last Updated: 2025-10-20*
