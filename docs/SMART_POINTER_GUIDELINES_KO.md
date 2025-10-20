# 스마트 포인터 사용 가이드라인

> **Language:** [English](SMART_POINTER_GUIDELINES.md) | **한국어**


**문서 버전**: 1.0
**생성일**: 2025-10-08
**관련 문서**: [RAII_GUIDELINES.md](./RAII_GUIDELINES.md)
**단계**: Phase 2 - 리소스 관리 표준화

---

## 의사결정 트리: 어떤 스마트 포인터를 사용할 것인가?

```
┌─────────────────────────────────┐
│ 리소스를 관리해야 합니까?        │
└────────────┬────────────────────┘
             │
             ├─ 아니오 → 비소유 접근을 위해 참조(&) 또는 포인터(*) 사용
             │
             └─ 예
                │
                ├─ 소유권이 배타적인가?
                │  │
                │  └─ 예 → std::unique_ptr<T>
                │
                ├─ 소유권이 공유되는가?
                │  │
                │  └─ 예 → std::shared_ptr<T>
                │
                └─ 공유 리소스에 대한 비소유 참조가 필요한가?
                   │
                   └─ 예 → std::weak_ptr<T>
```

---

## std::unique_ptr<T> - 배타적 소유권

### 사용 시점 (When to Use)

✅ **다음의 경우 std::unique_ptr 사용**:
- 리소스의 단일 소유자
- 소유권 이전이 필요한 경우
- 리소스를 명시적으로 관리해야 하는 경우
- 그렇지 않으면 `new`/`delete`를 사용할 경우

❌ **다음의 경우 사용하지 말 것**:
- 여러 소유자가 필요한 경우
- 리소스가 스택에 할당된 경우
- 비소유 접근으로 충분한 경우

### 기본 사용법 (Basic Usage)

```cpp
// ✅ make_unique로 생성 (C++14+)
auto widget = std::make_unique<Widget>(arg1, arg2);

// ✅ 명시적 생성 (커스텀 삭제자가 필요한 경우)
auto file = std::unique_ptr<FILE, decltype(&fclose)>(
    fopen("file.txt", "r"),
    &fclose
);

// ✅ 소유권 이전
std::unique_ptr<Widget> transfer_ownership() {
    auto widget = std::make_unique<Widget>();
    return widget;  // 자동 이동
}

// ✅ 배열 할당
auto array = std::make_unique<int[]>(size);
array[0] = 42;

// ❌ new를 직접 사용하지 말 것
Widget* bad = new Widget();  // 하지 마세요!
```

### 함수 파라미터 (Function Parameters)

```cpp
// ✅ 소유권 가져오기
void take_ownership(std::unique_ptr<Widget> widget) {
    // 이제 함수가 widget을 소유
}

// ✅ 소유권 이전 (호출자가 소유권 상실)
auto widget = std::make_unique<Widget>();
take_ownership(std::move(widget));
// widget은 이제 nullptr

// ✅ 비소유 접근
void use_widget(Widget* widget) {
    if (widget) {
        widget->do_something();
    }
}

// ✅ 비소유 접근 (null이 아님을 보장)
void use_widget_ref(Widget& widget) {
    widget.do_something();
}

// 사용법
auto widget = std::make_unique<Widget>();
use_widget(widget.get());           // 비소유 포인터
use_widget_ref(*widget);            // 비소유 참조
```

### 반환 값 (Return Values)

```cpp
// ✅ 팩토리 함수
std::unique_ptr<Connection> create_connection(const std::string& host) {
    auto conn = std::make_unique<Connection>();
    if (!conn->connect(host)) {
        return nullptr;  // 실패 표시
    }
    return conn;
}

// ✅ 더 나은 에러 처리를 위한 Result<T> 사용
Result<std::unique_ptr<Connection>> create_connection_safe(const std::string& host) {
    auto conn = std::make_unique<Connection>();
    auto result = conn->connect(host);
    if (is_error(result)) {
        return std::get<error_info>(result);
    }
    return std::move(conn);
}
```

### 커스텀 삭제자 (Custom Deleters)

