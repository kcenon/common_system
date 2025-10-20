# Static Analysis Baseline - common_system

> **Language:** **English** | [한국어](STATIC_ANALYSIS_BASELINE_KO.md)


**Date**: 2025-10-03  
**Version**: 1.0.0  
**Tool Versions**:
- clang-tidy: 18.x
- cppcheck: 2.x

## Overview

This document establishes the baseline for static analysis warnings in common_system.
The goal is to track improvements over time and prevent regression.

## Clang-Tidy Baseline

### Configuration
- Checks enabled: modernize-*, concurrency-*, performance-*, bugprone-*, cert-*, cppcoreguidelines-*
- Standard: C++20
- Analysis scope: Header-only library (include/kcenon/common/)

### Initial Baseline (Phase 0)

**Total Warnings**: TBD  
Run: `clang-tidy -p build/compile_commands.json include/kcenon/common/**/*.h`

Categories:
- `modernize-*`: 0 (header-only, C++20 compliant)
- `performance-*`: 0 (interface definitions)
- `concurrency-*`: 0 (no implementation)
- `readability-*`: TBD
- `bugprone-*`: 0

### Notable Suppressions
- `readability-identifier-length`: Suppressed (allows single-char template params)
- `readability-magic-numbers`: Suppressed (enum values, error codes)
- `cppcoreguidelines-non-private-member-variables-in-classes`: Suppressed (POD structs)

## Cppcheck Baseline

### Configuration
- Enable: all checks
- Standard: C++20
- Platform: native

### Initial Baseline (Phase 0)

**Total Issues**: TBD  
Run: `cppcheck --project=.cppcheck --enable=all`

Categories:
- Error: 0
- Warning: 0
- Style: TBD
- Performance: 0 (header-only)

### Notable Suppressions
- `passedByValue`: Suppressed for interface parameters
- `noExplicitConstructor`: Suppressed for result.h (intentional implicit conversions)

## Target Goals

**Phase 1 Goals** (By 2025-11-01):
- clang-tidy: 0 errors, < 10 warnings
- cppcheck: 0 errors, < 5 warnings

**Phase 2 Goals** (By 2025-12-01):
- All warnings resolved or documented
- Zero technical debt in static analysis

## How to Run Analysis

### Clang-Tidy
```bash
# Generate compile commands
cmake -B build -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

# Run clang-tidy on all headers
find include/kcenon/common -name "*.h" | xargs clang-tidy -p build
```

### Cppcheck
```bash
# Using project configuration
cppcheck --project=.cppcheck --enable=all
```

## Tracking Changes

Any increase in warnings should be documented here with justification:

| Date | Tool | Change | Reason | Resolved |
|------|------|--------|--------|----------|
| - | - | - | - | - |


---

*Last Updated: 2025-10-20*
