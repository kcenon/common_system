# Document Review Report - common_system

**Generated**: 2026-04-14
**Mode**: Report-only (no source .md files modified)
**Files analyzed**: 125

Scope: all Markdown files under the repository, excluding `build/` and `.git/`.
Includes `docs/` tree (121 files) plus top-level README.md, README.kr.md, CHANGELOG.md,
CONTRIBUTING.md, CODE_OF_CONDUCT.md, SECURITY.md, VCPKG_DEPLOYMENT.md, VERSIONING.md,
DEPENDENCY_MATRIX.md, CLAUDE.md, and `.github/*.md`.

## Findings Summary

| Severity | Phase 1 (Anchors) | Phase 2 (Accuracy) | Phase 3 (SSOT) | Total |
|----------|-------------------|---------------------|-----------------|-------|
| Must-Fix | 38 | 4 | 2 | 44 |
| Should-Fix | 77 | 9 | 12 | 98 |
| Nice-to-Have | 4 | 3 | 3 | 10 |

Tallies:
- **Phase 1** broken anchors/links: 8 intra-file, 117 inter-file (classified below), 6 inter-file anchor mismatches.
- **Phase 2** outdated copyrights/version mismatches/terminology inconsistencies.
- **Phase 3** duplicate SSOT declarations, orphan docs, missing cross-references.

---

## Must-Fix Items

### Phase 1 - Broken anchor/link targets

Intra-file broken anchors (link target does not resolve to any heading in the same file):

1. `docs/ADAPTER_GUIDE.md:41` - TOC link `#adapter` does not match heading `### adapter<T>` (slug `adaptert`). (Phase 1)
2. `docs/advanced/DEPENDENCY_MATRIX.md:55` - TOC link `#current-score-75-` points to heading `### Current Score: 75%` whose slug strips the trailing hyphen. (Phase 1)
3. `docs/advanced/DEPENDENCY_MATRIX.kr.md:55` - TOC link `#현재-점수-75-` has trailing hyphen that real slugger strips. (Phase 1)
4. `docs/API_REFERENCE.md:22` - TOC link `#result-pattern-recommended` does not match any heading in the file. (Phase 1)
5. `docs/API_REFERENCE.kr.md:22` - TOC link `#result-패턴-권장` has no matching heading. (Phase 1)
6. `docs/contributing/templates/GUIDE_TEMPLATE.md:31-33` - Template TOC references `#step-1-first-step`, `#step-2-second-step`, `#step-3-third-step` but template headings include trailing placeholder `[First Step Title]` producing slugs `step-1-first-step-title` etc. (Phase 1)

Inter-file anchor mismatches (target file exists, anchor does not):

7. `README.md:489,491` and `README.kr.md:443,445` - Link `docs/contributing/CONTRIBUTING.md#development-workflow` resolves to a file whose closest heading is `## Development Setup`. (Phase 1)
8. `README.md:490` and `README.kr.md:444` - Link `docs/contributing/CONTRIBUTING.md#code-style` does not match; file uses `## Code Style Guidelines`. (Phase 1)

Broken inter-file path references (target file simply does not exist at the referenced path):

