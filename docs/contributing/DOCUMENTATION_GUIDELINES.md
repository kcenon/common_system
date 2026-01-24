# Documentation Structure Guidelines

> **Language:** **English** | [한국어](DOCUMENTATION_GUIDELINES.kr.md)

**Version:** 1.0.0
**Last Updated:** 2026-01-23
**Status:** Active

This document defines the official documentation organization pattern for all KCENON ecosystem systems, serving as a reference for maintaining consistency across projects.

---

## Table of Contents

- [Overview](#overview)
- [Required Documents](#required-documents)
- [Standard Folder Structure](#standard-folder-structure)
- [Naming Conventions](#naming-conventions)
- [Translation Requirements](#translation-requirements)
- [Document Templates](#document-templates)
- [Content Guidelines](#content-guidelines)
- [Compliance Checklist](#compliance-checklist)
- [Migration Guide](#migration-guide)

---

## Overview

### Purpose

This guideline ensures:

- **Consistency**: Developers can navigate any ecosystem system with familiar structure
- **Discoverability**: Documentation is easy to find in predictable locations
- **Maintainability**: Clear organization reduces duplication and outdated content
- **Accessibility**: Bilingual support (English/Korean) for broader audience

### Scope

These guidelines apply to all KCENON ecosystem systems:

| Layer | Systems |
|-------|---------|
| Layer 0 | common_system |
| Layer 1 | thread_system, container_system |
| Layer 2 | logger_system, monitoring_system, database_system |
| Layer 3 | network_system |
| Application | messaging_system |

---

## Required Documents

### Root Level (Required)

Every system **MUST** have these files at the repository root:

| File | Purpose | Template |
|------|---------|----------|
| `README.md` | Project overview, badges, quick links | - |
| `README.kr.md` | Korean translation of README | - |

### docs/ Level (Required)

Every system **MUST** have these files in the `docs/` directory:

| File | Purpose | Template |
|------|---------|----------|
| `README.md` | Documentation navigation index | - |
| `ARCHITECTURE.md` | System architecture overview | [ARCHITECTURE_TEMPLATE.md](templates/ARCHITECTURE_TEMPLATE.md) |
| `API_REFERENCE.md` | Complete API documentation | - |
| `FEATURES.md` | Feature list and descriptions | [FEATURE_TEMPLATE.md](templates/FEATURE_TEMPLATE.md) |
| `CHANGELOG.md` | Version history | [CHANGELOG_TEMPLATE.md](CHANGELOG_TEMPLATE.md) |
| `PROJECT_STRUCTURE.md` | Codebase organization | - |

### docs/ Level (Recommended)

These files are strongly recommended:

| File | Purpose | When Required |
|------|---------|---------------|
| `BENCHMARKS.md` | Performance benchmarks | Systems with performance-critical code |
| `COMPATIBILITY.md` | Version compatibility matrix | Systems with external dependencies |
| `PRODUCTION_QUALITY.md` | Production readiness checklist | Production systems |

---

## Standard Folder Structure

```
<system>/
├── README.md                  # Project overview (English)
├── README.kr.md               # Project overview (Korean)
│
└── docs/
    ├── README.md              # Documentation navigation index
    │
    ├── # Core documents (Required)
    ├── ARCHITECTURE.md        # System architecture overview
    ├── ARCHITECTURE.kr.md     # Korean translation
    ├── API_REFERENCE.md       # API reference
    ├── API_REFERENCE.kr.md    # Korean translation
    ├── FEATURES.md            # Feature list and descriptions
    ├── FEATURES.kr.md         # Korean translation
    ├── CHANGELOG.md           # Change history
    ├── CHANGELOG.kr.md        # Korean translation
    ├── PROJECT_STRUCTURE.md   # Codebase structure
    ├── PROJECT_STRUCTURE.kr.md # Korean translation
    │
    ├── # Optional core documents
    ├── BENCHMARKS.md          # Performance benchmarks
    ├── COMPATIBILITY.md       # Version compatibility
    ├── PRODUCTION_QUALITY.md  # Production readiness
    │
    ├── advanced/              # Internal implementation details
    │   ├── MIGRATION.md       # Migration guides
    │   ├── STRUCTURE.md       # Internal structure details
    │   └── DEPENDENCY_MATRIX.md # Dependency relationships
    │
    ├── architecture/          # Detailed architecture documents
    │   └── *.md               # Specific architecture topics
    │
    ├── guides/                # User-facing guides
    │   ├── QUICK_START.md     # Getting started (recommended)
    │   ├── INTEGRATION.md     # Integration with other systems
    │   ├── BEST_PRACTICES.md  # Usage recommendations
    │   ├── FAQ.md             # Frequently asked questions
    │   └── TROUBLESHOOTING.md # Problem resolution
    │
    ├── performance/           # Performance documentation
    │   ├── BASELINE.md        # Performance baselines (required)
    │   └── TUNING.md          # Performance tuning guide
    │
    ├── contributing/          # Contributor guides
    │   ├── CONTRIBUTING.md    # How to contribute
    │   ├── CI_CD_GUIDE.md     # CI/CD setup guide
    │   ├── CHANGELOG_TEMPLATE.md # Changelog format template
    │   ├── DOCUMENTATION_GUIDELINES.md # This document
    │   └── templates/         # Document templates
    │       ├── ARCHITECTURE_TEMPLATE.md
    │       ├── FEATURE_TEMPLATE.md
    │       └── GUIDE_TEMPLATE.md
    │
    └── integration/           # System integration docs
        └── README.md          # Integration overview
```

### Directory Purposes

| Directory | Purpose | Contents |
|-----------|---------|----------|
| `docs/` | Main documentation root | Core documents, index |
| `docs/advanced/` | Internal details | Migration, internals, dependencies |
| `docs/architecture/` | Architecture deep-dives | Specific architectural topics |
| `docs/guides/` | User guides | How-to, tutorials, FAQs |
| `docs/performance/` | Performance docs | Baselines, tuning guides |
| `docs/contributing/` | Contributor docs | Guidelines, templates, CI/CD |
| `docs/integration/` | Integration docs | Cross-system integration |

---

## Naming Conventions

### File Naming

| Pattern | Usage | Example |
|---------|-------|---------|
| `UPPER_CASE.md` | Core documents | `ARCHITECTURE.md`, `FEATURES.md` |
| `UPPER_CASE.kr.md` | Korean translations | `ARCHITECTURE.kr.md` |
| `lower_case.md` | Supplementary docs | `mainpage.dox` |

### Translation Suffix

| Suffix | Language | Example |
|--------|----------|---------|
| `.md` (no suffix) | English (default) | `README.md` |
| `.kr.md` | Korean | `README.kr.md` |

**Important**: Use `.kr.md` suffix, NOT `_KO.md` or `_ko.md`.

### Directory Naming

- Use `lowercase` with underscores if needed
- Examples: `guides/`, `advanced/`, `contributing/`
- Exception: `API/` may be uppercase for visibility

---

## Translation Requirements

### Required Translations

All **required documents** should have Korean translations:

| English | Korean |
|---------|--------|
| `README.md` | `README.kr.md` |
| `docs/ARCHITECTURE.md` | `docs/ARCHITECTURE.kr.md` |
| `docs/API_REFERENCE.md` | `docs/API_REFERENCE.kr.md` |
| `docs/FEATURES.md` | `docs/FEATURES.kr.md` |
| `docs/CHANGELOG.md` | `docs/CHANGELOG.kr.md` |
| `docs/PROJECT_STRUCTURE.md` | `docs/PROJECT_STRUCTURE.kr.md` |

### Translation Header

Every document should include a language selector:

**English version:**
```markdown
> **Language:** **English** | [한국어](FILENAME.kr.md)
```

**Korean version:**
```markdown
> **언어:** [English](FILENAME.md) | **한국어**
```

### Translation Guidelines

1. **Maintain structure**: Keep same sections and headings
2. **Keep code blocks**: Code examples remain in English
3. **Translate descriptions**: Translate explanatory text
4. **Sync versions**: Keep translations up-to-date with English
5. **Use consistent terminology**: Maintain glossary of translated terms

---

## Document Templates

Templates are provided in `docs/contributing/templates/`:

| Template | Purpose | Use When |
|----------|---------|----------|
| [ARCHITECTURE_TEMPLATE.md](templates/ARCHITECTURE_TEMPLATE.md) | System architecture | Creating new architecture docs |
| [FEATURE_TEMPLATE.md](templates/FEATURE_TEMPLATE.md) | Feature documentation | Documenting features |
| [GUIDE_TEMPLATE.md](templates/GUIDE_TEMPLATE.md) | User guides | Writing how-to guides |

### Template Usage

1. Copy the appropriate template
2. Replace placeholder content
3. Remove unused sections
4. Add system-specific content
5. Create Korean translation

---

## Content Guidelines

### Document Header

Every document should include:

```markdown
# Document Title

> **Language:** **English** | [한국어](FILENAME.kr.md)

**Version:** X.Y.Z
**Last Updated:** YYYY-MM-DD
**Status:** Draft | Active | Deprecated

Brief description of the document's purpose.

---
```

### Table of Contents

Include a table of contents for documents longer than 3 sections:

```markdown
## Table of Contents

- [Section 1](#section-1)
- [Section 2](#section-2)
- [Section 3](#section-3)
```

### Code Examples

- All code examples must compile
- Include language identifier in code blocks
- Provide context for code snippets

```cpp
// Example: Using Result<T> for error handling
auto result = process_data(input);
if (result.is_ok()) {
    auto value = result.value();
    // Process value
}
```

### Cross-References

Use relative links for internal references:

```markdown
See [Architecture](ARCHITECTURE.md) for system design.
For integration, refer to [Integration Guide](guides/INTEGRATION.md).
```

### External Links

For external resources, use descriptive link text:

```markdown
- [GitHub Repository](https://github.com/kcenon/common_system)
- [Issue Tracker](https://github.com/kcenon/common_system/issues)
```

---

## Compliance Checklist

### Pre-Submission Checklist

Before submitting documentation:

- [ ] **Structure**: Follows standard folder structure
- [ ] **Required docs**: All required documents present
- [ ] **Naming**: Follows naming conventions
- [ ] **Header**: Includes version, date, language selector
- [ ] **TOC**: Table of contents for long documents
- [ ] **Links**: All internal links work
- [ ] **Code**: All code examples compile
- [ ] **Korean**: Required translations present
- [ ] **Sync**: Translations match English content

### Document Quality Checklist

- [ ] Clear and concise writing
- [ ] Consistent formatting throughout
- [ ] No duplicate content across documents
- [ ] Proper grammar and spelling
- [ ] Technical accuracy verified

---

## Migration Guide

### Migrating from Non-Standard Structure

If your system has documentation that doesn't follow these guidelines:

#### Step 1: Audit Current Documentation

```bash
# List all markdown files
find docs -name "*.md" | sort

# Check for root-level docs that should be in docs/
ls -la *.md
```

#### Step 2: Create Missing Directories

```bash
mkdir -p docs/advanced
mkdir -p docs/guides
mkdir -p docs/performance
mkdir -p docs/contributing/templates
mkdir -p docs/integration
```

#### Step 3: Relocate Files

| From | To |
|------|-----|
| Root `CHANGELOG.md` | `docs/CHANGELOG.md` |
| Root `BASELINE.md` | `docs/performance/BASELINE.md` |
| `benchmarks/BASELINE.md` | `docs/performance/BASELINE.md` |
| Root `STRUCTURE.md` | `docs/PROJECT_STRUCTURE.md` |
| Root `MIGRATION.md` | `docs/advanced/MIGRATION.md` |

#### Step 4: Consolidate Duplicates

- Merge duplicate CHANGELOG files (keep `docs/CHANGELOG.md`)
- Merge FEATURES and CAPABILITIES documents
- Merge similar guides into single comprehensive guides

#### Step 5: Update Cross-References

After moving files, update all links:

```bash
# Find files with old links
grep -r "](../CHANGELOG.md)" docs/
grep -r "](BASELINE.md)" docs/
```

#### Step 6: Create Missing Translations

For each required document without Korean translation:

```bash
# Create translation file
cp docs/ARCHITECTURE.md docs/ARCHITECTURE.kr.md
# Edit and translate content
```

---

## Related Documents

- [CONTRIBUTING.md](CONTRIBUTING.md) - General contribution guidelines
- [CHANGELOG_TEMPLATE.md](CHANGELOG_TEMPLATE.md) - Changelog format
- [CI_CD_GUIDE.md](CI_CD_GUIDE.md) - CI/CD pipeline documentation

---

## Support

### Documentation Issues

- **Missing info**: [Open documentation issue](https://github.com/kcenon/common_system/issues/new?labels=documentation)
- **Incorrect examples**: Report with details
- **Unclear instructions**: Suggest improvements

### Getting Help

1. Check existing documentation first
2. Search [GitHub Issues](https://github.com/kcenon/common_system/issues)
3. Ask on [GitHub Discussions](https://github.com/kcenon/common_system/discussions)

---

**Documentation Structure Guidelines** - KCENON Ecosystem Standard

**Version:** 1.0.0
**Last Updated:** 2026-01-23
