---
doc_id: "COM-GUID-008"
doc_title: "README_TEMPLATE.md"
doc_version: "1.0.0"
doc_date: "2026-04-04"
doc_status: "Released"
project: "common_system"
category: "GUID"
---

# README_TEMPLATE.md

> Canonical README structure for all kcenon ecosystem projects.
> Every project README (both `README.md` and `README.kr.md`) MUST follow
> this 13-section layout in the order shown below.

## How to Use This Template

1. Copy the section headings **in order** into your project README.
2. Replace `{{placeholders}}` with project-specific content.
3. Do **not** remove existing content -- restructure it to fit the standard sections.
4. Each section has a minimum content requirement noted in parentheses.
5. Sections marked **(required)** must have substantive content; sections marked
   **(link-ok)** may contain a brief summary plus a link to a detailed document.

---

## Section Layout

```
Badge Row
# {{Project Name}}
> Language switcher

## 1. Overview (required)
## 2. Key Features (required)
## 3. Requirements (required)
## 4. Quick Start (required)
## 5. Installation (required)
## 6. Architecture (required)
## 7. Core Concepts (required)
## 8. API Overview (link-ok)
## 9. Examples (required)
## 10. Performance (link-ok)
## 11. Ecosystem Integration (required)
## 12. Contributing (link-ok)
## 13. License (required)
```

---

## 1. Overview

**Purpose**: Introduce the project, its core value proposition, and current status.

**Required content**:
- One-paragraph project description
- 3-5 key value propositions (bullet list)
- Latest updates / current status summary

**Example**:
```markdown
## Overview

A modern C++20 {{domain}} library providing {{core capability}}.
Built with a modular, interface-based architecture for seamless ecosystem integration.

**Key Value Propositions**:
- **{{Benefit 1}}**: {{Description}}
- **{{Benefit 2}}**: {{Description}}
- **{{Benefit 3}}**: {{Description}}
```

---

## 2. Key Features

**Purpose**: Provide a scannable feature matrix.

**Required content**:
- Feature matrix table OR categorized bullet list
- Each feature should have a one-line description and status indicator

**Example**:
```markdown
## Key Features

| Category | Feature | Description | Status |
|----------|---------|-------------|--------|
| **Core** | {{Feature}} | {{Description}} | Stable |
| **Performance** | {{Feature}} | {{Description}} | Stable |
| **Integration** | {{Feature}} | {{Description}} | Beta |
```

---

## 3. Requirements

**Purpose**: List system requirements, compiler versions, and dependencies.

**Required content**:
- Compiler version table (GCC, Clang, MSVC, Apple Clang)
- CMake version
- Required and optional dependencies with version ranges
- Dependency flow diagram (text-based)

---

## 4. Quick Start

**Purpose**: Get a user from zero to working code in under 20 lines.

**Required content**:
- Minimal working example (< 20 lines of C++)
- Must compile and run with default build options
- Link to full getting-started guide

---

## 5. Installation

**Purpose**: Cover all installation methods.

**Required content** (at least two methods):
- vcpkg installation
- CMake FetchContent
- Manual build (clone + cmake)

**Optional**:
- C++20 module build instructions
- Platform-specific notes (Windows, macOS, Linux)

---

## 6. Architecture

**Purpose**: Show module structure and key design patterns.

**Required content**:
- Module/component diagram (ASCII art or Mermaid)
- Key abstractions list (2-3 sentences each)
- Link to detailed architecture document

---

## 7. Core Concepts

**Purpose**: Explain essential concepts for understanding and using the library.

**Required content**:
- 3-5 core concepts with brief explanations
- Code snippets illustrating each concept (2-5 lines)

---

## 8. API Overview

**Purpose**: Summarize key APIs without duplicating the full reference.

**Required content**:
- Key API summary table (class/function | purpose | header)
- Link to `docs/API_REFERENCE.md`

---

## 9. Examples

**Purpose**: Showcase representative usage patterns.

**Required content**:
- 3-5 example entries with descriptions
- Table format: Example | Description | Difficulty
- Build and run instructions
- Link to examples directory

---

## 10. Performance

**Purpose**: Provide benchmark summary and performance characteristics.

**Required content**:
- Key metrics table (operation | throughput/latency | notes)
- Platform information (CPU, OS, compiler)
- Link to `docs/BENCHMARKS.md`

---

## 11. Ecosystem Integration

**Purpose**: Show how this project fits into the kcenon ecosystem.

**Required content**:
- Dependency graph (which projects this depends on)
- Integration diagram showing upstream/downstream relationships
- 1-2 integration code examples with sibling projects
- Link to ecosystem documentation

---

## 12. Contributing

**Purpose**: Direct contributors to the right resources.

**Required content**:
- Link to `CONTRIBUTING.md` (or `docs/contributing/CONTRIBUTING.md`)
- Quick-start contribution steps (fork, branch, PR)

---

## 13. License

**Purpose**: State the license clearly.

**Required content**:
- License name and link to LICENSE file

---

## Footer (Optional)

A centered footer line may be included after the License section:

```markdown
---

<p align="center">
  Made with ... by ...
</p>
```

---

## Table of Contents Rule

A Table of Contents section is **not** part of the 13 standard sections.
It is placed immediately after the language switcher line and before
Section 1 (Overview). It lists all 13 sections as anchor links.

---

## Language Switcher

Every README begins with a language switcher immediately after the H1 title:

```markdown
> **Language:** **English** | [Korean](README.kr.md)
```

For Korean READMEs:
```markdown
> **Language:** [English](README.md) | **Korean**
```