9. `docs/advanced/STRUCTURE.md:661-666` - Links to `README.md`, `ARCHITECTURE.md`, `INTEGRATION.md`, `docs/ERROR_HANDLING.md`, `docs/SMART_POINTER_GUIDELINES.md`, `docs/RAII_GUIDELINES.md` all resolved relative to `docs/advanced/` rather than the actual file locations. (Phase 1)
10. `docs/ARCHITECTURE.md:21,422-424`, `docs/ARCHITECTURE.kr.md:387-389` - References `./INTEGRATION_POLICY.md`, `./INTEGRATION.md`, `./NEED_TO_FIX.md` all missing at `docs/` (actual files live under `docs/guides/`; `NEED_TO_FIX.md` does not exist anywhere). (Phase 1)
11. `docs/advanced/MIGRATION.md:693,694,777-780` and `docs/advanced/MIGRATION.kr.md:658,659,742-745` - References `./INTEGRATION.md`, `./ARCHITECTURE.md`, `./INTEGRATION_POLICY.md`, `./NEED_TO_FIX.md` - all unresolved. (Phase 1)
12. `docs/guides/INTEGRATION.md:645`, `docs/guides/INTEGRATION.kr.md:645`, `docs/guides/INTEGRATION_POLICY.md:144-145`, `docs/guides/INTEGRATION_POLICY.kr.md:144-145` - `./ARCHITECTURE.md` and `./NEED_TO_FIX.md` missing at `docs/guides/` (ARCHITECTURE.md is in `docs/`, NEED_TO_FIX.md nonexistent). (Phase 1)
13. `docs/guides/QUICK_START.md:290-292` - `../ERROR_HANDLING.md`, `../RAII_GUIDELINES.md`, `../INTEGRATION.md` resolve to `docs/` but actual files live in `docs/guides/`. (Phase 1)
14. `docs/guides/BEST_PRACTICES.md:1307-1311` - `../ERROR_HANDLING.md`, `../RAII_GUIDELINES.md`, `../SMART_POINTER_GUIDELINES.md`, `../IEXECUTOR_MIGRATION_GUIDE.md`, `../ERROR_CODE_GUIDELINES.md` all mis-pathed (ERROR_HANDLING/RAII/SMART_POINTER are in `docs/guides/`, IEXECUTOR_MIGRATION in `docs/advanced/`, ERROR_CODE_GUIDELINES in `docs/guides/`). (Phase 1)
15. `docs/contributing/CONTRIBUTING.md:998-1000` - `../ERROR_HANDLING.md`, `../RAII_GUIDELINES.md`, `../SMART_POINTER_GUIDELINES.md` resolve to `docs/` but live under `docs/guides/`. (Phase 1)
16. `docs/guides/RAII_GUIDELINES.md:681` and `docs/guides/RAII_GUIDELINES.kr.md:681` - `./ERRORS.md` does not exist. (Phase 1)
17. `docs/performance/E2E_BENCHMARKS.md:1138` - `../OPTIMIZATION.md` does not exist. (Phase 1)
18. `docs/guides/CONCEPTS_GUIDE.md:356-358` and `docs/guides/CONCEPTS_GUIDE.kr.md:328-330` - `result_pattern.md`, `event_bus.md`, `dependency_injection.md` (and `*_ko.md` variants) do not exist. (Phase 1)
19. `docs/README.kr.md:28,33,38,117,123,135,148` - Links to nonexistent files: `ARCHITECTURE_ISSUES.md`, `CURRENT_STATE.md`, `BASELINE.md`, `STATIC_ANALYSIS_BASELINE.md`, `IMPROVEMENTS.md`, `STRUCTURE.md` (under `docs/` rather than `docs/advanced/STRUCTURE.md`). (Phase 1)
20. `docs/README.kr.md:45,51,65,71,82,88,93,100,105,144-155` - Numerous links to guide files use flat paths but actual files are under `docs/guides/` or `docs/advanced/`. (Phase 1)
21. `docs/contributing/templates/ARCHITECTURE_TEMPLATE.md:309-311`, `docs/contributing/templates/FEATURE_TEMPLATE.md:96,312,318-320` - Template references `API_REFERENCE.md`, `FEATURES.md`, `guides/INTEGRATION.md`, `advanced/MIGRATION.md`, `ARCHITECTURE.md`, `guides/QUICK_START.md` resolved relative to `docs/contributing/templates/`. (Phase 1)
22. `DEPENDENCY_MATRIX.md:8` (root) - `./SOUP-LIST.md` does not exist at repo root (actual file: `docs/SOUP-LIST.md`). (Phase 1)

### Phase 2 - Factual / version mismatches

