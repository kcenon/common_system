---
doc_id: "COM-PROJ-012"
doc_title: "SOUP Inventory — kcenon Ecosystem"
doc_version: "1.0.0"
doc_date: "2026-04-04"
doc_status: "Released"
project: "common_system"
category: "PROJ"
---

# SOUP Inventory — kcenon Ecosystem

> **SOUP** = Software of Unknown Provenance (IEC 62304 §8.1.2)
>
> This document catalogs all third-party dependencies used across the kcenon
> ecosystem projects. Each entry identifies the component, its license, purpose,
> version constraints, and risk classification.

## Ecosystem Overview

All kcenon projects are licensed under **BSD-3-Clause**. Third-party dependencies
must be license-compatible with BSD-3-Clause.

| Project | Repository | Core SOUP | Optional SOUP |
|---------|-----------|-----------|---------------|
| common_system | kcenon/common_system | None | yaml-cpp, gtest, benchmark |
| thread_system | kcenon/thread_system | libiconv | spdlog, gtest, benchmark |
| network_system | kcenon/network_system | asio, zlib | openssl, grpc, protobuf, gtest, benchmark |
| logger_system | kcenon/logger_system | None | openssl, spdlog, opentelemetry-cpp, protobuf, grpc, gtest, benchmark |
| container_system | kcenon/container_system | None | grpc, protobuf, gtest, benchmark |
| database_system | kcenon/database_system | asio | libmariadb, libpq, libpqxx, sqlite3, mongo-cxx-driver, hiredis, openssl, spdlog, gtest, benchmark |
| monitoring_system | kcenon/monitoring_system | None | grpc, protobuf, gtest |

