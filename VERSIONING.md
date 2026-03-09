# Versioning Policy

This document defines the semantic versioning policy, release process, and consumer
guidance for the kcenon unified_system ecosystem.

> **Maintained in**: `common_system` (Tier 0, ecosystem root)
> **Applies to**: common_system, thread_system, container_system, logger_system,
> monitoring_system, network_system, database_system

## Version Format

All ecosystem libraries use [Semantic Versioning 2.0.0](https://semver.org/):

```
MAJOR.MINOR.PATCH
```

| Component | Increment when |
|-----------|----------------|
| **MAJOR** | Breaking changes to public API or ABI |
| **MINOR** | New features added in a backward-compatible manner |
| **PATCH** | Backward-compatible bug fixes |

Tag format: `v{MAJOR}.{MINOR}.{PATCH}` (e.g., `v0.1.0`, `v1.0.0`)

Pre-release versions append a hyphen suffix: `v1.0.0-rc.1`, `v1.0.0-beta.2`

## Breaking Change Definition

A **MAJOR** version bump is required when any of the following occur:

- **Public API removal or rename** — Removing or renaming a public function, class,
  struct, enum, or type alias that was present in the previous release
- **Signature change** — Modifying the parameter list or return type of a public function
- **Behavior change** — Changing the documented behavior of a public API in a way that
  requires consumer code changes (e.g., error semantics, ownership, lifetime)
- **Header reorganization** — Moving or renaming public headers that break `#include`
  paths (unless a compatibility shim is provided)
- **CMake interface change** — Renaming or removing CMake targets, options, or variables
  consumed by downstream `find_package` or `FetchContent`
- **C++ standard bump** — Increasing the minimum required C++ standard beyond C++20

Non-breaking changes that do **not** require a MAJOR bump:

- Adding new functions, classes, or overloads to an existing API
- Adding new CMake options with backward-compatible defaults
- Internal refactoring with no public interface change
- Deprecating (but not removing) a symbol
- Test or documentation changes

## Pre-1.0 Stability Promise

While `MAJOR == 0`, breaking changes may occur in **MINOR** releases. This reflects
the ecosystem's active development phase. Consumers pinning to a specific `v0.x.y`
tag are protected from unannounced breakage.

Once `v1.0.0` is tagged, full SemVer guarantees apply.

## Release Process

### Step 1 — Update version numbers

All three locations must agree before tagging:

| File | Location |
|------|----------|
| `CMakeLists.txt` | `project(... VERSION X.Y.Z ...)` |
| `vcpkg.json` | `"version": "X.Y.Z"` |
| Git tag | `vX.Y.Z` |

### Step 2 — Commit and push version bump

```bash
git add CMakeLists.txt vcpkg.json
git commit -m "chore(release): bump version to X.Y.Z"
git push origin main
```

### Step 3 — Create and push the tag

```bash
git tag vX.Y.Z
git push origin vX.Y.Z
```

Pushing the tag automatically triggers the `release.yml` workflow, which:

1. Validates version consistency across all three locations
2. Builds and tests on Ubuntu, macOS, and Windows
3. Publishes a GitHub Release with auto-generated changelog from conventional commits

### Step 4 — Verify the release

Confirm the GitHub Release is published at:
`https://github.com/kcenon/{repo}/releases/tag/vX.Y.Z`

## Changelog Generation

Release notes are generated automatically from commit messages using conventional
commit format. Use these prefixes to produce meaningful changelogs:

| Prefix | Section in changelog |
|--------|----------------------|
| `feat:` | New Features |
| `fix:` | Bug Fixes |
| `perf:` | Performance Improvements |
| `refactor:` | Code Refactoring |
| `docs:` | Documentation |
| `chore:` | Maintenance |
| `BREAKING CHANGE:` | Breaking Changes (footer) |

## Consumer Guide

### CMake FetchContent

Pin to a specific release tag rather than a branch:

```cmake
# Good — reproducible
FetchContent_Declare(
  common_system
  GIT_REPOSITORY https://github.com/kcenon/common_system.git
  GIT_TAG        v0.1.0
)
```

Avoid floating refs such as branch names in production or CI configurations,
because they make builds non-reproducible.

### vcpkg Overlay Port

Update your overlay port's `portfile.cmake` to reference the tagged commit:

```cmake
vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO kcenon/common_system
    REF v0.1.0
    SHA512 <sha512-of-the-tag-archive>
    HEAD_REF main
)
```

Regenerate the SHA512 hash after each version bump:

```bash
vcpkg install --overlay-ports=./overlay-ports kcenon-common-system
# Or compute manually:
curl -sL https://github.com/kcenon/common_system/archive/refs/tags/v0.1.0.tar.gz | sha512sum
```

And update `vcpkg.json`:

```json
{
  "name": "kcenon-common-system",
  "version": "0.1.0"
}
```

## Ecosystem Compatibility Matrix

Internal dependencies must use pinned tagged versions. Branch references (`main`) are
not permitted in production builds or CI release workflows.

| Consumer | Depends on | Minimum Version |
|----------|-----------|-----------------|
| thread_system | common_system | v0.1.0 |
| container_system | common_system | v0.1.0 |
| logger_system | common_system | v0.1.0 |
| monitoring_system | common_system | v0.1.0 |
| database_system | common_system | v0.1.0 |
| network_system | common_system | v0.1.0 |

Update this table in `DEPENDENCY_MATRIX.md` after each release that downstream projects
adopt.

## Deprecation Policy

Before removing a public symbol:

1. Mark it `[[deprecated("Use Foo instead")]]` in a MINOR release
2. Announce deprecation in the GitHub Release notes
3. Remove it in the next MAJOR release (not before)

This gives consumers at least one MINOR version cycle to migrate.

---

*Version: 1.0.0 | Established: 2026-03-09 | Owned by: common_system (Tier 0)*
