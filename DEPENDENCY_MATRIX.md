# Ecosystem Dependency Version Matrix

Centralized tracking of all third-party dependency versions across the kcenon ecosystem.
This document serves as the single source of truth for dependency version management,
SOUP (Software of Unknown Provenance) traceability, and license compliance.

> **Maintained in**: common_system (Tier 0, ecosystem root)
> **Related**: [SOUP-LIST.md](./SOUP-LIST.md) | [LICENSE-THIRD-PARTY](./LICENSE-THIRD-PARTY)

## Version Override Matrix

Pinned versions from each system's `vcpkg.json` overrides. Dash (—) means the system
does not use that dependency.

| Dependency | License | common | thread | logger | container | monitoring | database | network | Ecosystem Standard |
|-----------|---------|:------:|:------:|:------:|:---------:|:----------:|:--------:|:-------:|:------------------:|
| GTest | BSD-3-Clause | 1.14.0 | 1.14.0 | 1.14.0 | 1.14.0 | 1.14.0 | 1.14.0 | 1.14.0 | **1.14.0** |
| Benchmark | Apache-2.0 | 1.8.3 | 1.8.3 | 1.8.3 | 1.8.3 | — | 1.8.3 | 1.8.3 | **1.8.3** |
| ASIO | BSL-1.0 | — | — | — | — | — | 1.30.2 | 1.30.2 | **1.30.2** |
| OpenSSL | Apache-2.0 | — | — | 3.3.0 | — | — | 3.3.0 | 3.3.0 | **3.3.0** |
| zlib | zlib | — | — | — | — | — | — | 1.3.1 | **1.3.1** |
| LZ4 | BSD-2-Clause | — | — | — | — | — | — | 1.9.4 | **1.9.4** |
| spdlog | MIT | — | 1.13.0 | 1.13.0 | — | — | 1.13.0 | — | **1.13.0** |
| gRPC | Apache-2.0 | — | — | 1.51.1 | — | 1.51.1 | — | — | **1.51.1** |
| Protobuf | BSD-3-Clause | — | — | 3.21.12 | — | 3.21.12 | — | — | **3.21.12** |
| OTel C++ | Apache-2.0 | — | — | 1.14.2 | — | — | — | — | **1.14.2** |
| libiconv | LGPL-2.1 | — | 1.17 | — | — | — | — | — | **1.17** (dynamic linking) |
| libpqxx | BSD-3-Clause | — | — | — | — | — | 7.9.0 | — | **7.9.0** |
| libpq | PostgreSQL | — | — | — | — | — | 16.2 | — | **16.2** |
| sqlite3 | Public Domain | — | — | — | — | — | 3.45.3 | — | **3.45.3** |
| mongo-cxx-driver | Apache-2.0 | — | — | — | — | — | 3.10.1 | — | **3.10.1** (experimental) |
| hiredis | BSD-3-Clause | — | — | — | — | — | 1.2.0 | — | **1.2.0** (experimental) |

## Minimum Version Constraints

Version requirements from `vcpkg.json` dependencies and `CMakeLists.txt` find_package calls.

| Dependency | common | thread | logger | container | monitoring | database | network |
|-----------|:------:|:------:|:------:|:---------:|:----------:|:--------:|:-------:|
| CMake | >= 3.28 | >= 3.16 | >= 3.16 | >= 3.16 | >= 3.20 | >= 3.16 | >= 3.16 |
| C++ Standard | C++20 | C++20 | C++20 | C++20 | C++20 | C++20 | C++20 |
| ASIO | — | — | — | — | — | >= 1.29.0 | >= 1.30.2 |
| OpenSSL | — | — | >= 3.0 | — | — | >= 3.0.0 | >= 3.0.0 |
| GTest | >= 1.14.0 | — | — | — | — | — | — |
| Benchmark | — | >= 1.8.0 | — | — | — | — | — |
| gRPC | — | — | >= 1.51.1 | — | >= 1.51.1 | — | — |
| Protobuf | — | — | >= 3.21.0 | — | >= 3.21.0 | — | — |
| OTel C++ | — | — | >= 1.14.0 | — | — | — | — |
| spdlog | — | >= 1.12.0 | >= 1.13.0 | — | — | — | — |
| zlib | — | — | — | — | — | — | >= 1.3 |
| sqlite3 | — | — | — | — | — | >= 3.45.0 | — |
| mongo-cxx-driver | — | — | — | — | — | >= 3.8.0 | — |
| hiredis | — | — | — | — | — | >= 1.2.0 | — |

## Dependency Type Classification

How each dependency is used across the ecosystem.

| Dependency | Type | Systems Using |
|-----------|------|---------------|
| GTest | Test | All 7 systems |
| Benchmark | Test | common, thread, logger, container, database, network |
| ASIO | Core | database, network |
| OpenSSL | Core/Optional | network (core); logger, database (optional) |
| zlib | Core | network |
| LZ4 | Optional | network |
| spdlog | Optional | thread, logger, database |
| gRPC | Optional | logger (OTLP), container, monitoring |
| Protobuf | Optional | logger (OTLP), container, monitoring |
| OTel C++ | Optional | logger |
| libiconv | Optional | thread (non-Windows only) |
| libpqxx | Optional | database (PostgreSQL backend) |
| libpq | Optional | database (PostgreSQL backend) |
| sqlite3 | Optional | database (SQLite backend) |
| libmariadb | Optional | database (MySQL backend) |
| mongo-cxx-driver | Optional | database (MongoDB backend, experimental) |
| hiredis | Optional | database (Redis backend, experimental) |

## LGPL-2.1 Compliance Requirements