```cpp
// 함수 포인터 삭제자
auto file = std::unique_ptr<FILE, decltype(&fclose)>(
    fopen("data.txt", "r"),
    &fclose
);

// 람다 삭제자
auto resource = std::unique_ptr<Resource, void(*)(Resource*)>(
    acquire_resource(),
    [](Resource* r) {
        cleanup_resource(r);
        delete r;
    }
);

// 상태를 가진 삭제자
struct socket_deleter {
    int timeout_ms;

    void operator()(Socket* sock) const {
        sock->shutdown(timeout_ms);
        delete sock;
    }
};

auto socket = std::unique_ptr<Socket, socket_deleter>(
    new Socket(),
    socket_deleter{5000}  // 5초 타임아웃
);
```

### 일반적인 패턴 (Common Patterns)

```cpp
// 패턴 1: Pimpl 관용구
class Widget {
    class Impl;
    std::unique_ptr<Impl> pimpl_;

public:
    Widget();
    ~Widget();  // Impl이 완전한 .cpp에서 정의되어야 함
};

// 패턴 2: 다형성 팩토리
std::unique_ptr<ILogger> create_logger(LoggerType type) {
    switch (type) {
        case LoggerType::File:
            return std::make_unique<FileLogger>();
        case LoggerType::Network:
            return std::make_unique<NetworkLogger>();
        default:
            return nullptr;
    }
}

// 패턴 3: 리소스 가드
template<typename Resource>
class ResourceGuard {
    std::unique_ptr<Resource> resource_;

public:
    explicit ResourceGuard(std::unique_ptr<Resource> r)
        : resource_(std::move(r)) {}

    Resource* get() { return resource_.get(); }
};
```

---

## std::shared_ptr<T> - 공유 소유권

### 사용 시점 (When to Use)

✅ **다음의 경우 std::shared_ptr 사용**:
- 리소스의 여러 소유자
- 소유권 생명주기가 복잡한 경우
- 리소스가 단일 소유자보다 오래 유지되어야 하는 경우
- 비동기 작업이 리소스를 유지해야 하는 경우

❌ **다음의 경우 사용하지 말 것**:
- 단일 소유자로 충분한 경우 (`unique_ptr` 사용)
- 소유권이 명확하고 단순한 경우
- 성능이 중요한 경우 (shared_ptr은 오버헤드가 있음)

### 기본 사용법 (Basic Usage)

```cpp
// ✅ make_shared로 생성 (권장)
auto widget = std::make_shared<Widget>(arg1, arg2);

// ✅ 명시적 생성 (커스텀 삭제자가 필요한 경우)
auto file = std::shared_ptr<FILE>(
    fopen("file.txt", "r"),
    &fclose
);

// ✅ 복사는 소유권을 공유
auto widget1 = std::make_shared<Widget>();
auto widget2 = widget1;  // 둘 다 widget을 소유
// widget1과 widget2가 모두 파괴될 때 Widget 삭제됨

// ✅ unique_ptr에서 변환
std::unique_ptr<Widget> unique_widget = std::make_unique<Widget>();
std::shared_ptr<Widget> shared_widget = std::move(unique_widget);
```

### std::enable_shared_from_this

```cpp
class Session : public std::enable_shared_from_this<Session> {
public:
    void start_async_read() {
        // 세션을 유지하기 위해 자신의 shared_ptr 캡처
        auto self = shared_from_this();

        socket_.async_read([self, this](const auto& data) {
            // 'self'는 콜백 중 'this'를 유효하게 유지
            handle_data(data);
        });
    }

    void handle_data(const std::vector<char>& data) {
        // 데이터 처리
    }

private:
    Socket socket_;
};

// ❌ 잘못됨 - 댕글링 포인터 생성
class BadSession {
    void start_async_read() {
        socket_.async_read([this](const auto& data) {
            // 'this'는 콜백 전에 삭제될 수 있음!
            handle_data(data);
        });
    }
};
```

### 참조 카운팅 (Reference Counting)

