---
doc_id: "COM-ADR-001"
doc_title: "ADR-001: Standardize on vcpkg as the sole package manager"
doc_version: "1.0.0"
doc_date: "2026-04-04"
doc_status: "Released"
project: "common_system"
category: "ADR"
---

# ADR-001: Standardize on vcpkg as the sole package manager

> **SSOT**: This document is the single source of truth for **ADR-001: Standardize on vcpkg as the sole package manager**.

| Field | Value |
|-------|-------|
| Status | Accepted |
| Date | 2026-03-11 |
| Decision Makers | kcenon ecosystem maintainers |
| Tracking | [common_system#405](https://github.com/kcenon/common_system/issues/405) |

## Context

The kcenon ecosystem comprises 7 C++ libraries that all use **vcpkg** for dependency
management. Only `common_system` shipped a `conanfile.py` (plus `test_package/`), while
the remaining 6 projects had no Conan support at all.

Key observations:

- **Inconsistency**: 1 of 7 projects had Conan; 7 of 7 had vcpkg.
- **No CI coverage**: The Conan recipe was never tested in CI, so it could silently
  drift from the actual build configuration.
- **Maintenance cost**: Keeping two package manager definitions in sync across 7
  repositories doubles the packaging maintenance burden.
- **No external demand**: No issues or pull requests requesting Conan support have been
  filed by external users.

## Decision

**Standardize on vcpkg only.** Remove Conan support from `common_system` and do not
add it to the remaining ecosystem projects.

This corresponds to **Option A** from the issue description.

## Consequences

### Positive

- Single source of truth for dependency versions (`vcpkg.json` + `vcpkg-configuration.json`).
- Reduced maintenance: no need to keep `conanfile.py` in sync with CMake options.
- Simpler CI matrix (no Conan test jobs).
- Clearer contributor onboarding — one package manager to learn.

### Negative

- Users in Conan-only environments cannot consume kcenon libraries via Conan.
  - Mitigation: CMake FetchContent remains available as a universal fallback.
- If Conan support is requested in the future, new `conanfile.py` files must be written
  from scratch.

### Neutral

- vcpkg overlay ports continue to be the recommended distribution mechanism for
  internal consumers.

## Files Removed

- `common_system/conanfile.py`
- `common_system/test_package/` (Conan test package directory)
