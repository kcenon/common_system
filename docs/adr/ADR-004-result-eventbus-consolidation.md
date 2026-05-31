---
doc_id: "COM-ADR-004"
doc_title: "ADR-004: Result / event_bus Dual-Representation Consolidation Roadmap"
doc_version: "1.0.0"
doc_date: "2026-05-31"
doc_status: "Proposed"
project: "common_system"
category: "ADR"
---

# ADR-004: Result / event_bus Dual-Representation Consolidation Roadmap

> **SSOT**: This document is the single source of truth for **ADR-004: Result / event_bus Dual-Representation Consolidation Roadmap**.

| Field | Value |
|-------|-------|
| Status | Proposed |
| Date | 2026-05-31 |
| Decision Makers | kcenon ecosystem maintainers |
| Tracking issue | [#690](https://github.com/kcenon/common_system/issues/690) |

## Context

`kcenon.common` ships its two foundational abstractions, `Result<T>` and
`event_bus`, through **two parallel representations** that must be kept in
sync by hand. This ADR records a plan to reduce that dual-maintenance burden;
it does not perform the consolidation. The change accompanying this ADR is
limited to **visibility** (see Phase 0).

### Representation 1 — classic headers (the SSOT)

Under `include/kcenon/common/`:

- `patterns/result.h` — an umbrella facade. It contains no logic; it only
  re-includes the three consolidated headers in a fixed order:
  - `patterns/result/core.h` — `Result<T>`, `Optional<T>`, `error_info`.
  - `patterns/result/utilities.h` — factory functions, exception conversion,
    monadic helpers, macros.
  - `patterns/result/compat.h` — legacy error-code aliases kept for backward
    compatibility (deprecation tracked in `docs/DEPRECATION.md`).
  The facade preserves the stable public include path
  `#include <kcenon/common/patterns/result.h>`.
- `patterns/event_bus.h` — header-only `simple_event_bus` (aliased as
  `event_bus`), the common events, and the ABI-version guard. This header is
  the SSOT for the `event_bus` type; other systems (e.g. monitoring_system)
  wrap or extend it.

These headers are the canonical definition site. See ADR-002 (header-only
library design) and ADR-003 (Result<T> error handling pattern).

### Representation 2 — C++20 named-module units

Under `src/modules/`, built only by the optional `common_system_modules`
target (defined in `cmake/common-modules.cmake`):

- `result.cppm` — partition `kcenon.common:result`; `export import`s the
  `:result.core` and `:result.utilities` partitions.
- `result/core.cppm`, `result/utilities.cppm` — partitions that include the
  matching classic header in the global module fragment and then **re-export**
  the public names.
- `patterns.cppm` — partition exposing `event_bus` from `patterns/event_bus.h`.
- `common.cppm` — primary module `kcenon.common`; `export import`s every
  partition and also defines `module_version`.

The `.cppm` units are intentionally **front-ends**: they `#include` the
classic headers and re-export, rather than redefining symbols. This is the
right direction for avoiding ODR divergence, but it does not remove the
dual-maintenance burden, because each partition maintains an **explicit export
list** that mirrors the header's public surface.

### The dual-maintenance burden

- Every new public name added to a header must also be added to the
  corresponding `export namespace { using ...; }` block (or `export`
  declaration) in the matching `.cppm` partition.
- There is **no automated check** that the two representations stay in sync.
  A name added to a header but omitted from a partition's export list compiles
  cleanly for header consumers and silently disappears for module consumers.
- The drift is toolchain-dependent (see the coverage gap below), so a missing
  export can ship undetected on the platforms where modules are never built.

### The macOS module-build coverage gap

There are two layers to the gap, and only the first is currently signalled:

1. **CMake configure layer (signalled):** `cmake/common-modules.cmake`
   early-returns on AppleClang, on non-Ninja/non-VS generators, and on
   CMake < 3.28. Each early return now emits a `message(WARNING)` and sets
   `COMMON_BUILD_MODULES OFF`. **However**, these warnings only fire when a
   build explicitly passes `-DCOMMON_BUILD_MODULES=ON`. The module file's own
   header comment overstates this as "silently disabled with a warning"; in
   practice the warning is conditional on the option being requested.

2. **CI layer (the real silent gap):** the `module-build` job in
   `.github/workflows/ci.yml` is the only job that sets
   `-DCOMMON_BUILD_MODULES=ON`. Its matrix includes **Ubuntu/Clang-16 and
   Windows/MSVC only — macOS is omitted entirely**, with the exclusion
   recorded only in a code comment ("AppleClang does not support C++20
   modules, so macOS is excluded"). The job additionally runs with
   `continue-on-error: true`. The net effect: module coverage on macOS is
   never exercised, and nothing in a green CI run signals that the macOS
   module surface went unverified. A regression in a module export list on
   the macOS toolchain path cannot be caught by CI.

## Decision

This ADR records the **plan**. The structural consolidation is deferred to the
phased roadmap below, tracked under #690. The only code change shipped with
this ADR is **visibility** (Phase 0): make the macOS / AppleClang module-build
gap explicit in CI rather than implicit in a matrix omission, and correct the
misleading comment in `cmake/common-modules.cmake`.

## Consolidation Roadmap

### Phase 0 — Visibility (this PR)

- Add an explicit, always-running CI step in the `module-build` job of
  `.github/workflows/ci.yml` that emits a GitHub Actions warning annotation
  stating that the C++20 named-modules surface is **not** exercised on
  macOS / AppleClang and pointing to this ADR and #690.
- Correct the header comment in `cmake/common-modules.cmake` so it no longer
  claims an unconditional "silently disabled with a warning"; clarify that the
  warning is conditional on `COMMON_BUILD_MODULES=ON` and reference this ADR.
- No source/code consolidation, no change to header or module consumers, no
  change to which toolchains build modules.

### Phase 1 — Lock the contract

- Add a **module/header parity check**: a script (e.g. under `scripts/`) or
  test that extracts the public names exported by each header and asserts they
  appear in the matching `.cppm` export list (and vice versa). Wire it into
  the module-capable CI legs so a missing export fails the build.
- State the SSOT direction explicitly in `docs/ARCHITECTURE.md`: headers are
  canonical; modules re-export only.

### Phase 2 — Reduce duplication mechanically

- Replace hand-maintained `export namespace { using ...; }` blocks with a
  generated export list, or restructure so each partition re-exports the
  header namespace wholesale where the toolchain permits, minimising the
  hand-edited surface that can drift.
- Keep the global-module-fragment `#include` of the classic header as the
  single definition site (no symbol is ever defined twice).

### Phase 3 — Deprecation sequence for the parallel path

- Once the parity check has guarded the surface for at least one release
  cycle, decide the long-term shape:
  - **Option A (recommended):** headers stay SSOT; modules become a
    mechanically derived, checked re-export layer.
  - **Option B:** invert to modules-as-SSOT only if/when the ecosystem's
    minimum toolchain reliably supports named modules on every CI leg
    (notably AppleClang) — not currently the case.
- Sequence any removal of `patterns/result/compat.h` aliases per
  `docs/DEPRECATION.md`, independently of the header/module question, to avoid
  coupling two unrelated deprecations.

### Phase 4 — Close the toolchain coverage gap

- Track AppleClang named-modules support. When it stabilises, add a
  macOS leg to the `module-build` matrix and drop the AppleClang early return
  so the parity check runs there too.
- Until then, the explicit Phase 0 CI warning is the standing mitigation.

## Risks

- **Silent export drift (existing):** the highest-impact risk today; addressed
  by the Phase 1 parity check.
- **Toolchain fragmentation:** AppleClang and non-Ninja/non-VS generators
  cannot build modules; any "modules-as-SSOT" move would reduce platform
  coverage. Mitigated by keeping headers canonical (Option A).
- **`continue-on-error` masking:** the `module-build` job does not block merge.
  Tightening this is a Phase 1 concern and is intentionally out of scope for
  the visibility-only change here.
- **Deprecation coupling:** removing `compat.h` aliases and reshaping the
  module layer are independent; conflating them risks a larger, riskier
  change. Mitigated by sequencing them separately (Phase 3).
- **Generated-code complexity:** a generation step (Phase 2) adds build
  tooling. Mitigated by keeping it optional, falling back to a checked
  hand-maintained list.

## Consequences

### Positive

- Developers gain an explicit CI signal that module coverage is skipped on
  macOS (Phase 0), removing a class of silent regressions.
- The parity check (Phase 1) will make export drift a build failure instead of
  a latent, platform-specific bug.
- Long term, the dual-maintenance burden shrinks to a mechanically checked
  (and eventually generated) re-export layer, with a clear, decoupled
  deprecation path.

### Negative

- Phase 0 adds CI noise (a warning annotation on every `module-build` run)
  until the coverage gap is closed in Phase 4.
- Phases 1-2 add tooling that must itself be maintained.

## References

- Issue [#690](https://github.com/kcenon/common_system/issues/690) — refactor:
  consolidate Result/event_bus dual representation and surface macOS
  module-build coverage gap.
- ADR-002 — header-only library design
  (`docs/adr/ADR-002-header-only-library-design.md`).
- ADR-003 — Result<T> error handling pattern
  (`docs/adr/ADR-003-result-error-handling-pattern.md`).
- `cmake/common-modules.cmake` — module toolchain gate (configure-time skip).
- `.github/workflows/ci.yml` — `module-build` job (macOS omitted from matrix).
- `docs/DEPRECATION.md` — `compat.h` alias deprecation sequence.
