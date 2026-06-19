---
doc_id: "COM-COMP-001"
doc_title: "ISO Standards Overview — kcenon ecosystem"
doc_version: "1.0.0"
doc_date: "2026-04-21"
doc_status: "Released"
project: "common_system"
category: "Compliance"
---

# ISO Standards Overview — kcenon ecosystem

> **Language:** **English**

**Scope**: a single navigable index of every ISO standard that the kcenon ecosystem touches, with pointers to per-system mapping documents and operator responsibilities. Audit and procurement reviewers should be able to answer "which system handles which standard?" without crawling each repository.

## Table of Contents

- [How to use this document](#how-to-use-this-document)
- [Standards × systems matrix](#standards--systems-matrix)
- [Per-standard summaries](#per-standard-summaries)
  - [ISO/IEC 14882 — Programming language C++](#isoiec-14882--programming-language-c)
  - [ISO/IEC 27001 — Information security management](#isoiec-27001--information-security-management)
  - [ISO/IEC 20000-1 — IT service management](#isoiec-20000-1--it-service-management)
  - [ISO 8601 — Date and time representation](#iso-8601--date-and-time-representation)
  - [ISO/IEC 9075 — SQL](#isoiec-9075--sql)
  - [ISO 12052 — DICOM](#iso-12052--dicom)
  - [ISO 27799 — Health informatics](#iso-27799--health-informatics)
- [Operator responsibilities](#operator-responsibilities)
- [Maintenance](#maintenance)
- [References](#references)

## How to use this document

- **"Status = Documented"** means a per-system mapping doc exists and is linked below. Reviewers should read the linked doc for control-by-control detail.
- **"Status = Implemented"** means the standard is satisfied by code/configuration but no dedicated compliance doc is published (yet). Evidence is in the linked feature documentation.
- **"Status = Enforced"** means the standard is enforced automatically by the build, CI, or runtime; there is no behavioral choice left to the integrator.
- **"Status = Planned"** means the mapping doc is in progress; a sub-issue of [#645](https://github.com/kcenon/common_system/issues/645) tracks it.

## Standards × systems matrix

| Standard | Systems | Status | Per-system doc |
|----------|---------|--------|----------------|
| ISO/IEC 14882 (C++20) | all | Enforced via `CMAKE_CXX_EXTENSIONS OFF` + `cxx_std_20` | project-wide (all `CMakeLists.txt`) |
| ISO/IEC 27001 (InfoSec) | `logger_system` | Documented | [logger_system/docs/compliance/iso-27001.md](https://github.com/kcenon/logger_system/blob/develop/docs/compliance/iso-27001.md) |
| ISO/IEC 27001 (InfoSec) | `monitoring_system` | Planned | — (sub-issue of [#645](https://github.com/kcenon/common_system/issues/645)) |
| ISO/IEC 27001 (InfoSec) | `database_system` | Planned | — (sub-issue of [#645](https://github.com/kcenon/common_system/issues/645)) |
| ISO/IEC 20000-1 (ITSM) | `monitoring_system` | Planned | — |
| ISO 8601 (date/time) | `logger_system` | Implemented | logger feature docs (timestamp formatters) |
| ISO/IEC 9075 (SQL) | `database_system` | Planned | — |
| ISO 12052 (DICOM) | `pacs_system` | Implemented | pacs DICOM module docs |
| ISO 27799 (health info) | `pacs_system` | Planned | — (tracked as [pacs_system#1126](https://github.com/kcenon/pacs_system/issues/1126)) |

Per-system mapping docs will be added as their sub-issues of [#645](https://github.com/kcenon/common_system/issues/645) land. Empty cells are intentional — they signal "known gap, tracked" rather than "not planned."

## Per-standard summaries

### ISO/IEC 14882 — Programming language C++

**Edition**: 2020 (C++20). The ecosystem targets the standard with no GNU/Microsoft extensions.

**Scope**: every component compiles with `-std=c++20` (or MSVC equivalent), `CMAKE_CXX_EXTENSIONS OFF`, and at least one warnings-as-errors CI job on GCC 11+, Clang 14+, and MSVC 2022+.

**Evidence**: top-level `CMakeLists.txt` in each repository sets `cxx_std_20`. CI matrices run the three major compilers; sanitizer jobs pin specific versions. See each system's `docs/PRODUCTION_QUALITY.md`.

**Operator responsibility**: none. The standard is enforced by the build.

### ISO/IEC 27001 — Information security management

**Edition**: 2022. The 2013 edition's Annex A identifiers are still in common use; per-system docs cross-reference them.

**Scope**: `logger_system`, `monitoring_system`, and `database_system` provide technical controls for an organization's ISMS. The libraries are not themselves certified; adopters integrate them and supply organizational controls (policy, training, risk management, internal audit).

**Covered control families** (see per-system docs for full mapping):

- A.5.17 (authentication information), A.5.33 (protection of records), A.5.34 (PII protection)
- A.8.3 (information access restriction), A.8.11 (data masking), A.8.15 (logging), A.8.16 (monitoring activities), A.8.24 (cryptography), A.8.28 (secure coding), A.8.34 (audit protection)

**Evidence pointer**: [logger_system/docs/compliance/iso-27001.md](https://github.com/kcenon/logger_system/blob/develop/docs/compliance/iso-27001.md) (other systems: planned).

### ISO/IEC 20000-1 — IT service management

**Edition**: 2018. Service-management requirements relevant to operated services (incident, change, problem, availability, capacity).

**Scope**: `monitoring_system` supplies the technical primitives for a service-management toolchain — SLI/SLO metrics, alert pipelines, and OpenTelemetry export.

**Status**: Planned. Sub-issue of [#645](https://github.com/kcenon/common_system/issues/645) will produce `monitoring_system/docs/compliance/iso-20000-1.md`.

**Operator responsibility**: define service catalog, SLAs, and response procedures. The library provides measurement and alerting, not the process framework.

### ISO 8601 — Date and time representation

**Edition**: 2019. Covers date, time, duration, interval, and recurring-time formats.

**Scope**: `logger_system` timestamp formatters emit ISO 8601 / RFC 3339 combined date-time strings (`YYYY-MM-DDTHH:MM:SS.sssZ`) in JSON and logfmt formatters. Plain-text formatter accepts a custom pattern.

**Evidence pointer**: `logger_system/docs/FEATURES.md` → Formatting Options.

**Operator responsibility**: select a formatter that emits ISO 8601 (JSON, logfmt) or configure the plain formatter pattern accordingly if downstream parsers require it.

### ISO/IEC 9075 — SQL

**Edition**: 2023 (most recent). The ecosystem targets a pragmatic subset — SQL:2016 core with CTE, window functions, and JSON path support where the backend provides them.

**Scope**: `database_system` ORM and query builders generate SQL that conforms to the targeted subset; backend-specific extensions are surfaced explicitly rather than emitted silently.

**Status**: Planned. Sub-issue of [#645](https://github.com/kcenon/common_system/issues/645) will produce `database_system/docs/compliance/iso-9075.md` with a feature matrix (CTE, window functions, JSON, PARTITION BY, MERGE).

**Operator responsibility**: verify that the deployed database engine supports the subset used by the generated SQL. The library does not polyfill missing engine features.

### ISO 12052 — DICOM

**Edition**: DICOM 2023e (published as ISO 12052:2017 plus supplements). Medical imaging interchange standard.

**Scope**: `pacs_system` implements DICOM over DIMSE (C-STORE, C-FIND, C-MOVE, C-GET) and DICOMweb (QIDO-RS, WADO-RS, STOW-RS) via the DCMTK and custom IHE profiles.

**Evidence pointer**: `pacs_system/docs/` — DICOM module reference, conformance statement.

**Operator responsibility**: manage Application Entity (AE) titles, association negotiation, and TLS profiles for production deployments.

### ISO 27799 — Health informatics

**Edition**: 2016. Applies ISO/IEC 27002 controls to the healthcare domain; mandatory reference for HIPAA-adjacent and GDPR-Art.9 deployments.

**Scope**: `pacs_system` storage, access logging, and retention features provide primitives for ISO 27799 alignment.

**Status**: Planned. Tracked as [pacs_system#1126](https://github.com/kcenon/pacs_system/issues/1126).

**Operator responsibility**: supply the organizational controls (information-security policy, role-based access catalog, incident-response plan) that ISO 27799 requires of the operating entity.

## Operator responsibilities

Technical controls alone do not make a system compliant. The following responsibilities are retained by the organization operating the ecosystem:

| Responsibility | Relevant standards | Why it cannot be library-owned |
|----------------|-------------------|--------------------------------|
| Key rotation schedule | ISO 27001 A.8.24 | Depends on organizational risk appetite and regulator guidance. |
| Log retention windows | ISO 27001 A.5.33, ISO 27799 | Determined by jurisdiction, sector regulation, and record-type classification. |
| Access-control policy | ISO 27001 A.5.15–A.5.18, ISO 27799 | Role definitions and entitlements are organization-specific. |
| Incident response plan | ISO 27001 A.5.24–A.5.28 | Process, not code; must integrate with the organization's on-call rotation. |
| Backup and DR | ISO 27001 A.8.13, ISO 20000-1 | Infrastructure scope, outside any single library. |
| Training and awareness | ISO 27001 A.6.3 | Organizational. |

## Maintenance

This document is updated whenever:

- A sub-issue of [#645](https://github.com/kcenon/common_system/issues/645) lands — fill in the corresponding "Per-system doc" cell.
- A new ISO standard is added to the ecosystem — append a matrix row and a per-standard section.
- A standard is updated to a new edition — adjust the "Edition" field in the corresponding section.

Do not remove rows for planned standards; flipping a row from "Planned" to "Documented" in one PR is the signal that the corresponding sub-issue has landed.

## References

- [common_system#645](https://github.com/kcenon/common_system/issues/645) — Ecosystem-wide ISO compliance EPIC
- [common_system#651](https://github.com/kcenon/common_system/issues/651) — this overview (implementation ticket)
- [logger_system docs/compliance/iso-27001.md](https://github.com/kcenon/logger_system/blob/develop/docs/compliance/iso-27001.md) — ISO 27001 mapping, landed in [logger_system#622](https://github.com/kcenon/logger_system/issues/622)
- ISO catalog — <https://www.iso.org/standards.html>
