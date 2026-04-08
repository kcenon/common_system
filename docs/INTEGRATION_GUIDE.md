---
doc_id: "COM-INTR-002"
doc_title: "Cross-System Integration Guide"
doc_version: "1.0.0"
doc_date: "2026-04-04"
doc_status: "Released"
project: "common_system"
category: "INTR"
---

# Cross-System Integration Guide

> **SSOT**: This document is the single source of truth for **Cross-System Integration Guide**.

> **Language:** **English** | [한국어](INTEGRATION_GUIDE.kr.md)

This guide provides a comprehensive overview of how the 7 kcenon ecosystem systems work together. It is split into three focused sub-documents:

| Document | Contents |
|----------|----------|
| [Dependency Map](INTEGRATION_DEPENDENCY_MAP.md) | Overview, ecosystem dependency map (tiers, initialization order, rationale), integration patterns (bootstrapper, manual wiring, service container, lifecycle management) |
| [Patterns](INTEGRATION_PATTERNS.md) | Common integration scenarios (Web API, data pipeline, monitoring, full-stack), configuration across systems, error handling and Result composition |
| [Lifecycle](INTEGRATION_LIFECYCLE.md) | Initialization and shutdown sequences, timeout/error recovery, complete example application, related documentation |
