---
doc_id: "COM-PROJ-010"
doc_title: "Releasing common_system"
doc_version: "1.0.0"
doc_date: "2026-04-04"
doc_status: "Released"
project: "common_system"
category: "PROJ"
---

<!-- BSD 3-Clause License
     Copyright (c) 2025, kcenon
     See the LICENSE file in the project root for full license information. -->

# Releasing common_system

> **SSOT**: This document is the single source of truth for **Releasing common_system**.

This document describes the versioning policy and release process for common_system
and the broader kcenon ecosystem.

## Semantic Versioning

This project follows [Semantic Versioning 2.0.0](https://semver.org/spec/v2.0.0.html):

```
MAJOR.MINOR.PATCH
```

| Component | Increment When | Example |
|-----------|---------------|---------|
| **MAJOR** | Breaking API/ABI changes (removed headers, changed interfaces, incompatible Result\<T\> changes) | `0.x.y` → `1.0.0` |
| **MINOR** | Backward-compatible additions (new headers, new interfaces, new Result methods) | `0.1.y` → `0.2.0` |
| **PATCH** | Backward-compatible bug fixes (compile fixes, doc corrections, test improvements) | `0.1.0` → `0.1.1` |

> **Note:** While `MAJOR` is `0`, the API is considered unstable. Minor version bumps
> may include breaking changes during the `0.x` development phase.

## Version Sources

All version numbers **must** be consistent across these files:

| File | Field | Format | Example |
|------|-------|--------|---------|
| `CMakeLists.txt` | `project(... VERSION x.y.z.0)` | 4-part (CMake standard, tweak=0) | `VERSION 0.1.0.0` |
| `vcpkg.json` | `"version": "x.y.z"` | 3-part semver | `"version": "0.1.0"` |
| Git tag | `vx.y.z` | `v`-prefixed semver | `v0.1.0` |

The `abi_version.h.in` template automatically picks up `PROJECT_VERSION_MAJOR`,
`PROJECT_VERSION_MINOR`, and `PROJECT_VERSION_PATCH` from CMake — no manual
update needed.

## Release Process

### 1. Prepare the Release

```bash
# Ensure main is up to date
git checkout main && git pull origin main

# Verify version consistency
grep "VERSION" CMakeLists.txt | head -1
grep '"version"' vcpkg.json
```

### 2. Update Version Numbers

When bumping the version, update both files:

```bash
# Example: bumping from 0.1.0 to 0.2.0

# CMakeLists.txt: project(common_system VERSION 0.2.0.0 LANGUAGES CXX)
# vcpkg.json:     "version": "0.2.0"
```

### 3. Update CHANGELOG.md

Move items from `[Unreleased]` to a versioned section:

```markdown
## [0.2.0] - 2026-03-15

### Added
- ...

### Changed
- ...
```

### 4. Create the Release Commit

```bash
git add CMakeLists.txt vcpkg.json docs/CHANGELOG.md
git commit -m "chore(release): prepare v0.2.0"
git push origin main
```

### 5. Tag and Publish

```bash
# Create annotated tag
git tag -a v0.2.0 -m "Release v0.2.0"

# Push tag
git push origin v0.2.0
```

### 6. Create GitHub Release

```bash
gh release create v0.2.0 \
  --title "v0.2.0" \
  --generate-notes
```

## Downstream Dependency Pinning

Ecosystem projects should reference specific tags instead of `main`:

### CMake FetchContent

```cmake
FetchContent_Declare(
  common_system
  GIT_REPOSITORY https://github.com/kcenon/common_system.git
  GIT_TAG        v0.1.0   # Pin to specific version
)
```

### CMake find_package

```cmake
find_package(common_system 0.1.0 REQUIRED)
```

### vcpkg

Reference via vcpkg registry with version constraints.

## ABI Compatibility

The `abi_version.h` header provides compile-time and link-time ABI checks:

- **Compile-time**: `abi_checker<ExpectedVersion>` triggers `static_assert` on mismatch
- **Link-time**: Version-specific symbols cause linker errors on mismatch
- **Runtime**: `check_abi_version()` and `is_compatible()` for dynamic checking

When making a release, consider whether the ABI has changed:

| Change Type | ABI Impact | Version Bump |
|-------------|-----------|-------------|
| New header/class added | Compatible | MINOR |
| New method on existing class | Compatible (header-only) | MINOR |
| Changed method signature | **Breaking** | MAJOR |
| Removed public header | **Breaking** | MAJOR |
| Bug fix in template logic | Compatible | PATCH |

## Ecosystem Projects

The following projects depend on common_system and should be updated
when a new version is released:

| Project | Repository |
|---------|-----------|
| thread_system | `kcenon/thread_system` |
| logger_system | `kcenon/logger_system` |
| network_system | `kcenon/network_system` |
| monitoring_system | `kcenon/monitoring_system` |
| database_system | `kcenon/database_system` |
| container_system | `kcenon/container_system` |
| messaging_system | `kcenon/messaging_system` |
