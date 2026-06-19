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

## Versions

Known-good baseline for the kcenon ecosystem as published through the [`kcenon/vcpkg-registry`](https://github.com/kcenon/vcpkg-registry) overlay. Downstream consumers should lock to this baseline (see [Reproducing the ecosystem baseline](#reproducing-the-ecosystem-baseline) below) to avoid mixing versions that were not tested together.

### Version matrix

| Tier | vcpkg port | Current version | Depends on (kcenon) |
|------|-----------|-----------------|---------------------|
| 0 | `kcenon-common-system` | `0.2.0` | — |
| 1 | `kcenon-thread-system` | `0.3.2` | `kcenon-common-system` |
| 1 | `kcenon-container-system` | `0.1.0` | `kcenon-common-system` |
| 2 | `kcenon-logger-system` | `0.1.3` | `kcenon-common-system` |
| 3 | `kcenon-database-system` | `0.1.1` | `kcenon-common-system` |
| 3 | `kcenon-monitoring-system` | `0.1.0` | `kcenon-common-system`, `kcenon-thread-system` |
| 4 | `kcenon-network-system` | `0.1.1` | `kcenon-common-system`, `kcenon-thread-system` |
| 5 | `kcenon-pacs-system` | `0.1.0` | `kcenon-common-system`, `kcenon-container-system`, `kcenon-logger-system`, `kcenon-network-system`, `kcenon-thread-system` |

Tier classification follows `docs/ARCHITECTURE.md` (Tier 0 Foundation → Tier 5 Application). Tiers 0–2 are the stable core that downstream ecosystem components pin to; tiers 3–5 are higher-level systems that advance independently.

### Baseline SHA

Pin the overlay registry to commit [`b3fe244d03`](https://github.com/kcenon/vcpkg-registry/commit/b3fe244d03) (2026-04-10) for the version set above. Newer SHAs may bump port versions — audit the delta before advancing.

The authoritative version table is [`versions/baseline.json`](https://github.com/kcenon/vcpkg-registry/blob/main/versions/baseline.json) in the registry; the table above is a human-readable snapshot that is refreshed when the registry advances.

### Reproducing the ecosystem baseline

Add the registry overlay and pin it in your `vcpkg-configuration.json`:

```json
{
  "default-registry": {
    "kind": "git",
    "repository": "https://github.com/microsoft/vcpkg",
    "baseline": "<your chosen vcpkg baseline>"
  },
  "registries": [
    {
      "kind": "git",
      "repository": "https://github.com/kcenon/vcpkg-registry",
      "baseline": "b3fe244d03",
      "packages": [
        "kcenon-common-system",
        "kcenon-thread-system",
        "kcenon-container-system",
        "kcenon-logger-system",
        "kcenon-monitoring-system",
        "kcenon-database-system",
        "kcenon-network-system",
        "kcenon-pacs-system"
      ]
    }
  ]
}
```

Then depend on the ports you need from `vcpkg.json`:

```json
{
  "name": "my-app",
  "version": "0.1.0",
  "dependencies": [
    "kcenon-common-system",
    "kcenon-logger-system"
  ]
}
```

Vcpkg will resolve the full transitive dependency closure using the pinned baseline — you do not need to list `kcenon-thread-system` or other transitive deps by hand.

### How versions advance

New port versions land via the [`kcenon/vcpkg-registry`](https://github.com/kcenon/vcpkg-registry) repository. The source of truth for version bumps is:

- Each upstream repository's `release.yml` workflow — builds and validates the release artifact.
- The registry's `sync-vcpkg-registry.yml` workflow — publishes the new version, updates `versions/baseline.json`, and advances the overlay registry HEAD.

Consumers who have pinned a baseline SHA will not pick up new versions until they explicitly advance the baseline in their `vcpkg-configuration.json`. Until then, the ecosystem composition above is reproducible.

Part of [common_system#646](https://github.com/kcenon/common_system/issues/646) — Ecosystem-wide vcpkg distribution readiness EPIC.
