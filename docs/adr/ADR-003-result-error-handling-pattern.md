---
doc_id: "COM-ADR-003"
doc_title: "ADR-003: Result<T> Error Handling Pattern"
doc_version: "1.0.0"
doc_date: "2026-04-04"
doc_status: "Accepted"
project: "common_system"
category: "ADR"
---

# ADR-003: Result\<T\> Error Handling Pattern

> **SSOT**: This document is the single source of truth for **ADR-003: Result<T> Error Handling Pattern**.

| Field | Value |
|-------|-------|
| Status | Accepted |
| Date | 2024-06-01 |
| Decision Makers | kcenon ecosystem maintainers |

## Context

The kcenon ecosystem needed a consistent error handling strategy across all projects.
C++ offers several error propagation mechanisms, each with tradeoffs:

- **Exceptions** — Zero-cost happy path, but unpredictable control flow, disabled in
  some embedded/real-time contexts, and poor composability in async pipelines.
- **Error codes** — Explicit, but easily ignored, no type safety, and force out-parameters
  for return values.
- **`std::optional<T>`** — Signals absence but carries no error information.
- **`std::expected<T, E>`** (C++23) — Not available when the project started (C++20 target).

The ecosystem requires error handling that is:
1. Explicit — callers cannot accidentally ignore errors.
2. Composable — supports monadic chaining for pipeline-style code.
3. Informative — carries structured error information (code, message, source location).
4. Thread-safe for concurrent reads.

## Decision

**Adopt a Rust-inspired `Result<T>` type** as the standard error handling pattern
across the entire ecosystem.

```cpp
// Result<T> holds either a value or an error
auto result = some_operation();
if (result.has_value()) {
    use(result.value());
} else {
    handle(result.error());
}

// Monadic chaining
auto final = get_config()
    .and_then([](auto cfg) { return validate(cfg); })
    .map([](auto valid) { return transform(valid); })
    .or_else([](auto err) { return fallback(err); });
```

Key design choices:
- `Result<T>` wraps `std::optional<T>` (value) + `std::optional<error_info>` (error).
- `VoidResult` alias for operations with no return value.
- Default constructor is **deleted** — forces explicit success/failure construction.
- Monadic operations: `and_then()`, `map()`, `or_else()`, `map_error()`.
- `error_info` carries error code, message, and source location.
- Error code ranges are centralized: common (-1 to -99), thread (-100 to -199), etc.

## Alternatives Considered

### C++ Exceptions

- **Pros**: Zero overhead on success, idiomatic C++, stack unwinding.
- **Cons**: Unpredictable performance on error path, disabled by `-fno-exceptions`
  in some build configurations, poor composability in async/coroutine contexts,
  no compile-time enforcement of error handling.

### `std::error_code` with Out Parameters

- **Pros**: No heap allocation, well-understood pattern.
- **Cons**: Easily ignored (no `[[nodiscard]]`), forces awkward API signatures
  (`bool foo(int& out, std::error_code& ec)`), no monadic composition.

### `std::expected<T, E>` (C++23)

- **Pros**: Standard library support, monadic operations in C++23.
- **Cons**: Not available in C++20 (project baseline). The ecosystem's `Result<T>`
  predates `std::expected` availability. Migration may be considered post-C++23
  adoption.

## Consequences

### Positive

- **Ecosystem-wide consistency**: All 8 projects use `Result<T>` for fallible
  operations, creating a uniform error handling contract.
- **Composable pipelines**: `and_then`/`map`/`or_else` enable functional-style
  error propagation without nested if-else chains.
- **Explicit error handling**: Deleted default constructor + `[[nodiscard]]`
  prevent accidental error suppression.
- **Structured errors**: `error_info` with codes, messages, and source locations
  provides rich debugging context.

### Negative

- **Learning curve**: Contributors unfamiliar with monadic error handling need
  onboarding on the `Result<T>` API.
- **Not thread-safe for writes**: Concurrent modification requires external
  synchronization. Concurrent reads are safe.
- **Migration effort**: Existing code using exceptions or raw error codes must
  be migrated incrementally (ongoing in network_system at ~75-80%).
- **Binary size**: Template instantiations for each `T` increase binary size
  compared to exceptions or error codes.
