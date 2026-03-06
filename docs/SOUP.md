# SOUP List &mdash; common_system

> **Software of Unknown Provenance (SOUP) Register per IEC 62304:2006+AMD1:2015 &sect;8.1.2**
>
> This document is the authoritative reference for all external software dependencies.
> Every entry must include: title, manufacturer, unique version identifier, license, and known anomalies.

| Document | Version |
|----------|---------|
| IEC 62304 Reference | &sect;8.1.2 Software items from SOUP |
| Last Reviewed | 2026-03-06 |
| common_system Version | 1.0.0 |

---

## Production SOUP

common_system is a header-only/compiled C++20 foundation library with **no required third-party runtime dependencies**.

| ID | Name | Manufacturer | Version | License | Usage | Safety Class | Known Anomalies |
|----|------|-------------|---------|---------|-------|-------------|-----------------|
| &mdash; | *(none)* | | | | | | |

> **Note**: common_system has zero required production SOUP. It relies only on the C++20 standard library.

---

## Optional SOUP

| ID | Name | Manufacturer | Version | License | Usage | Safety Class | Known Anomalies |
|----|------|-------------|---------|---------|-------|-------------|-----------------|
| SOUP-001 | [yaml-cpp](https://github.com/jbeder/yaml-cpp) | Jesse Beder | Latest | MIT | YAML configuration file parsing (optional feature, detected at build time) | A | None |

---

## Development/Test SOUP (Not Deployed)

| ID | Name | Manufacturer | Version | License | Usage | Qualification |
|----|------|-------------|---------|---------|-------|--------------|
| SOUP-T01 | [Google Test](https://github.com/google/googletest) | Google | 1.14.0 | BSD-3-Clause | Unit testing framework (includes GMock) | Required |
| SOUP-T02 | [Google Benchmark](https://github.com/google/benchmark) | Google | 1.8.3 | Apache-2.0 | Performance benchmarking framework | Not required |
| SOUP-T03 | [Doxygen](https://www.doxygen.nl/) | Dimitri van Heesch | Latest | GPL-2.0 | API documentation generation (build tool only) | Not required |

---

## Safety Classification Key

| Class | Definition | Example |
|-------|-----------|---------|
| **A** | No contribution to hazardous situation | Logging, formatting, test frameworks |
| **B** | Non-serious injury possible | Data processing, network communication |
| **C** | Death or serious injury possible | Encryption, access control |

---

## Version Pinning (IEC 62304 Compliance)

All SOUP versions are pinned in `vcpkg.json` via the `overrides` field:

```json
{
  "overrides": [
    { "name": "gtest", "version": "1.14.0" },
    { "name": "benchmark", "version": "1.8.3" }
  ]
}
```

The vcpkg baseline is locked in `vcpkg-configuration.json` to ensure reproducible builds.

---

## Version Update Process

When updating any SOUP dependency:

1. Update the version in `vcpkg.json` (overrides section)
2. Update the corresponding row in this document
3. Verify no new known anomalies (check CVE databases)
4. Run full CI/CD pipeline to confirm compatibility
5. Document the change in the PR description

---

## License Compliance Summary

| License | Count | Copyleft | Obligation |
|---------|-------|----------|------------|
| MIT | 1 | No | Include copyright notice |
| BSD-3-Clause | 1 | No | Include copyright + no-endorsement clause |
| Apache-2.0 | 1 | No | Include license + NOTICE file |
| GPL-2.0 | 1 | Yes | Build tool only; not linked or distributed |

> **GPL contamination**: None. Doxygen is a build tool that does not link with or become part of the distributed software.
