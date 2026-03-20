# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Fixed

- Remove `const_cast` in `circuit_breaker::get_stats()` by making `failure_window` methods const-correct ([#492](https://github.com/kcenon/common_system/issues/492))
- Return error from `service_container::clear()` when container is frozen ([#494](https://github.com/kcenon/common_system/issues/494))

### Changed

- Add v1.0.0 removal timeline to deprecated `error_code` type alias ([#493](https://github.com/kcenon/common_system/issues/493))

## [0.2.0] - 2026-03-11

### Added
- Reusable release workflow and FetchContent pin checker (#433)
- Automated Apache-2.0 NOTICE file collection (#432)
- SOUP version drift detection to ecosystem CI (#424)
- Changelog generation from conventional commits (#423)

### Changed
- Standardized on vcpkg-only package management (#429)
- Pinned FetchContent examples to release tags (#422)
- Finalized fmt removal from ecosystem documentation (#435)

### Documentation
- fmt to std::format migration guide (#434)
- Transitive dependency chain documentation (#427)
- Ecosystem release pin matrix (#425)
- LICENSE-THIRD-PARTY with transitive dependencies (#431)

## [0.1.0] - 2026-03-09

### Added
- Core `Result<T>` pattern for unified error handling
- `IExecutor` interface for async execution abstraction
- `EventBus` for decoupled inter-module communication
- Health monitoring interfaces
- Configuration subsystem with file watching
- DI container and unified bootstrapper
- C++20 concepts integration
- Comprehensive adapter framework
- Feature flags framework
- Resilience patterns (circuit breaker, retry)
- Cross-system ecosystem pipeline benchmarks (#376)
- Unified compiler version enforcement (#373)
- CycloneDX SBOM-based CVE scanning (#385)
- IEC 62304 SOUP compliance documentation (#388)

### Infrastructure
- GitHub Actions CI/CD with sanitizer testing
- Doxygen documentation with reusable workflow
- vcpkg manifest with version overrides
- Cross-platform support (Linux, macOS, Windows)
