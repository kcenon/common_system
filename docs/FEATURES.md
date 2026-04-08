---
doc_id: "COM-FEAT-002"
doc_title: "Common System - Detailed Features"
doc_version: "1.0.0"
doc_date: "2026-04-04"
doc_status: "Released"
project: "common_system"
category: "FEAT"
---

# Common System - Detailed Features

> **SSOT**: This document is the single source of truth for **Common System - Detailed Features**.

**Language:** **English** | [한국어](FEATURES.kr.md)

This document was split into focused sub-documents for readability. See the sections below.

| Document | Contents |
|----------|----------|
| [FEATURES_CORE.md](FEATURES_CORE.md) | Core Advantages & Benefits, Core Components (Result\<T\>, IExecutor, Event Bus), Resilience Patterns (Circuit Breaker), Error Handling Foundation |
| [FEATURES_DI_CONFIG.md](FEATURES_DI_CONFIG.md) | Dependency Injection (Service Container, scoped lifetimes, circular dependency detection), System Bootstrapper, Unified Bootstrapper |
| [FEATURES_INTEGRATION.md](FEATURES_INTEGRATION.md) | Ecosystem Integration (thread/network/logger systems), Feature Flags, Production Quality (CI, thread safety, RAII), Advanced Features (C++20 source location, ABI versioning) |