```cpp
auto widget = std::make_shared<Widget>();
std::cout << widget.use_count() << "\n";  // 1

{
    auto widget2 = widget;  // 참조 카운트 증가
    std::cout << widget.use_count() << "\n";  // 2
}  // widget2 파괴, 참조 카운트 감소

std::cout << widget.use_count() << "\n";  // 1
// widget이 스코프를 벗어날 때 Widget 파괴됨
```

### 순환 참조 (위험!) (Circular References - Danger!)

```cpp
// ❌ 나쁨 - 순환 참조로 메모리 누수 발생
class Node {
    std::shared_ptr<Node> parent_;  // 순환!
    std::shared_ptr<Node> child_;

public:
    void set_parent(std::shared_ptr<Node> parent) {
        parent_ = parent;
    }

    void set_child(std::shared_ptr<Node> child) {
        child_ = child;
    }
};

// 순환 참조 생성 - 메모리 누수!
auto parent = std::make_shared<Node>();
auto child = std::make_shared<Node>();
parent->set_child(child);
child->set_parent(parent);  // 순환 참조!
// parent와 child 모두 절대 삭제되지 않음

// ✅ 좋음 - weak_ptr로 순환 끊기
class Node {
    std::weak_ptr<Node> parent_;    // 비소유
    std::shared_ptr<Node> child_;   // 소유

public:
    void set_parent(std::shared_ptr<Node> parent) {
        parent_ = parent;  // 약한 참조 저장
    }

    void set_child(std::shared_ptr<Node> child) {
        child_ = child;
    }

    std::shared_ptr<Node> get_parent() {
        return parent_.lock();  // weak를 shared로 변환
    }
};
```

### 스레드 안전성 (Thread Safety)

```cpp
// ✅ 참조 카운팅은 스레드 안전
std::shared_ptr<Widget> global_widget;

// 스레드 1
global_widget = std::make_shared<Widget>();  // 스레드 안전

// 스레드 2
auto local_copy = global_widget;  // 스레드 안전 복사

// ⚠️ 하지만 가리키는 객체 자체는 자동으로 스레드 안전하지 않음
class Counter {
    int count_ = 0;  // 스레드 안전하지 않음!

public:
    void increment() {
        ++count_;  // 여러 스레드에서 호출되면 경쟁 조건
    }
};

auto counter = std::make_shared<Counter>();
// 여러 스레드에서 counter->increment() 호출은 안전하지 않음!

// ✅ 객체 자체를 보호해야 함
class ThreadSafeCounter {
    int count_ = 0;
    std::mutex mutex_;

public:
    void increment() {
        std::lock_guard<std::mutex> lock(mutex_);
        ++count_;
    }
};
```

---

## std::weak_ptr<T> - 비소유 참조

### 사용 시점 (When to Use)

✅ **다음의 경우 std::weak_ptr 사용**:
- 순환 참조 끊기
- 캐싱 (캐시 항목이 만료될 수 있는 경우)
- 옵저버 패턴
- 공유 리소스가 여전히 존재하는지 확인해야 하는 경우

### 기본 사용법 (Basic Usage)

```cpp
std::shared_ptr<Widget> shared = std::make_shared<Widget>();
std::weak_ptr<Widget> weak = shared;  // 비소유 참조

// 리소스가 여전히 존재하는지 확인
if (auto locked = weak.lock()) {
    // locked는 shared_ptr<Widget>
    locked->do_something();
}  // locked가 스코프를 벗어나면 참조 카운트 감소

// 만료되었는지 확인
if (weak.expired()) {
    std::cout << "Resource was deleted\n";
}
```

### 옵저버 패턴 (Observer Pattern)

```cpp
class Subject {
    std::vector<std::weak_ptr<Observer>> observers_;

public:
    void attach(std::shared_ptr<Observer> observer) {
        observers_.push_back(observer);
    }

    void notify() {
        // 만료된 옵저버 제거
        observers_.erase(
            std::remove_if(observers_.begin(), observers_.end(),
                [](const auto& weak) { return weak.expired(); }),
            observers_.end()
        );

        // 남은 옵저버에게 알림
        for (const auto& weak : observers_) {
            if (auto observer = weak.lock()) {
                observer->on_notify();
            }
        }
    }
};
```