These dependencies require **dynamic linking** to maintain BSD-3-Clause license compatibility.

| Dependency | Version | System | Enforcement Mechanism |
|-----------|---------|--------|----------------------|
| libiconv | 1.17 | thread_system | vcpkg triplet (shared library) |
| libmariadb | 3.x | database_system | vcpkg triplet overlay + CMake TYPE check + CI verification |

## Known Version Discrepancies

Issues identified during SBOM analysis that require resolution.

| Issue | Details | Tracking |
|-------|---------|----------|
| gRPC not pinned in container_system | container_system uses gRPC but has no version override | [container_system#389](https://github.com/kcenon/container_system/issues/389) |
| gRPC minimum differs in network_system | network_system requires >= 1.50.0, ecosystem standard is 1.51.1 | [network_system#792](https://github.com/kcenon/network_system/issues/792) |
| ASIO minimum differs in database_system | database_system requires >= 1.29.0, override is 1.30.2 | [database_system#400](https://github.com/kcenon/database_system/issues/400) |
| OpenSSL minimum differs in database_system | database_system requires >= 3.0.0, ecosystem standard is 3.3.0 | [database_system#402](https://github.com/kcenon/database_system/issues/402) |
| fmt removed from ecosystem | All systems migrated to C++20 std::format | Completed: [#406](https://github.com/kcenon/common_system/issues/406) |
| vcpkg baseline differs in thread_system | thread_system uses different vcpkg-configuration.json baseline | — |

## Ecosystem Internal Dependencies

Tier-based dependency graph of kcenon systems.

```
Tier 0: common_system           (no third-party production deps)
   |
Tier 1: thread_system           -> common_system
         container_system        -> common_system
   |
Tier 2: logger_system           -> common_system, thread_system (optional)
   |
Tier 3: monitoring_system       -> common_system, thread_system
         database_system        -> common_system + DB backends
   |
Tier 4: network_system          -> common_system, thread_system, container_system
   |
Tier 5: pacs_system             -> common_system, container_system, network_system
```

## vcpkg Baseline Tracking

| System | Baseline Hash | Registry |
|--------|-------------|----------|
| common_system | `c4af3593e1f1aa9e14a560a09e45ea2cb0dfd74d` | builtin |
| thread_system | `50c0cb48a0cf2f6fc5c7b2c0d2bafbe26d0a7ca2` | git (microsoft/vcpkg) |
| logger_system | `c4af3593e1f1aa9e14a560a09e45ea2cb0dfd74d` | builtin |
| container_system | `c4af3593e1f1aa9e14a560a09e45ea2cb0dfd74d` | builtin |
| monitoring_system | `c4af3593e1f1aa9e14a560a09e45ea2cb0dfd74d` | builtin |
| database_system | `c4af3593e1f1aa9e14a560a09e45ea2cb0dfd74d` | builtin |
| network_system | `c4af3593e1f1aa9e14a560a09e45ea2cb0dfd74d` | builtin |

> **Note**: thread_system uses a different baseline and registry type. Consider aligning
> to the shared builtin baseline for consistency.

## License Compatibility Summary

All dependencies are compatible with the project's BSD-3-Clause license.

| License | Dependencies | Compatible | Notes |
|---------|-------------|:----------:|-------|
| BSD-3-Clause | GTest, Protobuf, libpqxx, hiredis | Yes | Same license family |
| MIT | spdlog, yaml-cpp | Yes | Permissive |
| Apache-2.0 | Benchmark, OpenSSL, gRPC, OTel C++, mongo-cxx-driver | Yes | Patent clause applies |
| BSL-1.0 | ASIO | Yes | Boost Software License |
| zlib | zlib | Yes | Permissive |
| BSD-2-Clause | LZ4 | Yes | Permissive |
| LGPL-2.1 | libiconv, libmariadb | Conditional | **Must use dynamic linking** |
| Public Domain | sqlite3 | Yes | No restrictions |
| PostgreSQL | libpq | Yes | BSD-family |

## Internal Ecosystem Version Pinning

Pinned versions of internal ecosystem libraries used by downstream consumers.
All references must use tagged versions — never `main` branch. See [VERSIONING.md](./VERSIONING.md).

| Library | Latest Release | FetchContent GIT_TAG | vcpkg REF |
|---------|---------------|----------------------|-----------|
| common_system | `v0.2.0` | `v0.2.0` | `v0.2.0` |
| thread_system | `v0.3.0` | `v0.3.0` | `v0.3.0` |
| container_system | `v0.1.0` | `v0.1.0` | `v0.1.0` |
| logger_system | `v0.1.0` | `v0.1.0` | `v0.1.0` |
| monitoring_system | `v0.1.0` | `v0.1.0` | `v0.1.0` |
| database_system | `v0.1.0` | `v0.1.0` | `v0.1.0` |
| network_system | `v0.1.0` | `v0.1.0` | `v0.1.0` |
| pacs_system | — (no release yet) | — (pending first tag) | — (pending first tag) |

> Update this table after each tagged release per [VERSIONING.md § Ecosystem Compatibility Matrix](./VERSIONING.md).
> Tracking issue: [#401](https://github.com/kcenon/common_system/issues/401)
> Verified against GitHub releases/tags on 2026-03-14 (Asia/Seoul).

## Maintenance

This matrix should be updated when:
- A dependency version is added, changed, or removed in any system's `vcpkg.json`
- A new system is added to the ecosystem
- A version discrepancy is resolved
- An internal ecosystem library publishes a new tagged release

---

*Part of the SBOM improvement initiative: [common_system#390](https://github.com/kcenon/common_system/issues/390)*
*Generated from SBOM analysis on 2026-03-06*
