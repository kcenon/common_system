# kcenon System Layout Standard

| Field | Value |
|-------|-------|
| **Version** | 1.0 |
| **Status** | Active |
| **Scope** | All 8 systems in the kcenon ecosystem |
| **Owner** | common_system (this repository) |
| **Master EPIC** | [kcenon/common_system#657](https://github.com/kcenon/common_system/issues/657) |
| **Sub-EPIC** | [kcenon/common_system#656](https://github.com/kcenon/common_system/issues/656) |

## Purpose

This document defines the canonical directory structure, build-system organization, and
test-infrastructure conventions for every system in the kcenon ecosystem:

`common_system`, `thread_system`, `logger_system`, `container_system`,
`monitoring_system`, `database_system`, `network_system`, `pacs_system`.

A single source-of-truth prevents each system from re-deciding conventions, makes
cross-system tooling and contributions predictable, and lets the ecosystem cross-build
CI workflow (issue #660) detect when a structural change in one repository breaks a sibling.

The layout pattern below is extracted from `pacs_system` (the reference implementation of
the modular CMake approach) and from the `kcenon::` header conventions already in use in
`common_system`. It is the canonical pattern; individual systems migrate toward it via
their respective sub-EPICs in the master EPIC.

> Conformance verification, including the per-system migration order, is tracked in
> kcenon/common_system#657. Phase 7 (kcenon/common_system#661) is the final audit.

## Layout

Every system's repository root MUST follow this skeleton:

```
<system_name>/
├── include/
│   └── kcenon/
│       └── <name>/         # public headers — see Headers
├── src/                    # implementation — see Sources
├── tests/                  # GTest unit tests — see Tests
├── integration_tests/      # cross-system tests — see Tests
├── examples/               # runnable usage examples
├── benchmarks/             # microbenchmarks (optional)
├── cmake/                  # CMake modules — see CMake
│   ├── options.cmake
│   ├── compiler.cmake
│   ├── dependencies.cmake
│   ├── targets.cmake
│   ├── install.cmake
│   ├── testing.cmake
│   ├── examples.cmake
│   └── warnings.cmake
├── docs/
├── CMakeLists.txt          # thin orchestrator that include()s cmake/*.cmake
├── CMakePresets.json
├── README.md
├── CHANGELOG.md
├── LICENSE
└── VERSION                 # SemVer string, single line
```

A system MAY add directories outside this list (e.g., `scripts/`, `tools/`) but MUST NOT
omit any of the directories above unless the system has a documented exception (see
Exceptions). A header-only system MAY have an empty or near-empty `src/` directory.

## Headers

### Path

Every public header MUST be reachable as:

```cpp
#include <kcenon/<name>/<path>.h>
```

where `<name>` matches the system name without the `_system` suffix
(e.g., `kcenon/thread/`, `kcenon/logger/`, `kcenon/common/`).

Internal-only headers (not part of the public API) live alongside their `.cpp` files
under `src/` and MUST NOT be installed.

### Namespace

Public symbols MUST live under the `kcenon::` namespace, with a per-system sub-namespace
matching the include path:

```cpp
namespace kcenon::common { /* ... */ }
namespace kcenon::thread { /* ... */ }
```

### Single header tree

Each system has exactly one public header tree at `include/kcenon/<name>/`. A system MUST
NOT scatter public headers across multiple top-level directories (e.g., no `interfaces/`,
`utilities/`, `core/` siblings to `include/`). If legacy directories exist, they are
absorbed into `include/kcenon/<name>/` during the system's migration phase.

## Sources

### Path

All implementation `.cpp` files live under `src/`. Sub-organization within `src/` is
permitted (`src/config/`, `src/modules/`, `src/<feature>/`) when it materially improves
navigation. Flat `src/` is acceptable for small systems.

### Header-source separation

Sources MUST NOT be placed under `include/`. The build system MUST install only files
from `include/kcenon/<name>/` as public headers.

### One translation unit per logical unit

Avoid mega-files: a single `.cpp` SHOULD correspond to one class or one closely-related
group of free functions. CMake collects sources via explicit lists in
`cmake/targets.cmake`, not globs.

## Tests

### Framework

The default test framework is **GTest**. New tests MUST use GTest. The Catch2 use in
`pacs_system` is a documented exception — see Exceptions.

### Directory split

| Directory | Purpose | CMake target type |
|-----------|---------|-------------------|
| `tests/` | Unit tests for code in `src/` | `GTest::gtest_main` |
| `integration_tests/` | Cross-system or large scenarios (failures, performance, stress, scenario-based) | Separate test executables, may use additional fixtures |

### One assertion focus per file

A test file is named after the unit it tests: `<unit>_test.cpp`. A test file SHOULD focus
on one component; sweeping multi-target test files (e.g., a single `unit_tests.cpp` that
covers everything) MUST be split during the migration phase.

### Discovery

Test registration is centralized in `cmake/testing.cmake`. The root `CMakeLists.txt`
MUST NOT contain `add_test()` calls.

## CMake

### Module decomposition

The root `CMakeLists.txt` MUST be a thin orchestrator that:

1. Sets minimum CMake version, project name, and language(s).
2. Sets `CMAKE_MODULE_PATH` to include `${CMAKE_CURRENT_SOURCE_DIR}/cmake`.
3. `include()`s the canonical modules in this order:

| Order | Module | Responsibility |
|-------|--------|----------------|
| 1 | `cmake/options.cmake` | Project options (`option()` calls), feature flags, version variables |
| 2 | `cmake/compiler.cmake` | Compiler/standard requirements (C++ standard, position-independent code, etc.) |
| 3 | `cmake/dependencies.cmake` | `find_package()` and `FetchContent` declarations |
| 4 | `cmake/warnings.cmake` | Warning flags applied to project targets |
| 5 | `cmake/targets.cmake` | Library/executable target definitions and source lists |
| 6 | `cmake/install.cmake` | Install rules and config-package generation |
| 7 | `cmake/testing.cmake` | Test target registration; gated on `BUILD_TESTING` |
| 8 | `cmake/examples.cmake` | Example target registration; gated on a project option |

A system MAY add `cmake/summary.cmake` (or similarly-named) for end-of-configure
diagnostics. This module is OPTIONAL and is included after `examples.cmake` when present.

### Module rules

- Each module MUST start with a header comment naming the module's responsibility and
  the input variables it expects from earlier modules.
- A module MUST NOT silently swallow compiler/linker flags from a sibling — flags belong
  in `compiler.cmake` or `warnings.cmake`, not scattered across `targets.cmake`.
- No module may rely on a global `file(GLOB)` for source discovery. Sources are
  enumerated explicitly in `targets.cmake`.
- Legacy fallback paths (parallel "old" and "new" CMakeLists branches) MUST be removed
  during the system's migration phase.

### Template

The reusable template implementing this decomposition is tracked in
[#659](https://github.com/kcenon/common_system/issues/659). Once that issue lands,
new systems adopt by copying the template directory and customizing the project-specific
inputs.

### Cross-build CI

Structural changes that touch `cmake/` or `include/` are validated by the ecosystem
cross-build CI workflow tracked in
[#660](https://github.com/kcenon/common_system/issues/660), which fetches every sibling
system at its default branch and verifies the dependency graph still builds.

## Forwarding

### When forwarding headers are required

Any change that **moves a public header path** or **renames a public symbol** MUST ship
a forwarding header at the old location. The forwarding header MUST:

1. Be marked `[[deprecated("Use <new path>")]]` so consumers see a warning at first
   compile.
2. Re-export the moved/renamed symbol using `using` declarations or include directives.
3. Carry a comment recording the version in which the forwarding header was added.

```cpp
// include/kcenon/common/old_path.h  (forwarding shim)
[[deprecated("Use <kcenon/common/new_path.h>")]]
#include <kcenon/common/new_path.h>
```

### Lifetime

A forwarding header MUST be retained for **one minor release** after introduction. It is
removed in the following minor release with a `BREAKING CHANGE` note in `CHANGELOG.md`.

### What does NOT need a forwarding header

- Internal-only headers under `src/` (not part of the public API).
- Renames inside an unstable / pre-1.0 sub-namespace explicitly marked unstable in its
  header comment.

## Exceptions

Documented exceptions to this standard. Each exception MUST be re-evaluated when its
owning system's next migration phase opens.

### pacs_system: Catch2 retained

`pacs_system` retains Catch2 for its existing test suite. Migrating it to GTest is
deferred per [kcenon/common_system#657](https://github.com/kcenon/common_system/issues/657)
Non-Goals. New tests added to `pacs_system` SHOULD still prefer GTest unless a strong
reason exists; review-time decision.

### Per-system deviations during migration

While a system is mid-migration, it MAY transiently violate parts of this standard. The
violation MUST be tracked by its sub-EPIC in
[kcenon/common_system#657](https://github.com/kcenon/common_system/issues/657) and
resolved before that sub-EPIC closes.

### Adding a new exception

A new exception is added by:

1. Opening a PR against this document under `docs/` in `common_system`.
2. Bumping the version (see Versioning).
3. Linking the originating issue or migration sub-EPIC.

## Versioning

This standard follows SemVer:

- **MAJOR** — breaking conventions (e.g., header path scheme change). Triggers a
  cross-ecosystem migration plan.
- **MINOR** — additive convention (new optional CMake module, new permitted directory).
  Adoption is non-blocking for existing systems until their next migration phase.
- **PATCH** — clarifications, typo fixes, non-normative wording.

The current version is **v1.0**.

## Discovery

Each system's `README.md` MUST link to this document near the top (under an
"Architecture", "Contributing", or similar heading). Tracking issue:
[#662](https://github.com/kcenon/common_system/issues/662).

## References

- Master EPIC: [kcenon/common_system#657](https://github.com/kcenon/common_system/issues/657)
- Sub-EPIC for this document: [kcenon/common_system#656](https://github.com/kcenon/common_system/issues/656)
- CMake template: [kcenon/common_system#659](https://github.com/kcenon/common_system/issues/659)
- Cross-build CI: [kcenon/common_system#660](https://github.com/kcenon/common_system/issues/660)
- README link integration: [kcenon/common_system#662](https://github.com/kcenon/common_system/issues/662)
- Phase 7 verification: [kcenon/common_system#661](https://github.com/kcenon/common_system/issues/661)