23. `docs/SOUP.md:52,53,75,76` - GoogleTest pinned to 1.14.0 and Google Benchmark to 1.8.3, but `vcpkg.json` and `CLAUDE.md` specify 1.17.0 and 1.9.5 respectively. SOUP is authoritative for third-party software inventory; this drift is a compliance/audit risk. (Phase 2)
24. `docs/PROJECT_STRUCTURE.md:41`, `docs/PROJECT_STRUCTURE.kr.md:41`, `docs/guides/QUICK_START.md:20`, `docs/COMPATIBILITY.kr.md:99` - State CMake 3.16+, but `CMakeLists.txt` requires 3.20, and `docs/COMPATIBILITY.md:99` says 3.28+. Three different minimum CMake versions documented. (Phase 2)
25. `docs/advanced/MIGRATION.md:753` and `docs/advanced/MIGRATION.kr.md:718` - Cite CMake 3.20+ which matches `CMakeLists.txt` but not the 3.28+ cited in COMPATIBILITY.md. (Phase 2)
26. `docs/RELEASING.md:12` - Copyright `(c) 2025, kcenon`; current year is 2026. (Phase 2)

### Phase 3 - SSOT conflicts

27. Duplicate SSOT declaration for "Error Code Registry": both `docs/ERROR_CODES.md` (line 11) and `docs/ERROR_CODE_REGISTRY.md` (line 13) claim to be the single source of truth for the same topic. Exactly one must be designated authoritative and the other redirected or removed. (Phase 3)
28. Duplicate SSOT declaration for "Common System - Project Structure": both `docs/PROJECT_STRUCTURE.md` and `docs/advanced/STRUCTURE.md` declare SSOT ownership of the same topic. (Phase 3)

---

## Should-Fix Items

### Phase 1 - Missing Korean translations and consistent path errors

29. Missing `.kr.md` counterparts referenced by their English siblings (11 unique, 16 references total):
    - `ADAPTER_GUIDE.kr.md`, `CONFIG_GUIDE.kr.md` (referenced from CONFIG_GUIDE.md and CONFIG_UNIFIED.md), `DOCUMENTATION_GUIDELINES.kr.md`, `ERROR_CODES.kr.md`, `FEATURE_FLAGS_GUIDE.kr.md` (referenced from 3 files), `STRUCTURE.kr.md`, `TROUBLESHOOTING.kr.md`, `RUST_PARITY.kr.md`, `GUIDE_NAME.kr.md` (placeholder in template - see item 32).
30. Cross-directory link path corrections needed (77 references; consolidate by fixing or updating directory structure). Highest offenders: `docs/README.kr.md` (uses flat paths for files under subdirs), `docs/advanced/STRUCTURE.md`, templates under `docs/contributing/templates/`.
31. `docs/guides/TROUBLESHOOTING.md:15` references `TROUBLESHOOTING.kr.md` which is missing.

### Phase 2 - Terminology and version drift

