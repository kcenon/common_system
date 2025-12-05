# Changelog

All notable changes to the Common System project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

> **Language:** **English** | [한국어](CHANGELOG_KO.md)

---

## [Unreleased]

### Added
- **SystemBootstrapper**: Fluent API for system initialization at the application level (#176)
  - Factory-based logger registration for lazy initialization
  - Default logger and named logger support via `with_default_logger()` and `with_logger()`
  - Initialization hooks via `on_initialize()` (called in registration order)
  - Shutdown hooks via `on_shutdown()` (called in reverse order - LIFO)
  - RAII support with automatic shutdown on destruction
  - Move semantics support for transferring ownership
  - Prevention of duplicate initialization/shutdown
  - `reset()` method for reconfiguration scenarios
- **GlobalLoggerRegistry**: Thread-safe singleton registry for runtime logger binding (#174)
  - Centralized logger management across all subsystems
  - Default and named logger support
  - Factory-based lazy initialization for deferred logger creation
  - NullLogger fallback for safe operation when logging is not configured
  - Convenience functions: `get_registry()`, `get_logger()`, `get_logger(name)`
  - Resolves circular dependency between thread_system and logger_system
- **Unified Logging Functions and Macros** (#175)
  - `log_functions.h`: Inline logging functions with automatic source_location capture
    - Level-specific functions: `log_trace()`, `log_debug()`, `log_info()`, `log_warning()`, `log_error()`, `log_critical()`
    - Support for default logger and named loggers
    - Utility functions: `is_enabled()`, `flush()`
  - `log_macros.h`: Convenient preprocessor macros
    - Standard macros: `LOG_TRACE`, `LOG_DEBUG`, `LOG_INFO`, `LOG_WARNING`, `LOG_ERROR`, `LOG_CRITICAL`
    - Named logger macros: `LOG_*_TO(logger_name, msg)`
    - Conditional logging: `LOG_IF(level, msg)` for avoiding expensive message construction
    - Compile-time level filtering via `KCENON_MIN_LOG_LEVEL`
  - Legacy compatibility macros: `THREAD_LOG_*` redirects to `LOG_*`
- Comprehensive documentation unification across ecosystem
- Standardized CHANGELOG format following Keep a Changelog

### Removed
- **BREAKING**: `Result<T>::is_uninitialized()` method removed
  - This method was deprecated and always returned `false`
  - Use `is_err()` instead to check for error state
  - See [Migration Guide](advanced/MIGRATION.md#migrating-from-is_uninitialized) for details

---

## [1.0.0] - 2025-10-09

### Added
- **Result<T> Pattern**: Complete implementation with monadic operations (`map`, `and_then`, `or_else`)
- **Error Code Registry**: Centralized error code system with compile-time validation
  - common_system: -1 to -99
  - thread_system: -100 to -199
  - logger_system: -200 to -299
  - monitoring_system: -300 to -399
  - container_system: -400 to -499
  - database_system: -500 to -599
  - network_system: -600 to -699
- **IExecutor Interface**: Universal task execution abstraction
- **ILogger Interface**: Standard logging interface
- **IMonitor Interface**: Standard monitoring interface
- **IDatabase Interface**: Database operations interface
- **Event Bus Pattern**: Publish-subscribe mechanism for decoupled communication
- **Smart Adapters**: Type adapters for seamless type conversions
- **Factory Patterns**: Standard creation patterns

### Changed
- Separated from monolithic thread_system into independent foundation library
- Moved to pure header-only design for zero runtime overhead
- Standardized all interfaces for cross-module compatibility

### Fixed
- Interface contract ambiguities
- Error code conflicts across modules
- Thread safety issues in shared abstractions

---

## [0.9.0-beta] - 2025-09-13

### Added
- Initial separation from thread_system
- Basic Result<T> implementation
- Core interface definitions (IExecutor, ILogger)

### Changed
- Established foundation layer architecture
- Created independent repository structure

---

## Version Numbering

This project uses Semantic Versioning:
- **MAJOR** version: Incompatible API changes
- **MINOR** version: Backwards-compatible functionality additions
- **PATCH** version: Backwards-compatible bug fixes

---

## Migration Guides

### Migrating to 1.0.0

#### From thread_system integrated interfaces

**Before** (thread_system 1.x):
```cpp
#include <thread/interfaces/executor_interface.h>
using namespace thread_system::interfaces;
```

**After** (common_system 1.0):
```cpp
#include <kcenon/common/interfaces/executor_interface.h>
using namespace common::interfaces;
```

#### Result<T> Pattern Adoption

**Before** (exception-based):
```cpp
Config load_config(const std::string& path) {
    if (!exists(path)) {
        throw std::runtime_error("Config not found");
    }
    return parse_config(path);
}
```

**After** (Result<T> pattern):
```cpp
#include <kcenon/common/patterns/result.h>

common::Result<Config> load_config(const std::string& path) {
    if (!exists(path)) {
        return common::error<Config>(
            common::error_codes::NOT_FOUND,
            "Configuration file not found",
            "config_loader"
        );
    }
    return common::ok(parse_config(path));
}
```

---

## Support

- **Issues**: [GitHub Issues](https://github.com/kcenon/common_system/issues)
- **Discussions**: [GitHub Discussions](https://github.com/kcenon/common_system/discussions)
- **Email**: kcenon@naver.com

---

## License

This project is licensed under the BSD 3-Clause License - see the [LICENSE](LICENSE) file for details.

---

[Unreleased]: https://github.com/kcenon/common_system/compare/v1.0.0...HEAD
[1.0.0]: https://github.com/kcenon/common_system/releases/tag/v1.0.0
[0.9.0-beta]: https://github.com/kcenon/common_system/releases/tag/v0.9.0-beta
