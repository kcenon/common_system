# Ecosystem Integration

## Position in kcenon Ecosystem

**common_system** is the foundation layer of the kcenon ecosystem. It provides core interfaces, design patterns, and shared utilities that all other systems depend on. No other system is required to use common_system.

## Dependencies

| System | Relationship |
|--------|-------------|
| (none) | common_system has no ecosystem dependencies — it is the root |

## Dependent Systems

| System | How It Uses common_system |
|--------|--------------------------|
| thread_system | Core interfaces for thread management and scheduling |
| logger_system | Base patterns for logging infrastructure |
| container_system | Shared utilities for type-safe containers |
| monitoring_system | Foundation interfaces for metrics and tracing |
| database_system | Core patterns for database abstraction |
| network_system | Base interfaces for networking protocols |
| pacs_system | All foundation types and utilities |

## All Ecosystem Systems

| System | Description | Docs |
|--------|------------|------|
| common_system | Foundation — interfaces, patterns, utilities | [Docs](https://kcenon.github.io/common_system/) |
| thread_system | High-performance thread pool, DAG scheduling | [Docs](https://kcenon.github.io/thread_system/) |
| logger_system | Async logging, decorators, OpenTelemetry | [Docs](https://kcenon.github.io/logger_system/) |
| container_system | Type-safe containers, SIMD serialization | [Docs](https://kcenon.github.io/container_system/) |
| monitoring_system | Metrics, tracing, alerts, plugins | [Docs](https://kcenon.github.io/monitoring_system/) |
| database_system | Multi-backend DB (PostgreSQL, SQLite, MongoDB, Redis) | [Docs](https://kcenon.github.io/database_system/) |
| network_system | TCP/UDP/WebSocket/HTTP2/QUIC/gRPC networking | [Docs](https://kcenon.github.io/network_system/) |
| pacs_system | DICOM medical imaging (12 libraries) | [Docs](https://kcenon.github.io/pacs_system/) |

## Ecosystem Overview

See the [Ecosystem Overview](https://kcenon.github.io/common_system/) in common_system for the complete dependency map and system selection guide.