32. GoogleTest/Benchmark version drift across docs (see Must-Fix #23). Once reconciled in SOUP, audit all doc cross-references.
33. Multiple CMake minimum-version statements (3.16+, 3.20+, 3.28+) create reader confusion. Pick a single statement or clearly delineate "core build" vs "C++20 modules optional build".
34. `docs/contributing/CONTRIBUTING.md:12,271,929` - Embedded code samples show `Copyright (c) 2025`. If these are literal template snippets for new files, update template to current year (2026).
35. `docs/ERROR_CODES.md:28` - `Last Updated: 2026-02-08` but `doc_date` frontmatter says `2026-04-04`. Two date fields disagree. (Phase 2)
36. Terminology: ecosystem name appears as both "unified_system" (older docs) and "kcenon ecosystem" / "KCENON ecosystem" (newer docs). Examples: `docs/ERROR_CODE_REGISTRY.md:14` says "unified_system"; `docs/COMPATIBILITY.md:23` says "KCENON ecosystem"; `CLAUDE.md` uses "kcenon ecosystem". Pick one canonical name. (Phase 2)
37. `docs/COMPATIBILITY.md:98` and `docs/COMPATIBILITY.kr.md:98` - Compiler version floor for MSVC given as "MSVC 19.30+ / VS 2022" (EN) and differently phrased (KR); verify alignment with `CLAUDE.md` ("MSVC 2022+"). (Phase 2)
38. `docs/SOUP.md:52` GoogleTest link text "Google Test" vs. `docs/SOUP-LIST.md` uses "gtest"; minor terminology normalization. (Phase 2)
39. `docs/CHANGELOG.md` and root `CHANGELOG.md` appear to duplicate the same release history with diverging entries; if one is authoritative, mark the other a pointer. (Phase 2)
40. Apple Clang version floor: `CLAUDE.md` says "Apple Clang 14+", `README.md:109` and `docs/COMPATIBILITY.md:98` also say "Apple Clang 14+", but `docs/CHANGELOG.md:96` lists MSVC 2022 17.4+ only (omits Apple Clang) for modules. Align compiler support matrix. (Phase 2)

### Phase 3 - SSOT hygiene and cross-references

41. `FEATURES` topic area is split across 5 files: `docs/FEATURES.md`, `docs/FEATURES_CORE.md`, `docs/FEATURES_DI_CONFIG.md`, `docs/FEATURES_INTEGRATION.md`, `docs/FEATURES_TEMPLATE.md`. Only `FEATURES.md` declares SSOT. Clarify which file is authoritative and how the `FEATURES_*` splits relate. (Phase 3)
42. `CONFIG` topic split across 5 files: `CONFIG_CLI_PARSER.md`, `CONFIG_GUIDE.md`, `CONFIG_LOADER.md`, `CONFIG_UNIFIED.md`, `CONFIG_WATCHER.md`. `CONFIG_UNIFIED.md` declares SSOT; verify it supersedes the others. (Phase 3)
43. `FEATURE_FLAGS` topic has three files: `FEATURE_FLAGS_GUIDE.md` (SSOT), `FEATURE_FLAGS_REFERENCE.md`, `FEATURE_FLAGS_USAGE.md`. The non-SSOT files reference `FEATURE_FLAGS_GUIDE.kr.md` which does not exist. Consolidate or clarify role. (Phase 3)
44. `PRODUCTION` topic split across 5 files: `PRODUCTION_GUIDE.md` (SSOT), `PRODUCTION_CONFIG.md`, `PRODUCTION_OBSERVABILITY.md`, `PRODUCTION_QUALITY.md`, `PRODUCTION_SECURITY.md`. Verify bidirectional cross-referencing between PRODUCTION_GUIDE.md and its satellites. (Phase 3)
45. `INTEGRATION` topic has 6 files: `INTEGRATION_DEPENDENCY_MAP.md`, `INTEGRATION_GUIDE.md` (SSOT), `INTEGRATION_LIFECYCLE.md`, `INTEGRATION_PATTERNS.md`, plus `guides/INTEGRATION.md`, `guides/INTEGRATION_POLICY.md`. Multiple entry points likely confusing. (Phase 3)
46. Orphan docs (no inbound Markdown links from any other doc in the repo) - some are legitimate entry points; others are likely abandoned:
    - Templates/Issues: `.github/ISSUE_TEMPLATE/bug_report.md`, `.github/ISSUE_TEMPLATE/feature_request.md`, `.github/pull_request_template.md` (OK - consumed by GitHub UI).
    - Project-level documents: `CHANGELOG.md` (OK - GitHub renders), `CODE_OF_CONDUCT.md` (OK), `SECURITY.md` (OK), `VCPKG_DEPLOYMENT.md` (should be linked from README).
    - Documentation orphans likely needing cross-links: `docs/API_QUICK_REFERENCE.md`, `docs/ECOSYSTEM_OVERVIEW.md`, `docs/ECOSYSTEM.md`, `docs/FEATURES_TEMPLATE.md`, `docs/GETTING_STARTED.md`. (Phase 3)
47. One-directional SSOT references: most SSOT declarations point downstream to dependent docs, but few dependent docs back-link to their SSOT. Example: `docs/FEATURE_FLAGS_REFERENCE.md` and `docs/FEATURE_FLAGS_USAGE.md` do not link back to `FEATURE_FLAGS_GUIDE.md` in a "See SSOT" block. Add symmetric xref blocks. (Phase 3)
48. `docs/CROSS_REFERENCE_GUIDE.md` appears intended as a cross-reference index but is referenced by very few docs; either strengthen it as a navigational hub or deprecate. (Phase 3)
49. `docs/TRACEABILITY.md` - exists but not verified to be up-to-date with current document set; could be driving cross-reference enforcement. (Phase 3)
50. SSOT coverage gaps: topics without a clearly-marked SSOT doc: "Observability" (spread across PRODUCTION_OBSERVABILITY, monitoring hints in multiple places), "Thread safety guarantees" (scattered across Result/shared_ptr/singleton docs), "Versioning / stability guarantees" (VERSIONING.md exists at root but not clearly marked SSOT within docs tree). (Phase 3)
51. `DEPENDENCY_MATRIX.md` at repo root and `docs/advanced/DEPENDENCY_MATRIX.md` cover overlapping topics; determine which is authoritative. (Phase 3)
52. `docs/SOUP.md` and `docs/SOUP-LIST.md` have different schemas (SOUP.md uses tabular with columns, SOUP-LIST.md uses ecosystem-wide view). Document relationship and deconflict version numbers (see item 23). (Phase 3)

---

## Nice-to-Have Items

### Phase 1

53. `docs/contributing/templates/GUIDE_TEMPLATE.md:245,257,258,262,263,316-318` - Template placeholders (`FAQ.md`, `RELATED_GUIDE_*.md`, `TOPIC_*.md`, `GUIDE_NAME.kr.md`). These are intentional placeholders but will produce broken-link warnings in link-check CI. Consider using angle-bracket pseudo-URLs or commented placeholders to document intent without producing broken links. (Phase 1)
54. Intra-anchor fix suggestions for items 1-4 can be done by either (a) updating the heading text or (b) updating the TOC link; choose per doc style. (Phase 1)

### Phase 2

55. `docs/RUST_PARITY.md` - Consider adding a current-as-of date. (Phase 2)
56. Some doc_version frontmatter values are stuck at `"1.0.0"` across 40+ docs; increment when doc content changes materially to aid change tracking. (Phase 2)
57. Many `--- doc_status: "Released"` frontmatter blocks - consider adding per-doc "Last verified" date. (Phase 2)

### Phase 3

58. Orphans that are legitimate entry points (CHANGELOG, CODE_OF_CONDUCT, SECURITY, etc.) do not need xref changes; document this policy in `docs/CROSS_REFERENCE_GUIDE.md`. (Phase 3)
59. Consider adding a short `docs/NAVIGATION.md` or keep `docs/README.md` as the authoritative index, but fix broken links (item 19-20) first. (Phase 3)
60. `docs/contributing/templates/*.md` - These templates contain example slugs, placeholder filenames, and example cross-references. Document them explicitly as "template - do not follow links" in a callout. (Phase 3)

---

## Score

| Dimension | Score (1-10) | Notes |
|-----------|--------------|-------|
| **Anchors / Links** | 5/10 | 125+ broken references total; most due to file moves into `guides/` and `advanced/` subdirectories without updating links. TOC-in-template placeholders also contribute. |
| **Accuracy / Consistency** | 6/10 | C++20 and Apple Clang floors consistent, but CMake minimum version, GoogleTest/Benchmark versions, and ecosystem name (kcenon vs unified_system) drift across docs. Copyright dates stale. |
| **SSOT / Structure** | 6/10 | 83 distinct SSOT topics declared and mostly followed, but two topic collisions (Error Code Registry, Project Structure) must be resolved. Topic-area splits (FEATURES_*, CONFIG_*, PRODUCTION_*, INTEGRATION_*) lack clear hub-and-spoke discipline. |
| **Overall** | **5.7/10** | Structurally intentional but executionally inconsistent. Several small fixes (link paths, version pins, removing duplicate SSOT) would raise overall quality substantially. |

---

## Notes

### System-specific observations

- **SSOT header convention is well-adopted** - 75+ docs include the `> **SSOT**: This document is the single source of truth for **<topic>**` marker. Enforce uniqueness of topic strings programmatically.
- **Frontmatter is consistent** - `doc_id`, `doc_title`, `doc_version`, `doc_date`, `doc_status`, `project`, `category` appear in nearly all `docs/` files. Automate drift checks against `CMakeLists.txt` and `vcpkg.json`.
- **Bilingual documentation** - EN/KR pairs are widely used (`*.md` + `*.kr.md`). 11 English-only docs declare a KR counterpart that does not exist (item 29). Either create stubs or remove the stale `[한국어](...)` links.
- **Template files leak into review** - `docs/contributing/templates/*.md` account for ~10% of broken links because they contain intentional placeholder paths. Consider excluding `**/templates/*.md` from link-check tooling or marking those links clearly.
- **Flat-then-hierarchical migration artifact** - Many broken links (items 9-15, 19-21) suggest the `docs/` tree was reorganized into `guides/`, `advanced/`, `architecture/`, `performance/`, `tutorials/`, `cookbook/` subdirectories without a follow-up link-update sweep. A single mechanical fix would resolve ~77 Should-Fix items.

### Recurring issue patterns

1. **Directory reorganization without link sweep** - 77/117 broken inter-file links point to files that exist somewhere in the repo but at a different relative path than the link assumes. A path-aware link-fix tool would resolve these mechanically.
2. **Missing Korean (`.kr.md`) translations** - 11 distinct English docs link to a `.kr.md` counterpart that has never been created. Either create stubs ("See English version until translation is available") or remove the `[한국어](...)` link.
3. **Version-number divergence between docs and authoritative sources** - `vcpkg.json` and `CMakeLists.txt` are the true source of truth for build dependencies; SOUP.md, SOUP-LIST.md, PROJECT_STRUCTURE.md, QUICK_START.md, and COMPATIBILITY.md all lag behind with stale version pins (gtest 1.14.0 vs 1.17.0, CMake 3.16+ vs 3.20 vs 3.28+, benchmark 1.8.3 vs 1.9.5).

### Recommendations

1. **Add a CI link-check** (e.g., `lychee`, `markdown-link-check`) with a repo-specific config that excludes `docs/contributing/templates/`. This would have caught all 117 Phase 1 inter-file issues before they landed.
2. **Generate version-sensitive docs from a single source** - extract compiler, CMake, GoogleTest, Benchmark version floors into a YAML file and render them into COMPATIBILITY.md/PROJECT_STRUCTURE.md/QUICK_START.md via a small template step.
3. **Enforce SSOT uniqueness** - add a pre-commit or CI hook that scans for `SSOT: This document is the single source of truth for **<topic>**` lines and fails if any `<topic>` appears in more than one English doc.
4. **Backfill the Korean-translation stubs** - for each English doc with an unmet KR link, either create a one-line stub pointing back to the English version or strip the KR link. This would close 16 broken links in a single PR.
5. **Adopt a hub-and-spoke model for the FEATURES/CONFIG/PRODUCTION/INTEGRATION topic areas** - one index doc (the SSOT) with consistent "Back to index" footers on the satellite docs. This turns 15+ files from a forest into a tree.

---

*End of report*
