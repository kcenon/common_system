# Common System Documentation

> **Language:** **English** | [한국어](README_KO.md)

Welcome to the common_system documentation. This directory contains comprehensive guides, references, and best practices for using and integrating the common system.

## 📚 Table of Contents

### 🏗️ Architecture & Design

- **[Architecture](ARCHITECTURE.md)** / **[한국어](ARCHITECTURE_KO.md)**
  - Complete system architecture overview
  - Layer architecture and module dependencies
  - Integration patterns and best practices

- **[Architecture Issues](ARCHITECTURE_ISSUES.md)** / **[한국어](ARCHITECTURE_ISSUES_KO.md)**
  - Known architectural issues and tracking
  - Resolution phases and priorities
  - Risk assessment and mitigation

- **[Structure](STRUCTURE.md)**
  - Project directory layout
  - Namespace organization
  - Component architecture

- **[Current State](CURRENT_STATE.md)** / **[한국어](CURRENT_STATE_KO.md)**
  - System baseline and status
  - Build configuration
  - Known issues and next steps

### ⚠️ Error Handling

- **[Error Handling Guidelines](ERROR_HANDLING.md)** / **[한국어](ERROR_HANDLING_KO.md)**
  - Result<T> pattern usage
  - Monadic operations (map, and_then, or_else)
  - Error propagation patterns
  - Best practices and testing

- **[Error Code Guidelines](ERROR_CODE_GUIDELINES.md)**
  - Error code ranges and allocation
  - Adding new error codes
  - Compile-time validation
  - Registry management

### 🔗 Integration & Migration

- **[Integration Guide](INTEGRATION.md)** / **[한국어](INTEGRATION_KO.md)**
  - System integration examples
  - Quick start guides
  - Complete integration patterns
  - Troubleshooting

- **[Integration Policy](INTEGRATION_POLICY.md)** / **[한국어](INTEGRATION_POLICY_KO.md)**
  - Official integration policy
  - Integration tiers (Required, Optional)
  - CMake configuration patterns
  - Dependency management

- **[Migration Guide](MIGRATION.md)** / **[한국어](MIGRATION_KO.md)**
  - Migrating to common_system integration
  - Result<T> pattern adoption
  - Standard interface migration
  - Version-specific guides

- **[Namespace Migration](NAMESPACE_MIGRATION.md)**
  - Namespace migration strategy
  - Backward compatibility
  - Recommended usage patterns

- **[IExecutor Migration Guide](IEXECUTOR_MIGRATION_GUIDE.md)**
  - Function-based to job-based API migration
  - Deprecation timeline
  - Code examples and patterns

### 🔍 Development Guidelines

- **[RAII Guidelines](RAII_GUIDELINES.md)** / **[한국어](RAII_GUIDELINES_KO.md)**
  - Resource Acquisition Is Initialization patterns
  - Lifetime management
  - Best practices

- **[Smart Pointer Guidelines](SMART_POINTER_GUIDELINES.md)** / **[한국어](SMART_POINTER_GUIDELINES_KO.md)**
  - std::unique_ptr and std::shared_ptr usage
  - Ownership semantics
  - Performance considerations

### 📊 Analysis & Metrics

- **[Performance Baseline](BASELINE.md)**
  - Result<T> pattern performance
  - Interface abstraction overhead
  - Compile-time metrics
  - Memory footprint

- **[Static Analysis Baseline](STATIC_ANALYSIS_BASELINE.md)** / **[한국어](STATIC_ANALYSIS_BASELINE_KO.md)**
  - clang-tidy baseline
  - cppcheck baseline
  - Target goals and tracking

- **[Dependency Matrix](DEPENDENCY_MATRIX.md)** / **[한국어](DEPENDENCY_MATRIX_KO.md)**
  - System dependency relationships
  - Module integration status
  - Dependency graph

### 📈 Improvements

- **[Improvements](IMPROVEMENTS.md)** / **[한국어](IMPROVEMENTS_KO.md)**
  - Completed improvements
  - Ongoing enhancements
  - Future roadmap

## 📖 Quick Links

### Getting Started
1. Start with [Architecture](ARCHITECTURE.md) for system overview
2. Read [Integration Guide](INTEGRATION.md) for practical examples
3. Follow [Error Handling Guidelines](ERROR_HANDLING.md) for best practices

### For Developers
- [Structure](STRUCTURE.md) - Understand project organization
- [Error Code Guidelines](ERROR_CODE_GUIDELINES.md) - Add new error codes
- [RAII Guidelines](RAII_GUIDELINES.md) - Resource management patterns

### For Integration
- [Integration Policy](INTEGRATION_POLICY.md) - Official integration requirements
- [Migration Guide](MIGRATION.md) - Migrate existing code
- [IExecutor Migration Guide](IEXECUTOR_MIGRATION_GUIDE.md) - Update executor usage

## 🔄 Documentation Updates

All documentation is actively maintained and updated with code changes. If you find any discrepancies:

1. Check the corresponding source code in `include/kcenon/common/`
2. Refer to the latest test cases in `tests/` and `integration_tests/`
3. Report issues via GitHub Issues

## 📝 Contributing

When adding or updating documentation:

1. Keep both English and Korean versions synchronized
2. Include practical code examples
3. Update this index when adding new documents
4. Follow the established format and structure

---

**Version:** 1.0.0
**Last Updated:** 2025-11-09
**Maintainer:** kcenon