### 캐시 패턴 (Cache Pattern)

```cpp
class ResourceCache {
    std::unordered_map<std::string, std::weak_ptr<Resource>> cache_;
    std::mutex mutex_;

public:
    std::shared_ptr<Resource> get_or_create(const std::string& key) {
        std::lock_guard<std::mutex> lock(mutex_);

        // 캐시에서 가져오기 시도
        auto it = cache_.find(key);
        if (it != cache_.end()) {
            if (auto resource = it->second.lock()) {
                return resource;  // 캐시 히트
            }
        }

        // 캐시 미스 또는 만료 - 새로 생성
        auto resource = std::make_shared<Resource>(key);
        cache_[key] = resource;
        return resource;
    }

    void cleanup_expired() {
        std::lock_guard<std::mutex> lock(mutex_);

        for (auto it = cache_.begin(); it != cache_.end();) {
            if (it->second.expired()) {
                it = cache_.erase(it);
            } else {
                ++it;
            }
        }
    }
};
```

---

## 원시 포인터 - 비소유 접근 (Raw Pointers - Non-Owning Access)

### 원시 포인터 사용 시점 (When to Use Raw Pointers)

✅ **다음의 경우 원시 포인터(*) 사용**:
- 비소유, 선택적 접근
- Null이 유효한 값인 경우
- C API 상호운용성

✅ **다음의 경우 참조(&) 사용**:
- 비소유, 유효성 보장
- 파라미터가 null이 아니어야 하는 경우
- 포인터보다 의미론적으로 명확한 경우

❌ **다음 용도로 원시 포인터를 절대 사용하지 말 것**:
- 소유권
- 동적 메모리 관리
- 소유권 이전

### 예시 (Examples)

```cpp
// ✅ 비소유 선택적 파라미터
void log_if_available(Logger* logger, const std::string& message) {
    if (logger) {
        logger->log(message);
    }
}

// ✅ 비소유 필수 파라미터
void log_always(Logger& logger, const std::string& message) {
    logger.log(message);  // Logger 유효성 보장
}

// ✅ 소유된 리소스 접근
auto widget = std::make_unique<Widget>();
Widget* raw = widget.get();  // 비소유 접근
use_widget(raw);  // widget의 생명주기 내에서 안전

// ❌ 원시 포인터로 소유권을 이전하지 말 것
Widget* create_widget() {
    return new Widget();  // 누가 이것을 삭제하나? 불명확!
}

// ✅ 소유권 이전에는 스마트 포인터 사용
std::unique_ptr<Widget> create_widget_safe() {
    return std::make_unique<Widget>();
}
```

---

## 소유권 문서화 (Ownership Documentation)

함수 시그니처와 주석에 소유권을 문서화:

```cpp
/**
 * @brief 새 연결 생성
 * @return 소유된 연결 객체. 호출자가 생명주기에 책임.
 */
std::unique_ptr<Connection> create_connection();

/**
 * @brief 로거를 사용하여 데이터 처리
 * @param logger 로거에 대한 비소유 포인터. null일 수 있음.
 */
void process_data(Logger* logger);

/**
 * @brief 옵저버 등록
 * @param observer 공유 소유권. 등록된 동안 객체 유지.
 */
void register_observer(std::shared_ptr<Observer> observer);
```

---

## 성능 고려사항 (Performance Considerations)

### unique_ptr 성능

- **크기**: 원시 포인터와 동일 (오버헤드 없음)
- **이동**: O(1), 저렴
- **복사**: 삭제됨 (컴파일 타임 에러)
- **오버헤드**: 소멸자 호출만

### shared_ptr 성능

- **크기**: 2x 포인터 (ptr + 제어 블록)
- **이동**: O(1), 저렴
- **복사**: O(1), 원자적 증가
- **오버헤드**: 참조 카운트를 위한 원자적 연산

```cpp
// 벤치마킹 결과 (근사치)
// 생성:
//   make_unique: ~10ns
//   make_shared: ~50ns
// 복사:
//   unique_ptr: N/A (삭제됨)
//   shared_ptr: ~5ns (원자적 증가)
// 소멸:
//   unique_ptr: ~5ns
//   shared_ptr: ~10ns (원자적 감소 + 확인)
```

