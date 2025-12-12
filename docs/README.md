# Common System Documentation

> **Language:** **English** | [한국어](README_KO.md)

**Version:** 0.2.0
**Last Updated:** 2025-11-11
**Status:** Comprehensive

Welcome to the common_system documentation! This is the foundational Layer 0 system providing standard interfaces, type-safe error handling (Result<T>), and core utilities for all KCENON C++ systems.

---

## 🚀 Quick Navigation

| I want to... | Document |
|--------------|----------|
| ⚡ Get started in 5 minutes | [Quick Start](guides/QUICK_START.md) |
| 🏗️ Understand the architecture | [Architecture](01-ARCHITECTURE.md) |
| 🔧 Integrate into my project | [Integration Guide](guides/INTEGRATION.md) |
| ❓ Find answers to common questions | [FAQ](guides/FAQ.md) (23 Q&A) |
| 🐛 Troubleshoot an issue | [Troubleshooting](guides/TROUBLESHOOTING.md) |
| ✨ Learn best practices | [Best Practices](guides/BEST_PRACTICES.md) |
| 🤝 Contribute to the project | [Contributing](contributing/CONTRIBUTING.md) |

---

## Table of Contents

- [Documentation Structure](#documentation-structure)
- [Documentation by Role](#documentation-by-role)
- [By Feature](#by-feature)
- [Contributing to Documentation](#contributing-to-documentation)

---

## Documentation Structure

### 📘 Core Documentation

Essential documents for understanding the system:

| Document | Description | Korean | Lines |
|----------|-------------|--------|-------|
| [01-ARCHITECTURE.md](01-ARCHITECTURE.md) | Layer 0 foundation architecture, standard interfaces, system dependencies | [🇰🇷](01-ARCHITECTURE_KO.md) | 800+ |

### 📗 User Guides

Step-by-step guides for users:

| Document | Description | Korean | Lines |
|----------|-------------|--------|-------|
| [QUICK_START.md](guides/QUICK_START.md) | 5-minute getting started guide for header-only integration | - | 324 |
| [FAQ.md](guides/FAQ.md) | 23 frequently asked questions with examples | - | 1020 |
| [TROUBLESHOOTING.md](guides/TROUBLESHOOTING.md) | Common problems, template errors, IDE configuration | - | 1221 |
| [BEST_PRACTICES.md](guides/BEST_PRACTICES.md) | Recommended patterns for Result<T>, RAII, error handling | - | 1272 |
| [ERROR_HANDLING.md](guides/ERROR_HANDLING.md) | Result<T> pattern usage and monadic operations | [🇰🇷](guides/ERROR_HANDLING_KO.md) | 600+ |
| [RAII_GUIDELINES.md](guides/RAII_GUIDELINES.md) | Resource Acquisition Is Initialization patterns | [🇰🇷](guides/RAII_GUIDELINES_KO.md) | 400+ |
| [SMART_POINTER_GUIDELINES.md](guides/SMART_POINTER_GUIDELINES.md) | unique_ptr and shared_ptr usage patterns | [🇰🇷](guides/SMART_POINTER_GUIDELINES_KO.md) | 300+ |
| [ERROR_CODE_GUIDELINES.md](guides/ERROR_CODE_GUIDELINES.md) | Error code ranges, allocation, compile-time validation | - | 200+ |
| [INTEGRATION.md](guides/INTEGRATION.md) | System integration patterns and examples | [🇰🇷](guides/INTEGRATION_KO.md) | 500+ |
| [INTEGRATION_POLICY.md](guides/INTEGRATION_POLICY.md) | Official integration policy and CMake patterns | [🇰🇷](guides/INTEGRATION_POLICY_KO.md) | 300+ |

### 📙 Advanced Topics

For experienced users and contributors:

| Document | Description | Korean | Lines |
|----------|-------------|--------|-------|
| [MIGRATION.md](advanced/MIGRATION.md) | Migration guide to common_system integration | [🇰🇷](advanced/MIGRATION_KO.md) | 400+ |
| [IEXECUTOR_MIGRATION_GUIDE.md](advanced/IEXECUTOR_MIGRATION_GUIDE.md) | Function-based to job-based API migration | - | 200+ |
| [NAMESPACE_MIGRATION.md](advanced/NAMESPACE_MIGRATION.md) | Namespace migration strategy and patterns | - | 150+ |
| [DEPENDENCY_MATRIX.md](advanced/DEPENDENCY_MATRIX.md) | System dependency relationships and integration status | [🇰🇷](advanced/DEPENDENCY_MATRIX_KO.md) | 200+ |
| [STRUCTURE.md](advanced/STRUCTURE.md) | Project directory layout and organization | - | 150+ |

### 🤝 Contributing

For contributors and maintainers:

| Document | Description | Korean | Lines |
|----------|-------------|--------|-------|
| [CONTRIBUTING.md](contributing/CONTRIBUTING.md) | Contribution guidelines, header-only best practices, testing | - | 996 |
| [CI_CD_GUIDE.md](contributing/CI_CD_GUIDE.md) | CI/CD pipeline, static analysis, header validation | - | 864 |

---

## Documentation by Role

### 👤 For New Users

**Getting Started Path**:
1. **⚡ Quick Start** - [5-minute guide](guides/QUICK_START.md) to first program with Result<T>
2. **🏗️ Architecture** - [System overview](01-ARCHITECTURE.md) and Layer 0 foundation
3. **🔧 Integration** - [Integration guide](guides/INTEGRATION.md) with CMake examples
4. **💡 Examples** - [FAQ](guides/FAQ.md) with 23 practical examples

**When You Have Issues**:
- Check [FAQ](guides/FAQ.md) first (23 common questions)
- Use [Troubleshooting](guides/TROUBLESHOOTING.md) for template errors
- Search [GitHub Issues](https://github.com/kcenon/common_system/issues)

### 💻 For Experienced Developers

**Advanced Usage Path**:
1. **🏗️ Architecture** - Understand [Layer 0 foundation](01-ARCHITECTURE.md)
2. **⚠️ Error Handling** - Master [Result<T> pattern](guides/ERROR_HANDLING.md)
3. **✨ Best Practices** - Learn [optimization patterns](guides/BEST_PRACTICES.md)
4. **🔍 Advanced** - Study [dependency matrix](advanced/DEPENDENCY_MATRIX.md)

**Deep Dive Topics**:
- [Error Handling](guides/ERROR_HANDLING.md) - Monadic operations and patterns
- [RAII Guidelines](guides/RAII_GUIDELINES.md) - Resource management
- [Smart Pointers](guides/SMART_POINTER_GUIDELINES.md) - Ownership semantics
- [Error Codes](guides/ERROR_CODE_GUIDELINES.md) - Adding custom error codes

### 🔧 For System Integrators

**Integration Path**:
1. **🔧 Integration Guide** - [System integration](guides/INTEGRATION.md)
2. **📋 Integration Policy** - [Official requirements](guides/INTEGRATION_POLICY.md)
3. **✨ Best Practices** - [Integration patterns](guides/BEST_PRACTICES.md#system-integration)
4. **🐛 Troubleshooting** - [Common issues](guides/TROUBLESHOOTING.md)

**Migration Resources**:
- [Migration Guide](advanced/MIGRATION.md) - Adopt common_system integration
- [IExecutor Migration](advanced/IEXECUTOR_MIGRATION_GUIDE.md) - Update executor usage
- [Namespace Migration](advanced/NAMESPACE_MIGRATION.md) - Namespace changes
- [Dependency Matrix](advanced/DEPENDENCY_MATRIX.md) - System relationships

### 🤝 For Contributors

**Contribution Path**:
1. **🤝 Contributing** - [How to contribute](contributing/CONTRIBUTING.md)
2. **🚀 CI/CD** - [Pipeline documentation](contributing/CI_CD_GUIDE.md)
3. **🏗️ Architecture** - [System internals](01-ARCHITECTURE.md)
4. **📊 Structure** - [Project organization](advanced/STRUCTURE.md)

**Development Resources**:
- [Code Style](contributing/CONTRIBUTING.md#code-style-guidelines)
- [Testing Guide](contributing/CI_CD_GUIDE.md#running-checks-locally)
- [Header-Only Best Practices](contributing/CONTRIBUTING.md#header-only-library-guidelines)

---

## By Feature

### ⚠️ Result<T> Pattern

| Topic | Document | Section |
|-------|----------|---------|
| Usage | [Error Handling](guides/ERROR_HANDLING.md) | Result<T> Pattern |
| Best Practices | [Best Practices](guides/BEST_PRACTICES.md) | Error Handling |
| FAQ | [FAQ](guides/FAQ.md) | Result<T> Usage |
| Examples | [Quick Start](guides/QUICK_START.md) | First Program |

### 🔗 Standard Interfaces

| Topic | Document | Section |
|-------|----------|---------|
| ILogger | [Architecture](01-ARCHITECTURE.md) | Standard Interfaces |
| IMonitor | [Architecture](01-ARCHITECTURE.md) | Standard Interfaces |
| IExecutor | [Architecture](01-ARCHITECTURE.md) | Standard Interfaces |
| Migration | [IExecutor Migration](advanced/IEXECUTOR_MIGRATION_GUIDE.md) | Job-Based API |

### 🛡️ RAII Patterns

| Topic | Document | Section |
|-------|----------|---------|
| Guidelines | [RAII Guidelines](guides/RAII_GUIDELINES.md) | Resource Management |
| Smart Pointers | [Smart Pointer Guidelines](guides/SMART_POINTER_GUIDELINES.md) | Ownership |
| Best Practices | [Best Practices](guides/BEST_PRACTICES.md) | RAII Patterns |
| Examples | [FAQ](guides/FAQ.md) | RAII Usage |

### 🔧 System Integration

| Topic | Document | Section |
|-------|----------|---------|
| Quick Integration | [Integration](guides/INTEGRATION.md) | Quick Start |
| CMake Patterns | [Integration Policy](guides/INTEGRATION_POLICY.md) | CMake Config |
| Migration | [Migration Guide](advanced/MIGRATION.md) | Step-by-Step |
| Dependencies | [Dependency Matrix](advanced/DEPENDENCY_MATRIX.md) | System Graph |

### 🐛 Error Codes

| Topic | Document | Section |
|-------|----------|---------|
| Guidelines | [Error Code Guidelines](guides/ERROR_CODE_GUIDELINES.md) | Allocation |
| Adding Codes | [Error Code Guidelines](guides/ERROR_CODE_GUIDELINES.md) | New Codes |
| Validation | [Error Code Guidelines](guides/ERROR_CODE_GUIDELINES.md) | Compile-Time |
| Troubleshooting | [Troubleshooting](guides/TROUBLESHOOTING.md) | Error Codes |

---

## Project Information

### Current Status
- **Version**: 1.0 (Layer 0 Foundation)
- **Type**: Header-Only Library
- **C++ Standard**: C++17 (C++20 for optional features)
- **License**: BSD 3-Clause
- **Test Status**: Under Development

### Layer 0 Foundation
Common system provides the foundational layer for all KCENON systems:
- ✅ **Standard Interfaces** - ILogger, IMonitor, IExecutor abstractions
- ✅ **Result<T> Pattern** - Type-safe error handling with monadic operations
- ✅ **RAII Support** - Resource management utilities and guidelines
- ✅ **Smart Pointer Guidelines** - Ownership semantics and best practices
- ✅ **Error Code System** - Compile-time validated error code registry
- ✅ **Cross-Platform** - Windows, macOS, Linux support

### Key Features
- 🎯 **Header-Only** - No build artifacts, include and use
- 🔗 **Standard Interfaces** - Unified abstractions for logging, monitoring, execution
- ⚠️ **Result<T> Pattern** - Type-safe error handling replacing exceptions
- 🛡️ **RAII Patterns** - Resource safety with compile-time guarantees
- 🔧 **System Integration** - Foundation for 6 higher-layer systems
- 📦 **Dependency Injection** - Interface-based design for testability
- 🧵 **Thread Safe** - Core utilities verified with TSan
- 🔐 **Production Ready** - Used across all KCENON production systems

### Dependent Systems
Common system serves as Layer 0 foundation for:
- **Layer 1**: thread_system, container_system
- **Layer 2**: logger_system, monitoring_system, database_system
- **Layer 3**: network_system
- **Application**: messaging_system

---

## Contributing to Documentation

### Documentation Standards
Follow the [Documentation Standard](/Users/raphaelshin/Sources/template_document/DOCUMENTATION_STANDARD.md):
- Front matter on all documents
- Code examples must compile
- Bilingual support (English/Korean)
- Cross-references with relative links

### Areas for Improvement
- [ ] Korean translations for new guides (FAQ, TROUBLESHOOTING, BEST_PRACTICES)
- [ ] Video tutorials for Result<T> pattern
- [ ] Interactive examples for RAII patterns
- [ ] More integration scenarios

### Submission Process
1. Read [Contributing Guide](contributing/CONTRIBUTING.md)
2. Edit markdown files
3. Test all code examples (header-only compile checks)
4. Update Korean translations
5. Submit pull request

---

## 📞 Getting Help

### Documentation Issues
- **Missing info**: [Open documentation issue](https://github.com/kcenon/common_system/issues/new?labels=documentation)
- **Incorrect examples**: Report with details
- **Unclear instructions**: Suggest improvements

### Technical Support
1. Check [FAQ](guides/FAQ.md) - 23 common questions
2. Read [Troubleshooting](guides/TROUBLESHOOTING.md) - Template errors and solutions
3. Search [GitHub Issues](https://github.com/kcenon/common_system/issues)
4. Ask on [GitHub Discussions](https://github.com/kcenon/common_system/discussions)

### Support Resources
- **Issues**: Bug reports and feature requests
- **Discussions**: Questions and support
- **Pull Requests**: Code and documentation contributions

---

## External Resources

- **GitHub Repository**: [kcenon/common_system](https://github.com/kcenon/common_system)
- **Issue Tracker**: [GitHub Issues](https://github.com/kcenon/common_system/issues)
- **Main README**: [../README.md](../README.md)
- **Layer Architecture**: [01-ARCHITECTURE.md](01-ARCHITECTURE.md)

---

## Documentation Roadmap

### ✅ Current (v1.0 - 2025-11-11)
- ✅ Comprehensive architecture documentation
- ✅ 23 FAQ questions covering all features
- ✅ Detailed troubleshooting guide
- ✅ Best practices documentation
- ✅ Header-only specific guidelines
- ✅ Integration policy and patterns
- ✅ Migration guides
- ✅ CI/CD documentation

### 📋 Future Enhancements
- 📝 Korean translations for new guides
- 🎥 Video tutorials for Result<T> and RAII
- 📊 Interactive examples with compiler explorer
- 🌐 Multi-language support (Japanese, Chinese)
- 📖 Case studies from dependent systems
- 🔄 Migration guides for major versions

---

**Common System Documentation** - Layer 0 Foundation for C++17/20 Systems

**Last Updated**: 2025-11-11
**Next Review**: 2026-02-11
