# Error Code Registry

> **Language:** **English** | [한국어](ERROR_CODES.kr.md)

Comprehensive registry of all error codes across the kcenon ecosystem systems.

**Purpose**: Centralized error code documentation, conflict prevention, and cross-system error handling guidance.

**Last Updated**: 2026-02-08
**Maintained By**: kcenon team
**Source**: `/include/kcenon/common/error/error_codes.h`

---

## Table of Contents

- [Overview](#overview)
- [Error Code Range Allocation](#1-error-code-range-allocation)
- [Per-System Error Code Catalog](#2-per-system-error-code-catalog)
  - [Common Errors](#21-common-errors--1-to--99)
  - [thread_system](#22-thread_system--100-to--199)
  - [logger_system](#23-logger_system--200-to--299)
  - [monitoring_system](#24-monitoring_system--300-to--399)
  - [container_system](#25-container_system--400-to--499)
  - [database_system](#26-database_system--500-to--599)
  - [network_system](#27-network_system--600-to--699)
  - [pacs_system](#28-pacs_system--700-to--799)
  - [logger_system (Alternate)](#29-logger_system-alternate-error-codes-positive-integers)
- [Error Code Lookup](#3-error-code-lookup)
- [Cross-System Error Handling](#4-cross-system-error-handling)
- [Compile-Time Verification](#5-compile-time-verification)

---

## Overview

The kcenon ecosystem uses a **decentralized error code registry** where each system is allocated a dedicated range of negative integer codes. This prevents conflicts and enables error code namespacing by system.

### Design Principles

1. **Negative Integers**: All system error codes are negative to distinguish from success (0)
2. **100-Code Ranges**: Each system gets 100 error codes (e.g., -100 to -199)
3. **Compile-Time Validation**: Static assertions prevent range violations at compile time
4. **Category-Based**: Errors are organized by category (pool, worker, job, etc.)
5. **Human-Readable**: Helper functions convert codes to descriptive strings

### Success Code

| Code | Name | Description |
|------|------|-------------|
| `0` | SUCCESS | Operation completed successfully |

---

## 1. Error Code Range Allocation

All error codes are defined in `/include/kcenon/common/error/error_codes.h`.

| System | Range | Allocated | Used | Available | Location |
|--------|-------|----------|------|-----------|----------|
| **common_system** | -1 to -99 | 99 | 11 | 88 | `kcenon::common::error::codes::common_errors` |
| **thread_system** | -100 to -199 | 100 | 14 | 86 | `kcenon::common::error::codes::thread_system` |
| **logger_system** | -200 to -299 | 100 | 9 | 91 | `kcenon::common::error::codes::logger_system` |
| **monitoring_system** | -300 to -399 | 100 | 10 | 90 | `kcenon::common::error::codes::monitoring_system` |
| **container_system** | -400 to -499 | 100 | 12 | 88 | `kcenon::common::error::codes::container_system` |
| **database_system** | -500 to -599 | 100 | 12 | 88 | `kcenon::common::error::codes::database_system` |
| **network_system** | -600 to -699 | 100 | 11 | 89 | `kcenon::common::error::codes::network_system` |
| **pacs_system** | -700 to -799 | 100 | 24 | 76 | `kcenon::common::error::codes::pacs_system` |
| **Future Systems** | -1000+ | — | — | — | Reserved |

**Total Errors Defined**: 103 (across 8 systems)

---

## 2. Per-System Error Code Catalog

### 2.1 Common Errors (-1 to -99)

General-purpose errors used across all systems.

**Namespace**: `kcenon::common::error::codes::common_errors`

| Code | Name | Description | Usage |
|------|------|-------------|-------|
| `-1` | `invalid_argument` | Invalid function argument | Validation failures |
| `-2` | `not_found` | Resource not found | Lookup failures |
| `-3` | `permission_denied` | Permission denied | Access control |
| `-4` | `timeout` | Operation timed out | Async operations |
| `-5` | `cancelled` | Operation cancelled | User cancellation |
| `-6` | `not_initialized` | Component not initialized | Lifecycle errors |
| `-7` | `already_exists` | Resource already exists | Duplicate creation |
| `-8` | `out_of_memory` | Out of memory | Allocation failures |
| `-9` | `io_error` | I/O error | File/network I/O |
| `-10` | `network_error` | Network error | Generic network issues |
| `-11` | `registry_frozen` | Registry is frozen | Registry modification after freeze |
| `-99` | `internal_error` | Internal error | Unexpected conditions |

**Range Utilization**: 11 / 99 (11%)

---

### 2.2 thread_system (-100 to -199)

Thread pool, worker, job, and queue errors.

**Namespace**: `kcenon::common::error::codes::thread_system`

#### Pool Errors (-100 to -119)

| Code | Name | Description |
|------|------|-------------|
| `-100` | `pool_full` | Thread pool full |
| `-101` | `pool_shutdown` | Thread pool shutdown |
| `-102` | `pool_not_started` | Thread pool not started |
| `-103` | `invalid_pool_size` | Invalid pool size |

#### Worker Errors (-120 to -139)

| Code | Name | Description |
|------|------|-------------|
| `-120` | `worker_failed` | Worker failed |
| `-121` | `worker_not_found` | Worker not found |
| `-122` | `worker_busy` | Worker busy |

#### Job Errors (-140 to -159)

| Code | Name | Description |
|------|------|-------------|
| `-140` | `job_rejected` | Job rejected |
| `-141` | `job_timeout` | Job timeout |
| `-142` | `job_cancelled` | Job cancelled |
| `-143` | `invalid_job` | Invalid job |

#### Queue Errors (-160 to -179)

| Code | Name | Description |
|------|------|-------------|
| `-160` | `queue_full` | Queue full |
| `-161` | `queue_empty` | Queue empty |
| `-162` | `queue_stopped` | Queue stopped |

**Range Utilization**: 14 / 100 (14%)

---

### 2.3 logger_system (-200 to -299)

File, writer, format, and filter errors (centralized error codes).

**Namespace**: `kcenon::common::error::codes::logger_system`

#### File Errors (-200 to -219)

| Code | Name | Description |
|------|------|-------------|
| `-200` | `file_open_failed` | File open failed |
| `-201` | `file_write_failed` | File write failed |
| `-202` | `file_close_failed` | File close failed |
| `-203` | `file_rotation_failed` | File rotation failed |
| `-204` | `file_permission_denied` | File permission denied |

#### Writer Errors (-220 to -239)

| Code | Name | Description |
|------|------|-------------|
| `-220` | `writer_not_initialized` | Writer not initialized |
| `-221` | `writer_stopped` | Writer stopped |
| `-222` | `writer_full` | Writer full |
| `-223` | `async_writer_failed` | Async writer failed |

#### Format Errors (-240 to -259)

| Code | Name | Description |
|------|------|-------------|
| `-240` | `invalid_format` | Invalid format |
| `-241` | `format_error` | Format error |

#### Filter Errors (-260 to -279)

| Code | Name | Description |
|------|------|-------------|
| `-260` | `invalid_filter` | Invalid filter |
| `-261` | `filter_rejected` | Filter rejected |

**Range Utilization**: 9 / 100 (9%)

---

### 2.4 monitoring_system (-300 to -399)

Metric, storage, event, and profiler errors.

**Namespace**: `kcenon::common::error::codes::monitoring_system`

#### Metric Errors (-300 to -319)

| Code | Name | Description |
|------|------|-------------|
| `-300` | `metric_not_found` | Metric not found |
| `-301` | `invalid_metric_type` | Invalid metric type |
| `-302` | `metric_collection_failed` | Metric collection failed |

#### Storage Errors (-320 to -339)

| Code | Name | Description |
|------|------|-------------|
| `-320` | `storage_full` | Storage full |
| `-321` | `storage_error` | Storage error |

#### Event Errors (-340 to -359)

| Code | Name | Description |
|------|------|-------------|
| `-340` | `event_publish_failed` | Event publish failed |
| `-341` | `event_subscribe_failed` | Event subscribe failed |
| `-342` | `invalid_event_type` | Invalid event type |

#### Profiler Errors (-360 to -379)

| Code | Name | Description |
|------|------|-------------|
| `-360` | `profiler_not_enabled` | Profiler not enabled |
| `-361` | `profiler_error` | Profiler error |

**Range Utilization**: 10 / 100 (10%)

---

### 2.5 container_system (-400 to -499)

Value, serialization, memory pool, and container errors.

**Namespace**: `kcenon::common::error::codes::container_system`

#### Value Errors (-400 to -419)

| Code | Name | Description |
|------|------|-------------|
| `-400` | `value_type_mismatch` | Value type mismatch |
| `-401` | `invalid_value_type` | Invalid value type |
| `-402` | `value_conversion_failed` | Value conversion failed |

#### Serialization Errors (-420 to -439)

| Code | Name | Description |
|------|------|-------------|
| `-420` | `serialization_failed` | Serialization failed |
| `-421` | `deserialization_failed` | Deserialization failed |
| `-422` | `invalid_format` | Invalid format |

#### Memory Pool Errors (-440 to -459)

| Code | Name | Description |
|------|------|-------------|
| `-440` | `pool_exhausted` | Pool exhausted |
| `-441` | `allocation_failed` | Allocation failed |
| `-442` | `invalid_allocation_size` | Invalid allocation size |

#### Container Errors (-460 to -479)

| Code | Name | Description |
|------|------|-------------|
| `-460` | `key_not_found` | Key not found |
| `-461` | `duplicate_key` | Duplicate key |
| `-462` | `container_full` | Container full |

**Range Utilization**: 12 / 100 (12%)

---

### 2.6 database_system (-500 to -599)

Connection, pool, query, and transaction errors.

**Namespace**: `kcenon::common::error::codes::database_system`

#### Connection Errors (-500 to -519)

| Code | Name | Description |
|------|------|-------------|
| `-500` | `connection_failed` | Connection failed |
| `-501` | `connection_lost` | Connection lost |
| `-502` | `connection_timeout` | Connection timeout |
| `-503` | `invalid_connection_string` | Invalid connection string |

#### Pool Errors (-520 to -539)

| Code | Name | Description |
|------|------|-------------|
| `-520` | `pool_exhausted` | Pool exhausted |
| `-521` | `pool_shutdown` | Pool shutdown |
| `-522` | `pool_timeout` | Pool timeout |

#### Query Errors (-540 to -559)

| Code | Name | Description |
|------|------|-------------|
| `-540` | `query_failed` | Query failed |
| `-541` | `query_syntax_error` | Query syntax error |
| `-542` | `query_timeout` | Query timeout |

#### Transaction Errors (-560 to -579)

| Code | Name | Description |
|------|------|-------------|
| `-560` | `transaction_failed` | Transaction failed |
| `-561` | `transaction_rolled_back` | Transaction rolled back |
| `-562` | `transaction_timeout` | Transaction timeout |

**Range Utilization**: 12 / 100 (12%)

---

### 2.7 network_system (-600 to -699)

Connection, session, send/receive, and server errors.

**Namespace**: `kcenon::common::error::codes::network_system`

#### Connection Errors (-600 to -619)

| Code | Name | Description |
|------|------|-------------|
| `-600` | `connection_failed` | Connection failed |
| `-601` | `connection_refused` | Connection refused |
| `-602` | `connection_timeout` | Connection timeout |
| `-603` | `connection_closed` | Connection closed |

#### Session Errors (-620 to -639)

| Code | Name | Description |
|------|------|-------------|
| `-620` | `session_not_found` | Session not found |
| `-621` | `session_expired` | Session expired |
| `-622` | `invalid_session` | Invalid session |

#### Send/Receive Errors (-640 to -659)

| Code | Name | Description |
|------|------|-------------|
| `-640` | `send_failed` | Send failed |
| `-641` | `receive_failed` | Receive failed |
| `-642` | `message_too_large` | Message too large |

#### Server Errors (-660 to -679)

| Code | Name | Description |
|------|------|-------------|
| `-660` | `server_not_started` | Server not started |
| `-661` | `server_already_running` | Server already running |
| `-662` | `bind_failed` | Bind failed |

**Range Utilization**: 11 / 100 (11%)

---

### 2.8 pacs_system (-700 to -799)

DICOM file, element, encoding, network, and storage errors.

**Namespace**: `kcenon::common::error::codes::pacs_system`

#### DICOM File Errors (-700 to -719)

| Code | Name | Description |
|------|------|-------------|
| `-700` | `file_not_found` | DICOM file not found |
| `-701` | `file_read_error` | Failed to read DICOM file |
| `-702` | `file_write_error` | Failed to write DICOM file |
| `-703` | `invalid_dicom_file` | Invalid DICOM file format |
| `-704` | `missing_dicm_prefix` | Missing DICM prefix |
| `-705` | `invalid_meta_info` | Invalid File Meta Information |
| `-706` | `missing_transfer_syntax` | Missing Transfer Syntax |
| `-707` | `unsupported_transfer_syntax` | Unsupported Transfer Syntax |

#### DICOM Element Errors (-720 to -739)

| Code | Name | Description |
|------|------|-------------|
| `-720` | `element_not_found` | DICOM element not found |
| `-721` | `value_conversion_error` | Value conversion failed |
| `-722` | `invalid_vr` | Invalid VR (Value Representation) |
| `-723` | `invalid_tag` | Invalid DICOM tag |
| `-724` | `data_size_mismatch` | Data size mismatch |

#### Encoding/Decoding Errors (-740 to -759)

| Code | Name | Description |
|------|------|-------------|
| `-740` | `decode_error` | DICOM decode error |
| `-741` | `encode_error` | DICOM encode error |
| `-742` | `compression_error` | Compression error |
| `-743` | `decompression_error` | Decompression error |

#### Network/Association Errors (-760 to -779)

| Code | Name | Description |
|------|------|-------------|
| `-760` | `association_rejected` | DICOM association rejected |
| `-761` | `association_aborted` | DICOM association aborted |
| `-762` | `dimse_error` | DIMSE protocol error |
| `-763` | `pdu_error` | PDU error |

#### Storage Errors (-780 to -799)

| Code | Name | Description |
|------|------|-------------|
| `-780` | `storage_failed` | DICOM storage failed |
| `-781` | `retrieve_failed` | DICOM retrieve failed |
| `-782` | `query_failed` | DICOM query failed |

**Range Utilization**: 24 / 100 (24%)

---

### 2.9 logger_system (Alternate Error Codes: Positive Integers)

**Note**: logger_system also defines a **separate, internal error code system** using positive integers (0-1999) in `/include/kcenon/logger/core/error_codes.h`.

**Purpose**: Detailed, logger-specific error codes for internal operations.

**Namespace**: `kcenon::logger::logger_error_code`

**Error Code Ranges** (Positive Integers):

| Range | Category | Examples |
|-------|----------|----------|
| `0-999` | General errors | `success (0)`, `unknown_error (1)`, `not_implemented (2)`, `invalid_argument (3)` |
| `1000-1099` | Writer errors | `writer_not_found (1000)`, `writer_initialization_failed (1001)`, `writer_already_exists (1002)` |
| `1100-1199` | File errors | `file_open_failed (1100)`, `file_write_failed (1101)`, `file_rotation_failed (1102)` |
| `1200-1299` | Network errors | `network_connection_failed (1200)`, `network_send_failed (1201)`, `network_timeout (1202)` |
| `1300-1399` | Buffer/Queue errors | `buffer_overflow (1300)`, `queue_full (1301)`, `queue_stopped (1302)` |
| `1400-1499` | Configuration errors | `invalid_configuration (1400)`, `configuration_missing (1401)` |
| `1500-1599` | Metrics errors | `metrics_collection_failed (1500)`, `metrics_not_available (1501)` |
| `1600-1699` | Processing errors | `flush_timeout (1600)`, `processing_failed (1601)`, `filter_error (1602)`, `formatter_error (1603)` |
| `1700-1799` | Security errors | `encryption_failed (1700)`, `decryption_failed (1701)`, `authentication_failed (1702)` |
| `1800-1899` | DI Container errors | `di_not_available (1800)`, `component_not_found (1801)`, `registration_failed (1802)` |
| `1900-1999` | Writer errors | `writer_not_available (1900)`, `writer_configuration_error (1901)` |

**Total Codes Defined**: ~60 positive integer codes

**Usage**: Primarily for internal logger_system error handling. For cross-system errors, use the centralized negative codes (`-200` to `-299`).

**Conversion Function**:

```cpp
std::string logger_error_to_string(logger_error_code code);
```

---

## 3. Error Code Lookup

### 3.1 Programmatic Lookup

Use the helper functions in `kcenon::common::error` namespace:

```cpp
#include <kcenon/common/error/error_codes.h>

using namespace kcenon::common::error;

// Get human-readable error message
int code = codes::thread_system::pool_full;
std::string_view message = get_error_message(code);
// Returns: "Thread pool full"

// Get category name
std::string_view category = get_category_name(code);
// Returns: "ThreadSystem"
```

### 3.2 Error Message Mapping

The `get_error_message(int code)` function maps all defined error codes to human-readable strings. Unmapped codes return `"Unknown error"`.

**Example Mappings**:

| Code | Message |
|------|---------|
| `0` | "Success" |
| `-1` | "Invalid argument" |
| `-100` | "Thread pool full" |
| `-200` | "Failed to open log file" |
| `-300` | "Metric not found" |
| `-400` | "Value type mismatch" |
| `-500` | "Database connection failed" |
| `-600` | "Network connection failed" |
| `-700` | "DICOM file not found" |

### 3.3 Category Detection

The `get_category_name(int code)` function returns the system name based on error code range:

```cpp
get_category_name(0);     // "Success"
get_category_name(-42);   // "Common"
get_category_name(-142);  // "ThreadSystem"
get_category_name(-242);  // "LoggerSystem"
get_category_name(-342);  // "MonitoringSystem"
get_category_name(-442);  // "ContainerSystem"
get_category_name(-542);  // "DatabaseSystem"
get_category_name(-642);  // "NetworkSystem"
get_category_name(-742);  // "PACSSystem"
```

---

## 4. Cross-System Error Handling

### 4.1 Error Propagation Patterns

When errors cross system boundaries, use the `common::Result<T>` pattern to wrap and propagate errors with context.

**Example 1: Thread System → Logger System**

```cpp
#include <kcenon/common/error/error_codes.h>
#include <kcenon/common/patterns/result.h>

using namespace kcenon::common;

// Thread system operation
Result<int> submit_job(std::function<int()> job) {
    if (pool_is_full()) {
        return make_error<int>(
            error::codes::thread_system::pool_full,
            "Thread pool exhausted, cannot submit job",
            "thread_system"
        );
    }
    // ... submit logic
    return ok(job_id);
}

// Caller in logger system
void log_with_background_processing() {
    auto result = submit_job([]() { /* background log write */ return 0; });

    if (result.is_err()) {
        // Log error with original context
        logger->error("Background job submission failed: {} (code: {})",
                     result.error().message,
                     result.error().code);

        // Optionally wrap with logger-specific error
        auto wrapped_error = make_error<void>(
            error::codes::logger_system::async_writer_failed,
            "Async writer failed: " + result.error().message,
            "logger_system"
        );
        // Handle or propagate wrapped_error
    }
}
```

**Example 2: Database System → Network System**

```cpp
// Database operation
Result<UserData> fetch_user(int user_id) {
    if (!connection_pool.has_available()) {
        return make_error<UserData>(
            error::codes::database_system::pool_exhausted,
            "No database connections available",
            "database_system"
        );
    }
    // ... query logic
    return ok(user_data);
}

// Network handler
void handle_request(Request req) {
    auto result = fetch_user(req.user_id);

    if (result.is_err()) {
        // Convert to network error
        send_error_response(req, {
            .status_code = 503,  // Service Unavailable
            .error_code = error::codes::network_system::send_failed,
            .message = "Database unavailable: " + result.error().message
        });
    } else {
        send_json_response(req, result.value());
    }
}
```

### 4.2 Error Context Addition

When propagating errors, add context using the `error_info` struct:

```cpp
Result<ConfigData> load_config_with_context(const std::string& path) {
    auto file_result = open_file(path);

    if (file_result.is_err()) {
        // Add context to error
        return make_error<ConfigData>(
            file_result.error().code,
            file_result.error().message + " (file: " + path + ")",
            file_result.error().category + "/config_loader"
        );
    }

    // ... parse config
}
```

### 4.3 Best Practices

1. **Preserve Original Error Code**: When wrapping errors, keep the original code in logs or error chains
2. **Add Context**: Each layer should add meaningful context (file path, operation name, etc.)
3. **Use System-Specific Codes**: Only use your system's error code range for new errors
4. **Document Error Flows**: In complex error propagation, document which systems errors flow through
5. **Avoid Error Code Reuse**: Don't create new errors with codes from another system's range

---

## 5. Compile-Time Verification

### 5.1 Static Assertions

The `error_codes.h` file includes **compile-time validation** to prevent error code range violations:

```cpp
// From kcenon::common::error::validation namespace

// Validate thread_system range (-100 to -199)
static_assert(codes::thread_system::base == -100,
              "thread_system base must be -100");
static_assert(codes::thread_system::pool_full >= -199 &&
              codes::thread_system::pool_full <= -100,
              "thread_system error codes must be in range [-199, -100]");

// Similar assertions for all systems...
```

**Benefits**:
- Errors are caught at **compile time**, not runtime
- Prevents accidental range violations when adding new error codes
- Self-documenting: static_assert messages explain range requirements

### 5.2 Adding New Error Codes

When adding a new error code, follow these steps:

1. **Choose Correct Namespace**: Add to the appropriate system namespace
2. **Use Base Offset**: Calculate code as `base - offset`
3. **Stay Within Range**: Ensure code stays within system's 100-code range
4. **Add Static Assertion**: Add a new static_assert in the `validation` namespace
5. **Update `get_error_message()`**: Add mapping to human-readable string

**Example: Adding a new thread_system error**

```cpp
// In kcenon::common::error::codes::thread_system namespace
constexpr int policy_conflict = base - 70;  // = -170

// In validation namespace
static_assert(codes::thread_system::policy_conflict >= -199 &&
              codes::thread_system::policy_conflict <= -100,
              "thread_system error codes must be in range [-199, -100]");

// In get_error_message()
case codes::thread_system::policy_conflict:
    return "Thread pool policy conflict";
```

### 5.3 Verification Script

**Optional**: Create a Python script to verify error code uniqueness at CI time:

```python
#!/usr/bin/env python3
# scripts/verify_error_codes.py

import re
import sys

def parse_error_codes(header_file):
    """Parse error codes from error_codes.h"""
    codes = {}
    with open(header_file) as f:
        content = f.read()
        # Extract constexpr int definitions
        pattern = r'constexpr int (\w+) = (-?\d+);'
        for match in re.finditer(pattern, content):
            name, code = match.groups()
            code = int(code)
            if code in codes:
                print(f"ERROR: Duplicate error code {code}: {name} and {codes[code]}")
                sys.exit(1)
            codes[code] = name
    return codes

def verify_ranges(codes):
    """Verify error codes are within expected ranges"""
    ranges = {
        'common': (-99, -1),
        'thread': (-199, -100),
        'logger': (-299, -200),
        'monitoring': (-399, -300),
        'container': (-499, -400),
        'database': (-599, -500),
        'network': (-699, -600),
        'pacs': (-799, -700),
    }

    for code, name in codes.items():
        if code == 0:
            continue  # Success code

        for system, (min_code, max_code) in ranges.items():
            if min_code <= code <= max_code:
                break
        else:
            print(f"ERROR: Code {code} ({name}) not in any valid range")
            sys.exit(1)

if __name__ == '__main__':
    codes = parse_error_codes('include/kcenon/common/error/error_codes.h')
    verify_ranges(codes)
    print(f"✅ Verification passed: {len(codes)} error codes verified")
```

**CI Integration** (`.github/workflows/error-code-check.yml`):

```yaml
name: Error Code Verification

on: [push, pull_request]

jobs:
  verify-error-codes:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v3
      - name: Verify error codes
        run: python3 scripts/verify_error_codes.py
```

---

## Related Documentation

- [Architecture Overview](ARCHITECTURE.md) - System design philosophy
- [API Reference](API_REFERENCE.md) - C++ interface specifications
- [Integration Guide](INTEGRATION_GUIDE.md) - Cross-system integration patterns
- [Result Pattern](../include/kcenon/common/patterns/result.h) - Error handling pattern

---

**Document Version**: 1.0.0
**Last Updated**: 2026-02-08
**Authors**: kcenon team
**Related Issues**: kcenon/common_system#331
**Source File**: `/include/kcenon/common/error/error_codes.h`
