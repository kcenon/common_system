# kcenon Ecosystem Overview

## System Map

```
common_system (Foundation — interfaces, patterns, utilities)
├── thread_system (High-performance thread pool, DAG scheduling)
├── logger_system (Async logging, decorators, OpenTelemetry)
├── container_system (Type-safe containers, SIMD serialization)
├── monitoring_system (Metrics, tracing, alerts, plugins)
├── database_system (Multi-backend DB: PostgreSQL, SQLite, MongoDB, Redis)
├── network_system (TCP/UDP/WebSocket/HTTP2/QUIC/gRPC)
└── pacs_system (DICOM medical imaging, 12 libraries)
```

## Which System Do I Need?

| Use Case | Start Here | Then Add |
|----------|-----------|----------|
| Building a server | network_system | container_system, logger_system |
| Need logging | logger_system | monitoring_system |
| Thread pool / async work | thread_system | common_system |
| Database access | database_system | common_system |
| System monitoring | monitoring_system | logger_system |
| Data serialization | container_system | common_system |
| Medical imaging (DICOM) | pacs_system | (includes everything) |

## Documentation Links

| System | GitHub Pages | Repository |
|--------|------------|------------|
| common_system | https://kcenon.github.io/common_system/ | https://github.com/kcenon/common_system |
| thread_system | https://kcenon.github.io/thread_system/ | https://github.com/kcenon/thread_system |
| logger_system | https://kcenon.github.io/logger_system/ | https://github.com/kcenon/logger_system |
| container_system | https://kcenon.github.io/container_system/ | https://github.com/kcenon/container_system |
| monitoring_system | https://kcenon.github.io/monitoring_system/ | https://github.com/kcenon/monitoring_system |
| database_system | https://kcenon.github.io/database_system/ | https://github.com/kcenon/database_system |
| network_system | https://kcenon.github.io/network_system/ | https://github.com/kcenon/network_system |
| pacs_system | https://kcenon.github.io/pacs_system/ | https://github.com/kcenon/pacs_system |
