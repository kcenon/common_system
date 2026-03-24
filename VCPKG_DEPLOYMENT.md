# vcpkg Deployment Guide

Single source of truth for the kcenon ecosystem's vcpkg configuration strategy,
version matrix, baseline update protocol, and CI integration.

## Table of Contents

1. [Ecosystem Overview](#1-ecosystem-overview)
2. [Configuration Standard](#2-configuration-standard)
3. [Version Matrix](#3-version-matrix)
4. [Baseline Update Protocol](#4-baseline-update-protocol)
5. [CI Integration](#5-ci-integration)
6. [Troubleshooting](#6-troubleshooting)

---

## 1. Ecosystem Overview

### Dependency Hierarchy

The ecosystem is organized into tiers. Higher tiers depend on lower tiers;
lower tiers never depend on higher ones.

```
Tier 0 (Foundation)
  common_system ................. Result<T>, interfaces, utilities

Tier 1 (Core Services)
  thread_system ................. Multithreading framework
  logger_system ................. Async logging, file rotation
  container_system .............. Thread-safe serializable containers

Tier 2 (Infrastructure)
  monitoring_system ............. Metrics, tracing, alerting
  network_system ................ Async network messaging
  database_system ............... Multi-backend database layer

Tier 3 (Messaging)
  messaging_system .............. Pub/sub, request/reply, event streaming

Tier 4 (Domain)
  pacs_system ................... DICOM medical imaging

Tier 5 (Applications)
  pacs_bridge ................... HL7-DICOM bridge
```

### Project Directory

| Tier | Project | Package Name | Version | Description |
|------|---------|-------------|---------|-------------|
| 0 | common_system | `kcenon-common-system` | 0.2.0 | Foundation library |
| 1 | thread_system | `kcenon-thread-system` | 0.3.1 | Multithreading framework |
| 1 | logger_system | `kcenon-logger-system` | 0.1.3 | Async logging library |
| 1 | container_system | `kcenon-container-system` | 0.1.0 | Thread-safe containers |
| 2 | monitoring_system | `kcenon-monitoring-system` | 0.1.0 | Monitoring and tracing |
| 2 | network_system | `kcenon-network-system` | 0.1.1 | Network messaging |
| 2 | database_system | `kcenon-database-system` | 0.1.0 | Database abstraction |
| 3 | messaging_system | `kcenon-messaging-system` | 1.0.0 | Messaging infrastructure |
| 4 | pacs_system | `kcenon-pacs-system` | 0.1.0 | DICOM PACS |
| 5 | pacs_bridge | `pacs-bridge` | 0.2.0 | HL7-DICOM bridge |

### Dependency Graph

```
common_system (Tier 0)
  |
  +-- thread_system (Tier 1)
  |     |
  |     +-- monitoring_system (Tier 2)
  |     +-- network_system (Tier 2)
  |
  +-- logger_system (Tier 1)
  |
  +-- container_system (Tier 1)
  |     |
  |     +-- network_system (Tier 2)
  |     +-- pacs_system (Tier 4)
  |
  +-- network_system (Tier 2)
  |     |
  |     +-- pacs_system (Tier 4)
  |
  +-- database_system (Tier 2)
  |
  +-- messaging_system (Tier 3)
  |
  +-- pacs_system (Tier 4)
        |
        +-- pacs_bridge (Tier 5)
```

---

## 2. Configuration Standard

### vcpkg-configuration.json Template

Every project in the ecosystem MUST use the following `vcpkg-configuration.json`:

```json
{
  "$schema": "https://raw.githubusercontent.com/microsoft/vcpkg-tool/main/docs/vcpkg-configuration.schema.json",
  "default-registry": {
    "kind": "builtin",
    "baseline": "d90a9b159c08169f39adcd1b0f1ac0ca12c4b96c"
  },
  "registries": [
    {
      "kind": "git",
      "repository": "https://github.com/kcenon/vcpkg-registry.git",
      "baseline": "77cc46d5ba5e2aef1581f2ec674f83e1ac906b43",
      "packages": [
        "kcenon-*"
      ]
    }
  ]
}
```

### Registry Resolution

vcpkg resolves package names using a two-registry strategy:

1. **Default registry** (`builtin`) -- The official microsoft/vcpkg repository, pinned to a
   specific commit SHA. All standard open-source packages (gtest, asio, openssl, etc.)
   are resolved from here.

2. **Custom registry** (`kcenon/vcpkg-registry`) -- A Git-based registry that provides
   ecosystem packages. The `"packages": ["kcenon-*"]` pattern routes any package whose
   name starts with `kcenon-` to this registry.

### How kcenon-* Packages Are Resolved

When a project declares a dependency like `"kcenon-common-system"`:

1. vcpkg checks the `packages` patterns in each registry entry.
2. The `kcenon-*` wildcard matches, so vcpkg fetches the port from
   `https://github.com/kcenon/vcpkg-registry.git` at the specified baseline.
3. The port definition in that registry points to the corresponding GitHub release.
4. All non-`kcenon-*` packages fall through to the builtin (microsoft/vcpkg) registry.

### Baseline Consistency Rule

All projects MUST share the same two baselines:

| Registry | Baseline | Purpose |
|----------|----------|---------|
| `builtin` (microsoft/vcpkg) | `d90a9b159c08169f39adcd1b0f1ac0ca12c4b96c` | Pins third-party package versions |
| `kcenon/vcpkg-registry` | `77cc46d5ba5e2aef1581f2ec674f83e1ac906b43` | Pins ecosystem package versions |

Mismatched baselines across projects will cause version conflicts in CI and local builds.

---

## 3. Version Matrix

### Shared Overrides

All projects pin specific versions via the `"overrides"` array in `vcpkg.json`.
The table below shows every overridden package and which projects use it.

| Package | Version | common | thread | logger | container | monitoring | network | database | messaging | pacs | pacs_bridge |
|---------|---------|--------|--------|--------|-----------|------------|---------|----------|-----------|------|-------------|
| gtest | 1.17.0 | x | x | x | x | x | x | x | x | x | x |
| benchmark | 1.9.5 | x | x | x | x | x | x | x | x | x | x |
| spdlog | 1.15.3 | | x | x | | | | x | x | | x |
| openssl | 3.4.1 | | | x | | | x | x | x | x | x |
| asio | 1.30.2 | | | | | | x | x | x | x | |
| grpc | 1.60.0 | | | x | x | x | x | x | | | |
| protobuf | 4.25.1 | | | x | x | x | x | x | | | |
| simdutf | 5.2.5 | | x | | | | x | | | | |
| zlib | 1.3.1 | | | | | | x | | | | |
| lz4 | 1.9.4 | | | | | | x | | | | |
| opentelemetry-cpp | 1.18.0 | | | x | | | | | | | |
| sqlite3 | 3.45.3 | | | | | | | x | | x | |
| libpq | 16.2 | | | | | | | x | | | |
| libpqxx | 7.9.0 | | | | | | | x | | | |
| mongo-cxx-driver | 3.10.1 | | | | | | | x | | | |
| hiredis | 1.2.0 | | | | | | | x | | | |
| libjpeg-turbo | 3.0.2 | | | | | | | | | x | |
| libpng | 1.6.43 | | | | | | | | | x | |
| openjpeg | 2.5.2 | | | | | | | | | x | |
| charls | 2.4.2 | | | | | | | | | x | |
| openjph | 0.21.0 | | | | | | | | | x | |
| crow | 1.2.1 | | | | | | | | | x | |

### Consistency Requirements

1. **Universal overrides**: `gtest` and `benchmark` MUST be overridden in every project
   to the same version.
2. **Shared overrides**: When two or more projects override the same package, the
   version MUST be identical. For example, `spdlog 1.15.3` is used by thread_system,
   logger_system, database_system, messaging_system, and pacs_bridge.
3. **New override rule**: When adding a new override, check the matrix above. If the
   package already appears, use the existing version unless there is a documented
   reason to upgrade (which requires updating all projects simultaneously).

---

## 4. Baseline Update Protocol

### When to Update Baselines

- A new vcpkg release includes a critical security fix for a package the ecosystem uses.
- A package upgrade is needed and requires a newer baseline to resolve.
- A new ecosystem package is published to `kcenon/vcpkg-registry`.

### Step-by-Step Procedure

#### Phase 1: Preparation

1. Identify the target vcpkg commit SHA (for builtin baseline) or registry commit SHA.
2. Review the changelog for breaking changes in packages the ecosystem depends on.
3. Create a tracking issue listing all projects that need updating.

#### Phase 2: Test in Isolation

4. Start with the lowest-tier project (`common_system`).
5. Create a branch: `chore/update-vcpkg-baseline-YYYYMMDD`.
6. Update `vcpkg-configuration.json` with the new baseline(s).
7. Update any `"overrides"` in `vcpkg.json` if package versions changed.
8. Build and run all tests locally on all supported platforms (or rely on CI).
9. Fix any build failures before proceeding.

#### Phase 3: Coordinated Roll-Out (Tier 0 -> 5)

10. Merge the `common_system` baseline update first.
11. Publish the updated `common_system` port to `kcenon/vcpkg-registry` if needed.
12. Proceed to Tier 1 projects (`thread_system`, `logger_system`, `container_system`).
    Each project gets the same baseline update + any override adjustments.
13. Continue through Tier 2, 3, 4, 5 in order.
14. At each tier, wait for CI to pass before moving to the next tier.

#### Phase 4: Verification

15. After all projects are updated, verify cross-project integration.
16. Close the tracking issue.

### Rollback Procedure

If a baseline update causes failures that cannot be quickly resolved:

1. Revert the `vcpkg-configuration.json` change in the failing project.
2. If the failure is in a higher-tier project, the lower-tier updates can remain
   as long as they are independently stable.
3. If the failure is in `common_system` or the registry baseline, revert all
   projects in reverse tier order (Tier 5 -> 0).
4. Document the failure reason in the tracking issue.

---

## 5. CI Integration

### setup-vcpkg Composite Action

The ecosystem provides a reusable GitHub Actions composite action at:

```
kcenon/common_system/.github/actions/setup-vcpkg@main
```

This action bootstraps vcpkg pinned to the ecosystem baseline with intelligent caching.

### Basic Usage

```yaml
steps:
  - uses: actions/checkout@v4
  - uses: kcenon/common_system/.github/actions/setup-vcpkg@main
  - run: cmake -B build -DCMAKE_TOOLCHAIN_FILE=$CMAKE_TOOLCHAIN_FILE
```

### Input Parameters

| Input | Required | Default | Description |
|-------|----------|---------|-------------|
| `vcpkg-commit` | No | `d90a9b159c08169f39adcd1b0f1ac0ca12c4b96c` | vcpkg commit SHA to checkout |
| `manifest-dir` | No | `.` | Directory containing `vcpkg.json` and `vcpkg-configuration.json` |
| `extra-cache-key` | No | `''` | Additional cache key suffix for project differentiation |

### Outputs

| Output | Description |
|--------|-------------|
| `vcpkg-root` | Path to the vcpkg installation directory |
| `toolchain-file` | Path to `vcpkg.cmake` toolchain file |

### Environment Variables Set

After the action runs, these variables are available to all subsequent steps:

- `VCPKG_ROOT` -- Path to the vcpkg installation
- `CMAKE_TOOLCHAIN_FILE` -- Path to `vcpkg.cmake`

### Cache Strategy

The action caches the vcpkg installation directory (excluding `buildtrees/`,
`packages/`, and `downloads/`). The cache key includes:

- Runner OS
- vcpkg commit SHA
- Hash of `vcpkg.json` + `vcpkg-configuration.json`
- Optional `extra-cache-key` suffix

Cache hits skip the clone and bootstrap steps entirely.

### Migration from Inline Setup

If a project currently bootstraps vcpkg inline in its workflow, replace:

```yaml
# Before (inline setup)
- name: Clone vcpkg
  run: |
    git clone https://github.com/microsoft/vcpkg.git
    cd vcpkg && git checkout d90a9b159c08169f39adcd1b0f1ac0ca12c4b96c
    ./bootstrap-vcpkg.sh -disableMetrics
- name: Set environment
  run: |
    echo "VCPKG_ROOT=${{ github.workspace }}/vcpkg" >> $GITHUB_ENV
    echo "CMAKE_TOOLCHAIN_FILE=${{ github.workspace }}/vcpkg/scripts/buildsystems/vcpkg.cmake" >> $GITHUB_ENV
```

With:

```yaml
# After (composite action)
- uses: kcenon/common_system/.github/actions/setup-vcpkg@main
```

Benefits: automatic caching, single source of truth for the baseline commit,
cross-platform support (Linux, macOS, Windows).

### Using with Extra Cache Key

When multiple projects share a CI runner or when you need to differentiate caches:

```yaml
- uses: kcenon/common_system/.github/actions/setup-vcpkg@main
  with:
    extra-cache-key: 'my-project'
```

---

## 6. Troubleshooting

### Common vcpkg Resolution Errors

#### "no version database entry" for kcenon-* packages

**Cause**: The custom registry baseline is outdated or the package has not been published.

**Fix**:
1. Verify the package exists in `kcenon/vcpkg-registry` at the expected baseline.
2. Update the registry baseline in `vcpkg-configuration.json` if a new version was
   published after the current baseline.

#### "version conflict" between two packages

**Cause**: Two packages require incompatible versions of a transitive dependency,
or an override is missing.

**Fix**:
1. Add the conflicting package to the `"overrides"` array in `vcpkg.json`.
2. Pin it to the version that satisfies all dependents.
3. Add the same override version to all ecosystem projects (see Version Matrix).

#### "baseline does not contain entry for port"

**Cause**: The builtin baseline is too old to contain the requested package or version.

**Fix**:
1. Update the builtin baseline to a newer vcpkg commit that includes the package.
2. Follow the Baseline Update Protocol (Section 4).

### Cache Invalidation

#### When to invalidate

- After updating `vcpkg-configuration.json` baselines.
- After modifying `vcpkg.json` overrides or dependencies.
- When CI builds fail with stale package errors.

#### How to invalidate

**GitHub Actions**: The cache key includes the hash of `vcpkg.json` and
`vcpkg-configuration.json`, so changes to these files automatically bust the cache.

**Manual invalidation**: Change the `extra-cache-key` input or delete caches via
the GitHub Actions cache management UI (Settings > Actions > Caches).

**Local builds**: Delete the vcpkg `installed/` directory:
```bash
rm -rf build/vcpkg_installed
```

### Registry Sync Issues

#### Custom registry not resolving latest versions

**Cause**: The registry baseline in `vcpkg-configuration.json` points to an older commit.

**Fix**:
1. Check the latest commit in `kcenon/vcpkg-registry`:
   ```bash
   git ls-remote https://github.com/kcenon/vcpkg-registry.git HEAD
   ```
2. Update the registry baseline across all projects (follow Section 4).

#### Authentication errors when cloning registry

**Cause**: `kcenon/vcpkg-registry` is a public repository, so this is usually
a network or proxy issue rather than an authentication problem.

**Fix**:
1. Verify you can access the repository: `git ls-remote https://github.com/kcenon/vcpkg-registry.git`
2. Check proxy/firewall settings.
3. In CI, ensure the runner has outbound HTTPS access to `github.com`.

#### Inconsistent builds between local and CI

**Cause**: Different vcpkg commit checked out locally vs. CI.

**Fix**:
1. Ensure your local vcpkg checkout matches the builtin baseline:
   ```bash
   cd $VCPKG_ROOT
   git checkout d90a9b159c08169f39adcd1b0f1ac0ca12c4b96c
   ./bootstrap-vcpkg.sh -disableMetrics
   ```
2. Delete and rebuild the `installed/` directory.