> **fmt** has been fully removed from the ecosystem. All projects now use C++20 `std::format`.
> Migration completed per [#406](https://github.com/kcenon/common_system/issues/406).

## SOUP Catalog

### Critical Risk

Components whose failure may cause data loss, security breach, or system
unavailability.

#### OpenSSL

| Field | Value |
|-------|-------|
| Name | OpenSSL |
| SPDX License | Apache-2.0 |
| Minimum Version | 3.0.0 |
| Projects | network_system (ssl feature), database_system (postgresql feature), logger_system (encryption feature) |
| Purpose | TLS/SSL encryption for secure network communication |
| Linking | Dynamic (shared library) |
| BSD-3 Compatible | Yes |
| Risk Classification | **Critical** |
| Anomaly Impact | Loss of encryption → data exposure; certificate validation failure → MITM attacks |
| Notes | OpenSSL 1.1.1 reached EOL 2023-09-11. Only 3.x is supported. |

### High Risk

Components critical to core functionality whose failure degrades primary
features.

#### ASIO (Standalone)

| Field | Value |
|-------|-------|
| Name | Standalone Asio C++ Library |
| SPDX License | BSL-1.0 |
| Minimum Version | 1.29.0 (database_system), 1.30.2 (network_system) |
| Projects | network_system (core), database_system (core) |
| Purpose | Asynchronous I/O, networking, timers |
| Linking | Header-only |
| BSD-3 Compatible | Yes |
| Risk Classification | **High** |
| Anomaly Impact | Network I/O failure → complete communication breakdown |
| Notes | network_system pins to 1.30.2 (FetchContent fallback tag: asio-1-30-2). Boost.ASIO is NOT compatible (different include path). |

#### libmariadb (MariaDB Connector/C)

| Field | Value |
|-------|-------|
| Name | MariaDB Connector/C |
| SPDX License | LGPL-2.1-or-later |
| Projects | database_system (mysql feature) |
| Purpose | MySQL/MariaDB database connectivity (wire-compatible MySQL replacement) |
| Linking | **Dynamic only** (LGPL compliance) |
| BSD-3 Compatible | Yes (dynamic linking) |
| Risk Classification | **High** |
| Anomaly Impact | Database connection failure → data unavailability |
| LGPL Policy | Must be dynamically linked. Static linking requires providing object files for re-linking. See database_system LICENSE-THIRD-PARTY. |
| Notes | Replaced libmysql (GPL-2.0) in 2026-03. Provides `mysql.h` compatibility header — no code changes needed. |

#### libpq / libpqxx

| Field | Value |
|-------|-------|
| Name | PostgreSQL client library (libpq) + C++ wrapper (libpqxx) |
| SPDX License | PostgreSQL (libpq), BSD-3-Clause (libpqxx) |
| Projects | database_system (postgresql feature) |
| Purpose | PostgreSQL database connectivity |
| Linking | Dynamic |
| BSD-3 Compatible | Yes |
| Risk Classification | **High** |
| Anomaly Impact | Database connection failure → data unavailability |

#### mongo-cxx-driver

| Field | Value |
|-------|-------|
| Name | MongoDB C++ Driver |
| SPDX License | Apache-2.0 |
| Minimum Version | 3.8.0 |
| Projects | database_system (mongodb feature) |
| Purpose | MongoDB database connectivity |
| Linking | Dynamic |
| BSD-3 Compatible | Yes |
| Risk Classification | **High** |
| Anomaly Impact | Database connection failure → data unavailability |
| Notes | Pulls in libbson 1.26.2 and mongo-c-driver 1.26.2 transitively. mongo-c-driver further depends on zlib and OpenSSL on non-Windows/non-Apple builds. |

#### gRPC

| Field | Value |
|-------|-------|
| Name | gRPC C++ |
| SPDX License | Apache-2.0 |
| Minimum Version | 1.51.1 |
| Projects | container_system (grpc module), logger_system (otlp feature), monitoring_system (grpc feature), network_system (network-grpc / official gRPC mode) |
| Purpose | High-performance RPC framework for container RPC, telemetry export, and monitoring |
| Linking | Dynamic |
| BSD-3 Compatible | Yes |
| Risk Classification | **High** |
| Anomaly Impact | Telemetry/monitoring data loss; observability degradation |
| Notes | Pulls in protobuf, OpenSSL, and zlib directly plus abseil 20240116.1, c-ares 1.28.1, re2 2024-04-01, and upb 2022-06-21 transitively from the pinned vcpkg baseline. |

### Medium Risk

Components whose failure causes reduced functionality but does not compromise
core operations.

#### zlib

| Field | Value |
|-------|-------|
| Name | zlib compression library |
| SPDX License | Zlib |
| Minimum Version | 1.3 (CVE-2023-45853 fix) |
| Projects | network_system (core) |
| Purpose | HTTP compression support |
| Linking | Dynamic |
| BSD-3 Compatible | Yes |
| Risk Classification | **Medium** |
| Anomaly Impact | Compression failure → degraded network performance |

#### protobuf

| Field | Value |
|-------|-------|
| Name | Protocol Buffers |
| SPDX License | BSD-3-Clause |
| Minimum Version | 3.21.0 |
| Projects | container_system (grpc module), logger_system (otlp feature), monitoring_system (grpc feature), network_system (network-grpc / official gRPC mode) |
| Purpose | Serialization format for container RPC, gRPC, and telemetry |
| Linking | Dynamic |
| BSD-3 Compatible | Yes |
| Risk Classification | **Medium** |
| Anomaly Impact | Serialization failure → telemetry data loss |

#### sqlite3

| Field | Value |
|-------|-------|
| Name | SQLite |
| SPDX License | Public Domain |
| Minimum Version | 3.45.0 |
| Projects | database_system (sqlite feature) |
| Purpose | Embedded SQL database |
| Linking | Static or dynamic |
| BSD-3 Compatible | Yes |
| Risk Classification | **Medium** |
| Anomaly Impact | Database corruption → local data loss |

#### hiredis

| Field | Value |
|-------|-------|
| Name | Hiredis |
| SPDX License | BSD-3-Clause |
| Minimum Version | 1.2.0 |
| Projects | database_system (redis feature) |
| Purpose | Redis client library |
| Linking | Dynamic |
| BSD-3 Compatible | Yes |
| Risk Classification | **Medium** |
| Anomaly Impact | Cache connection failure → performance degradation |

#### opentelemetry-cpp

| Field | Value |
|-------|-------|
| Name | OpenTelemetry C++ SDK |
| SPDX License | Apache-2.0 |
| Minimum Version | 1.14.0 |
| Projects | logger_system (otlp feature) |
| Purpose | Distributed tracing and metrics export via OTLP |
| Linking | Dynamic |
| BSD-3 Compatible | Yes |
| Risk Classification | **Medium** |
| Anomaly Impact | Observability data loss; no impact on core functionality |
| Notes | Base port depends on abseil 20240116.1 and nlohmann-json 3.11.3. The logger_system otlp-http feature also pulls in curl 8.7.1, while otlp-grpc reuses the gRPC dependency chain documented above. |

### Verified Transitive Dependencies

These packages are not declared directly in the affected ecosystem manifests, but
they are pulled in through pinned third-party ports at the shared vcpkg baseline
`c4af3593e1f1aa9e14a560a09e45ea2cb0dfd74d`. Direct SOUP entries already cataloged
above (for example `protobuf`, `openssl`, and `zlib`) are not duplicated here.

| Dependency | Resolved Version | SPDX License | Transitive Source | Projects | Purpose | BSD-3 Compatible |
|------------|------------------|--------------|-------------------|----------|---------|------------------|
| abseil | 20240116.1 | Apache-2.0 | gRPC 1.51.1; opentelemetry-cpp 1.14.2 | container_system, logger_system, monitoring_system, network_system | Common utility layer used by gRPC and OpenTelemetry C++ | Yes |
| c-ares | 1.28.1 | MIT-CMU | gRPC 1.51.1 | container_system, logger_system, monitoring_system, network_system | Async DNS resolver used by gRPC | Yes |
| re2 | 2024-04-01 | BSD-3-Clause | gRPC 1.51.1 | container_system, logger_system, monitoring_system, network_system | Regular expression engine used by gRPC | Yes |
| upb | 2022-06-21 | BSD-2-Clause | gRPC 1.51.1 | container_system, logger_system, monitoring_system, network_system | Lightweight protobuf runtime/code generator used by gRPC | Yes |
| nlohmann-json | 3.11.3 | MIT | opentelemetry-cpp 1.14.2 | logger_system | JSON serialization support for OTLP exporters | Yes |
| curl | 8.7.1 | curl AND ISC AND BSD-3-Clause | opentelemetry-cpp 1.14.2 (`otlp-http`) | logger_system | HTTP transport for OTLP export | Yes |
| mongo-c-driver | 1.26.2 | Apache-2.0 | mongo-cxx-driver 3.10.1 | database_system | C driver layer underlying mongocxx | Yes |
| libbson | 1.26.2 | Apache-2.0 | mongo-cxx-driver 3.10.1; mongo-c-driver 1.26.2 | database_system | BSON parsing and encoding layer used by the MongoDB drivers | Yes |

Compatibility notes:

- `curl` uses a permissive composite license expression and remains BSD-3-Clause compatible.
- `mongo-c-driver` and `libbson` use the upstream Apache-2.0 `COPYING` file in the MongoDB C driver repository.
- `upb` resolves to BSD-2-Clause at the pinned baseline, which is permissive and BSD-3-Clause compatible.

### Low Risk

Components whose failure has minimal operational impact.

#### yaml-cpp

| Field | Value |
|-------|-------|
| Name | yaml-cpp |
| SPDX License | MIT |
| Minimum Version | 0.9.0 |
| Projects | common_system (yaml config feature, optional) |
| Purpose | YAML configuration file parsing |
| Linking | Static or dynamic |
| BSD-3 Compatible | Yes |
| Risk Classification | **Low** |
| Anomaly Impact | YAML config unavailable; fallback to other config methods |
| Notes | Optional feature, gated by `BUILD_WITH_YAML_CPP` CMake option. Detected at build time via `find_package(yaml-cpp QUIET)`. |

#### libiconv

| Field | Value |
|-------|-------|
| Name | GNU libiconv |
| SPDX License | LGPL-2.1-or-later |
| Projects | thread_system (core, non-Windows) |
| Purpose | Character encoding conversion (wide/narrow strings) |
| Linking | **Dynamic only** (LGPL compliance) |
| BSD-3 Compatible | Yes (dynamic linking) |
| Risk Classification | **Low** |
| Anomaly Impact | Encoding fallback → limited character conversion |
| LGPL Policy | Must be dynamically linked. macOS provides as system framework (always dynamic). Linux glibc includes iconv natively. Windows excluded (`"platform": "!windows"`). See thread_system LICENSE-THIRD-PARTY. |

#### spdlog

| Field | Value |
|-------|-------|
| Name | spdlog |
| SPDX License | MIT |
| Minimum Version | 1.13.0 (logger_system benchmarks) |
| Projects | thread_system (logging feature), database_system (logging feature), logger_system (benchmarks feature) |
| Purpose | Fast C++ logging library (optional, for benchmarks/development) |
| Linking | Header-only or shared |
| BSD-3 Compatible | Yes |
| Risk Classification | **Low** |
| Anomaly Impact | Logging degradation; no impact on core functionality |

### Test-Only (N/A Risk)

These dependencies are used exclusively during development and testing. They are
not distributed with production binaries.

#### gtest / gmock

| Field | Value |
|-------|-------|
| Name | Google Test / Google Mock |
| SPDX License | BSD-3-Clause |
| Minimum Version | 1.14.0 (common_system) |
| Projects | All ecosystem projects (testing feature) |
| Purpose | Unit testing and mocking framework |
| BSD-3 Compatible | Yes |

#### benchmark

| Field | Value |
|-------|-------|
| Name | Google Benchmark |
| SPDX License | Apache-2.0 |
| Projects | All ecosystem projects except monitoring_system (testing feature) |
| Purpose | Micro-benchmarking framework |
| BSD-3 Compatible | Yes |

## LGPL Compliance Summary

Two SOUP items use LGPL-2.1 and require dynamic linking:

| SOUP | Project | Compliance Mechanism |
|------|---------|---------------------|
| libiconv | thread_system | CMake build-time `get_target_property(TYPE)` check warns on static linking. macOS/glibc provide natively. Windows excluded. |
| libmariadb | database_system | vcpkg triplet overlay forces dynamic linking. CMake build-time check + CI `ldd`/`otool -L` verification. Replaced GPL-2.0 libmysql in 2026-03. |

**Policy**: LGPL-2.1 components must always be dynamically linked to preserve
BSD-3-Clause licensing of kcenon code. Static linking would impose copyleft
obligations on the entire project.

## License Compatibility Matrix

| License | Type | BSD-3 Compatible | Commercial Use | Patent Grant |
|---------|------|:----------------:|:--------------:|:------------:|
| MIT | Permissive | Yes | Yes | No |
| BSD-3-Clause | Permissive | Yes | Yes | No |
| BSL-1.0 | Permissive | Yes | Yes | No |
| Zlib | Permissive | Yes | Yes | No |
| PostgreSQL | Permissive | Yes | Yes | No |
| Public Domain | Unrestricted | Yes | Yes | N/A |
| Apache-2.0 | Permissive | Yes | Yes | Yes |
| LGPL-2.1 | Weak Copyleft | Yes (dynamic) | Yes | No |
| GPL-2.0 | Strong Copyleft | **No** | Restricted | No |

> **Note**: GPL-2.0 dependencies are prohibited in the kcenon ecosystem.
> The libmysql → libmariadb migration (2026-03) eliminated the last GPL dependency.

## Maintenance

| Field | Value |
|-------|-------|
| Last Review | 2026-03-09 |
| Next Review | 2026-06-09 |
| Review Cadence | Quarterly |
| Responsible | Backend Developer |

### Review Checklist

- [ ] Verify all vcpkg.json dependencies are listed
- [ ] Verify transitive chains for gRPC, OpenTelemetry OTLP, and mongo-cxx-driver against the pinned vcpkg baseline
- [ ] Confirm version constraints match actual vcpkg.json contents
- [ ] Check for new CVEs against listed SOUP versions
- [ ] Validate LGPL dynamic linking compliance
- [ ] Update risk classifications if usage patterns changed

---

*Last Updated: 2026-03-09*