### 최적화 팁 (Optimization Tips)

```cpp
// ✅ 소유권을 이전하지 않을 때는 const 참조로 전달
void process(const std::shared_ptr<Widget>& widget) {
    widget->do_something();  // 참조 카운트 변경 없음
}

// ❌ 불필요하게 값으로 전달
void process_bad(std::shared_ptr<Widget> widget) {
    widget->do_something();  // 불필요한 참조 카운트 증가/감소
}

// ✅ make_unique/make_shared 사용
auto widget = std::make_shared<Widget>();  // 단일 할당

// ❌ 별도의 할당 피하기
auto widget = std::shared_ptr<Widget>(new Widget());  // 두 번의 할당
```

---

## 마이그레이션 체크리스트 (Migration Checklist)

코드를 스마트 포인터 사용으로 변환할 때:

### Phase 1: 식별
- [ ] 모든 `new`와 `delete` 사용 찾기
- [ ] 소유권 의미론 식별
- [ ] 현재 생명주기 가정 문서화

### Phase 2: 변환
- [ ] `new`를 `make_unique` 또는 `make_shared`로 대체
- [ ] 수동 `delete` 호출 제거
- [ ] 함수 시그니처 업데이트
- [ ] 필요한 곳에 이동 시맨틱 추가

### Phase 3: 검증
- [ ] AddressSanitizer로 실행
- [ ] ThreadSanitizer로 실행
- [ ] 이중 삭제 확인
- [ ] 순환 참조 확인
- [ ] 성능 테스트 (중요 경로인 경우)

### Phase 4: 문서화
- [ ] 소유권 주석 추가
- [ ] API 문서 업데이트
- [ ] 공유 소유권 주의사항 기록

---

## 시스템별 예시 (Examples by System)

### logger_system

```cpp
class file_logger {
    std::unique_ptr<file_handle> file_;  // 배타적 소유권

public:
    static Result<std::unique_ptr<file_logger>> create(const std::string& path) {
        auto file = file_handle::open(path);
        if (is_error(file)) {
            return std::get<error_info>(file);
        }

        return std::unique_ptr<file_logger>(
            new file_logger(std::move(std::get<std::unique_ptr<file_handle>>(file)))
        );
    }
};
```

### network_system

```cpp
class session : public std::enable_shared_from_this<session> {
    void start() {
        auto self = shared_from_this();
        socket_.async_read([self, this](auto data) {
            handle_data(data);
        });
    }
};
```

### database_system

```cpp
class connection_pool {
    std::vector<std::unique_ptr<connection>> connections_;  // 연결 소유

    class connection_guard {
        connection_pool* pool_;
        connection* conn_;  // 비소유

    public:
        ~connection_guard() {
            pool_->release(conn_);
        }
    };
};
```

---

## 빠른 참조 (Quick Reference)

| 시나리오 | 스마트 포인터 |
|----------|---------------|
| 단일 소유자 | `std::unique_ptr<T>` |
| 여러 소유자 | `std::shared_ptr<T>` |
| 순환 참조 끊기 | `std::weak_ptr<T>` |
| 선택적 접근 | `T*` (원시 포인터) |
| 필수 접근 | `T&` (참조) |
| 소유권 이전 | `std::unique_ptr<T>` (이동) |
| 소유권 공유 | `std::shared_ptr<T>` (복사) |
| 팩토리 함수 | `std::unique_ptr<T>` |
| 비동기 콜백 | `std::shared_ptr<T>` |
| 캐시/옵저버 | `std::weak_ptr<T>` |

---

## 참고 자료 (References)

- [C++ Core Guidelines: Smart Pointers](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#Rr-smartpointer)
- [RAII Guidelines](./RAII_GUIDELINES.md)
- [NEED_TO_FIX.md Phase 2](../../NEED_TO_FIX.md)

---

**문서 상태**: Phase 2 기준선
**다음 검토**: Phase 2 완료 후
**유지관리자**: 아키텍처 팀
