# Changelog

All notable changes to the Common System project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

> **Language:** **English** | [한국어](CHANGELOG_KO.md)

---

## [Unreleased]

### Breaking Changes
- **Removed deprecated file/line/function log() method from ILogger interface** (#217)
  - Removed: `virtual VoidResult log(log_level, const std::string&, const std::string& file, int line, const std::string& function)`
  - Use `log(log_level, std::string_view, const source_location&)` instead
  - Custom ILogger implementations must remove this method override
  - See `docs/DEPRECATION.md` for migration guide

### Added
- **Unified Transport Interfaces** (#233)
  - New `include/kcenon/common/interfaces/transport/` directory with transport abstractions
  - `IHttpClient`: HTTP client interface for dependency injection
    - `http_request`/`http_response` structs with builder pattern support
    - Status helpers: `is_success()`, `is_client_error()`, `is_server_error()`
    - `null_http_client`: No-op implementation for disabled transport
  - `IUdpClient`: UDP client interface for metric reporting and low-latency messaging
    - Connected mode (`connect()` + `send()`) with kernel routing optimization
    - Connectionless mode (`send_to()`) for ad-hoc packet delivery
    - Statistics tracking: `packets_sent`, `bytes_sent`, `send_failures`
    - String convenience methods for easy metric string transmission
    - `null_udp_client`: No-op implementation for disabled transport
  - `transport.h`: Umbrella header for all transport interfaces
  - Header-only design with no runtime dependencies
  - Comprehensive unit tests (26 tests) for interface concepts

- **KCENON_WITH_COMMON_SYSTEM Flag** (#230)
  - Added `KCENON_WITH_COMMON_SYSTEM=1` to `feature_system_deps.h`
  - Auto-defined when `feature_flags.h` is included
  - Exported via CMake compile definitions for dependent projects
  - Prevents ABI incompatibility in downstream projects (e.g., network_system)
  - Added `COMMON_SYSTEM` option to `kcenon_configure_features()` function

- **Unified Feature Flag Headers** (#224)
  - New `feature_flags.h` as main entry point for all feature detection
  - `feature_flags_core.h`: Preprocessor helpers, compiler/platform detection (KCENON_COMPILER_*, KCENON_PLATFORM_*, KCENON_HAS_CPP*)
  - `feature_detection.h`: C++ standard library feature detection
    - KCENON_HAS_SOURCE_LOCATION (C++20 std::source_location)
    - KCENON_HAS_JTHREAD / KCENON_HAS_STOP_TOKEN (C++20 cooperative threading)
    - KCENON_HAS_FORMAT, KCENON_HAS_SPAN, KCENON_HAS_RANGES (C++20)
    - KCENON_HAS_EXPECTED, KCENON_HAS_STACKTRACE (C++23)
    - KCENON_HAS_CONCEPTS, KCENON_HAS_COROUTINES
  - `feature_system_deps.h`: System module integration flags (KCENON_WITH_THREAD_SYSTEM, KCENON_WITH_LOGGER_SYSTEM, etc.)
  - Legacy aliases available via KCENON_ENABLE_LEGACY_ALIASES (default: enabled)
  - Updated `features.cmake` with `export_kcenon_features()` function
  - Refactored `source_location.h` to use unified feature detection

- **Downstream System Deprecation Notifications** (#220)
  - Created migration tracking issues in all dependent systems
  - thread_system, logger_system, monitoring_system, pacs_system, database_system notified
  - Each issue includes migration guide links and v3.0.0 removal timeline
  - Updated `docs/DEPRECATION.md` with notification status table

- **Deprecated API Documentation** (#213)
  - New `docs/DEPRECATION.md` documenting all deprecated APIs
  - Migration guides with before/after examples
  - Compiler warning suppression instructions
  - CI integration guidance for deprecation warnings
  - Timeline for v3.0.0 API removal
  - Both English and Korean versions

- **Unified Version Management System** (#204)
  - New `docs/COMPATIBILITY.md` with version compatibility matrix
  - Document version requirements for all 7 KCENON ecosystem systems
  - Dependency graph showing system relationships
  - CMake version validation examples for dependent systems
  - Safe upgrade order guidelines
  - Known incompatibilities and resolutions documentation
  - Both English and Korean versions
  - New `docs/contributing/CHANGELOG_TEMPLATE.md` for ecosystem-wide standardization

- **C++20 Concepts for Type Validation** (#192)
  - New `include/kcenon/common/concepts/` directory with comprehensive concept definitions
  - `core.h`: Result/Optional concepts (Resultable, Unwrappable, Mappable, Chainable, MonadicResult)
  - `callable.h`: Callable concepts (Invocable, VoidCallable, Predicate, JobLike, ExecutorLike)
  - `event.h`: Event bus concepts (EventType, EventHandler, EventFilter, TimestampedEvent)
  - `service.h`: DI concepts (ServiceInterface, ServiceImplementation, ServiceFactory)
  - `container.h`: Container concepts (Container, SequenceContainer, CircularBuffer)
  - `concepts.h`: Unified header for all concepts
  - Added `value_type` and `error_type` type aliases to `Result<T>` for concept compatibility
  - Added `value_type` type alias to `Optional<T>` for concept compatibility
  - Applied concepts to `simple_event_bus` (publish, subscribe, subscribe_filtered methods)
  - Applied concepts to `IServiceContainer` (register_type, register_factory, register_simple_factory methods)

### Benefits
- **Clearer compile-time errors**: Template errors now show concept violations instead of SFINAE failures
- **Self-documenting code**: Concepts explicitly express type requirements
- **Reduced boilerplate**: Eliminates `std::enable_if` and `static_assert` noise
- **Better IDE support**: Improved auto-completion and type hints

---

## [2.0.0] - 2025-12-07

### Added
- **Cross-System Integration Tests**: Comprehensive tests for runtime binding pattern (#178)
  - GlobalLoggerRegistry integration tests (7 tests)
    - MultipleSystemsShareLogger: Verifies systems share unified logger
    - ThreadSafeAccess: 100 threads × 10 logs concurrent access
    - ConcurrentRegistrationAndRetrieval: Mixed register/retrieve operations
    - FactoryBasedLazyInitialization: Factory called only on first access
    - NullLoggerFallback: Safe fallback for unregistered loggers
    - StressTestHighConcurrency: 50 threads × 1000 mixed operations
    - CleanupAfterHeavyUsage: Proper cleanup after bulk operations
  - SystemBootstrapper integration tests (6 tests)
    - InitializeAndShutdown: Basic lifecycle verification
    - ShutdownHooksExecuteInOrder: LIFO order verification
    - InitializationHooksExecuteInOrder: FIFO order verification
    - MultipleNamedLoggers: Default + named logger registration
    - DoubleInitializationPrevented: Error on duplicate init
    - RAIIShutdownOnDestruction: Automatic cleanup on scope exit
  - CrossSystem integration tests (3 tests)
    - LoggingFromMultipleSystems: Unified logging from mock systems
    - ConcurrentCrossSystemLogging: Thread-safe cross-system logging
    - PerSystemNamedLoggers: Separate loggers per system type
  - LevelConversion tests (4 tests)
    - AllLevelsConvertCorrectly: trace through critical verification
    - LevelFilteringWorks: is_enabled() behavior verification
    - LevelStringRoundtrip: to_string/from_string inverse
    - CaseInsensitiveLevelParsing: Case-insensitive level parsing
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
- **ILogger source_location Support** (#177)
  - Extended `ILogger` interface with C++20 `source_location` support
  - New preferred method: `log(log_level, std::string_view, const source_location&)`
  - Default implementation in base class delegates to legacy method for backward compatibility
  - Extended `log_entry` struct with `source_location location` field
  - New factory method: `log_entry::create()` automatically captures source_location
  - Updated logging functions to use new source_location-based interface
- Comprehensive documentation unification across ecosystem
- Standardized CHANGELOG format following Keep a Changelog

### Deprecated
- **ILogger legacy file/line/function method** (#177)
  - `log(log_level, const std::string&, const std::string&, int, const std::string&)` is now deprecated
  - Use `log(log_level, std::string_view, const source_location&)` instead
  - Will be removed in v3.0.0

### Removed
- **BREAKING**: `Result<T>::is_uninitialized()` method removed
  - This method was deprecated and always returned `false`
  - Use `is_err()` instead to check for error state
  - See [Migration Guide](advanced/MIGRATION.md#migrating-from-is_uninitialized) for details
- **BREAKING**: Deprecated Result factory functions removed (#180)
  - `Ok<T>(value)` - use lowercase `ok<T>(value)` instead
  - `Err<T>(message)` and `Err<T>(code, message, module)` - use `make_error<T>()` instead
- **BREAKING**: Legacy macro aliases removed (#180)
  - `RETURN_IF_ERROR` - use `COMMON_RETURN_IF_ERROR` instead
  - `ASSIGN_OR_RETURN` - use `COMMON_ASSIGN_OR_RETURN` instead
  - `RETURN_ERROR_IF` - use `COMMON_RETURN_ERROR_IF` instead
  - `RETURN_ERROR_IF_WITH_DETAILS` - use `COMMON_RETURN_ERROR_IF_WITH_DETAILS` instead
- **BREAKING**: Legacy logging macros removed (#180)
  - `THREAD_LOG_TRACE`, `THREAD_LOG_DEBUG`, `THREAD_LOG_INFO`,
    `THREAD_LOG_WARNING`, `THREAD_LOG_ERROR`, `THREAD_LOG_CRITICAL`
  - Use `LOG_TRACE`, `LOG_DEBUG`, `LOG_INFO`, `LOG_WARNING`, `LOG_ERROR`, `LOG_CRITICAL` instead

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

[Unreleased]: https://github.com/kcenon/common_system/compare/v2.0.0...HEAD
[2.0.0]: https://github.com/kcenon/common_system/releases/tag/v2.0.0
[1.0.0]: https://github.com/kcenon/common_system/releases/tag/v1.0.0
[0.9.0-beta]: https://github.com/kcenon/common_system/releases/tag/v0.9.0-beta
