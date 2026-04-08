---
doc_id: "COM-GUID-003"
doc_title: "Configuration Subsystem Guide"
doc_version: "1.0.0"
doc_date: "2026-04-04"
doc_status: "Released"
project: "common_system"
category: "GUID"
---

# Configuration Subsystem Guide

> **Language:** **English** | [한국어](CONFIG_GUIDE.kr.md)

Complete guide for the configuration subsystem in common_system: unified configuration management, hot-reload file watching, CLI argument parsing, and YAML configuration loading.

## Sub-Documents

| Document | Description |
|----------|-------------|
| [CONFIG_UNIFIED.md](CONFIG_UNIFIED.md) | Overview, configuration architecture, unified config schema (`unified_config.h`), environment variable reference, and YAML configuration examples |
| [CONFIG_WATCHER.md](CONFIG_WATCHER.md) | Config watcher (`config_watcher.h`) for file system monitoring, debouncing, version tracking, rollback support, and hot-reload patterns |
| [CONFIG_CLI_PARSER.md](CONFIG_CLI_PARSER.md) | CLI config parser (`cli_config_parser.h`) for command-line argument parsing, boolean flags, list arguments, and help text generation |
| [CONFIG_LOADER.md](CONFIG_LOADER.md) | Config loader (`config_loader.h`) for YAML file loading, schema validation, environment variable substitution, complete usage examples, and troubleshooting |
