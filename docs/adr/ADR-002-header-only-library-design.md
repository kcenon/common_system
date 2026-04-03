---
doc_id: "COM-ADR-002"
doc_title: "ADR-002: Header-Only Library Design"
doc_version: "1.0.0"
doc_date: "2026-04-04"
doc_status: "Accepted"
project: "common_system"
category: "ADR"
---

# ADR-002: Header-Only Library Design

> **SSOT**: This document is the single source of truth for **ADR-002: Header-Only Library Design**.

| Field | Value |
|-------|-------|
| Status | Accepted |
| Date | 2025-01-15 |
| Decision Makers | kcenon ecosystem maintainers |

## Context

common_system is the Tier 0 foundation of the kcenon ecosystem — every other project
depends on it. The library provides interfaces (`IExecutor`, `IJob`, `ILogger`),
patterns (`Result<T>`, `event_bus`), C++20 concepts, and utility abstractions.

Two distribution models were considered:

1. **Compiled library** — Traditional `.a`/`.lib` static library requiring consumers
   to link against a pre-built artifact.
2. **Header-only library** — All code in headers, consumed via `#include` with no
   separate compilation step for the library itself.

Key constraints:
- All 7 downstream projects must integrate common_system.
- The library is predominantly interfaces (pure virtual) and templates.
- Cross-platform support (Linux, macOS, Windows) with multiple compilers.
- Pre-1.0 API: frequent interface changes expected.

## Decision

**Adopt a header-only distribution model** with `INTERFACE` CMake target.

The CMake target `common_system` is declared as `INTERFACE`, meaning it only
exports include paths and compile definitions — no object files are compiled
when consumers add `target_link_libraries(... common_system)`.

```cmake
add_library(common_system INTERFACE)
target_include_directories(common_system INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>)
```

The `COMMON_HEADER_ONLY` CMake option (ON by default) controls this behavior.
When disabled, the library compiles a static library from `.cpp` source files
for non-template implementations.

## Alternatives Considered

### Compiled Static Library

- **Pros**: Faster downstream builds (no re-parsing headers), ABI stability,
  ability to hide implementation details.
- **Cons**: Requires pre-built artifacts per platform/compiler/config combination,
  complicates cross-platform CI, version mismatch risk when updating common_system
  independently of consumers.

### Shared Library (`.so`/`.dll`)

- **Pros**: Reduced binary size, runtime updates without recompilation.
- **Cons**: ABI compatibility burden, symbol visibility management, complicates
  deployment for a pre-1.0 library with evolving interfaces.

## Consequences

### Positive

- **Zero build friction**: Downstream projects only need the include path — no
  pre-compilation, no artifact management, no linker errors from version mismatches.
- **Template-friendly**: The library is primarily interfaces and templates, which
  must be in headers regardless. Header-only aligns with the natural code structure.
- **Single-source-of-truth builds**: Consumers always compile against the exact
  version of common_system they reference — no stale `.a` artifacts.
- **Simplified CI**: No need to build and cache common_system artifacts separately
  for each platform/compiler/config matrix.

### Negative

- **Increased downstream compile times**: Every translation unit that includes
  common_system headers re-parses them. Mitigated by precompiled headers (PCH)
  and C++20 modules (experimental).
- **No ABI stability**: Any header change forces recompilation of all consumers.
  Acceptable for a pre-1.0 ecosystem.
- **Header pollution**: All implementation details are visible to consumers.
  Mitigated by `detail/` namespace convention.
