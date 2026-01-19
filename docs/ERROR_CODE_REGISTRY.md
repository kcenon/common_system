> **Language:** **English** | [한국어](ERROR_CODE_REGISTRY.kr.md)

# Error Code Registry

This document provides a complete reference of all error codes used across the unified_system ecosystem.

## Error Code Ranges

| System | Range | Namespace |
|--------|-------|-----------|
| Success | 0 | `common_errors::success` |
| Common | -1 to -99 | `codes::common_errors` |
| thread_system | -100 to -199 | `codes::thread_system` |
| logger_system | -200 to -299 | `codes::logger_system` |
| monitoring_system | -300 to -399 | `codes::monitoring_system` |
| container_system | -400 to -499 | `codes::container_system` |
| database_system | -500 to -599 | `codes::database_system` |
| network_system | -600 to -699 | `codes::network_system` |
| Reserved | -1000+ | Future systems |

---

## Common Error Codes (-1 to -99)

These errors can occur in any system.

| Code | Name | Description | Possible Causes | Resolution |
|------|------|-------------|-----------------|------------|
| 0 | `success` | Operation completed successfully | - | - |
| -1 | `invalid_argument` | Invalid argument provided | Null pointer, out-of-range value, wrong type | Validate inputs before calling |
| -2 | `not_found` | Requested resource not found | Missing file, unregistered handler, deleted object | Check resource existence first |
| -3 | `permission_denied` | Insufficient permissions | Wrong credentials, missing access rights | Check permissions/credentials |
| -4 | `timeout` | Operation timed out | Slow network, deadlock, overloaded system | Increase timeout or retry |
| -5 | `cancelled` | Operation was cancelled | User cancellation, shutdown | Check cancellation before use |
| -6 | `not_initialized` | Component not initialized | Missing init call, wrong order | Call initialize() first |
| -7 | `already_exists` | Resource already exists | Duplicate registration, file exists | Check existence before creating |
| -8 | `out_of_memory` | Memory allocation failed | Large allocation, memory leak | Free unused resources |
| -9 | `io_error` | I/O operation failed | Disk full, file locked, hardware error | Check storage/permissions |
| -10 | `network_error` | Network operation failed | No connectivity, DNS failure | Check network connection |
| -11 | `registry_frozen` | Registry is frozen | Modification after freeze | Expected behavior; use before freeze |
| -99 | `internal_error` | Internal system error | Bug, unexpected state | Report issue with logs |

---

## thread_system Error Codes (-100 to -199)

Errors related to thread pools, workers, and job scheduling.

### Pool Errors (-100 to -119)

| Code | Name | Description | Possible Causes | Resolution |
|------|------|-------------|-----------------|------------|
| -100 | `pool_full` | Thread pool is at capacity | Too many concurrent jobs | Wait or increase pool size |
| -101 | `pool_shutdown` | Thread pool is shutting down | Shutdown called | Don't submit new jobs |
| -102 | `pool_not_started` | Thread pool not started | start() not called | Call start() first |
| -103 | `invalid_pool_size` | Invalid pool size specified | Zero or negative size | Use positive pool size |

### Worker Errors (-120 to -139)

| Code | Name | Description | Possible Causes | Resolution |
|------|------|-------------|-----------------|------------|
| -120 | `worker_failed` | Worker thread failed | Unhandled exception, crash | Check error logs |
| -121 | `worker_not_found` | Worker not found | Invalid worker ID | Verify worker exists |
| -122 | `worker_busy` | Worker is busy | Long-running task | Wait or use different worker |

### Job Errors (-140 to -159)

| Code | Name | Description | Possible Causes | Resolution |
|------|------|-------------|-----------------|------------|
| -140 | `job_rejected` | Job was rejected | Pool full, invalid job | Retry or check job validity |
| -141 | `job_timeout` | Job execution timed out | Infinite loop, deadlock | Increase timeout or fix job |
| -142 | `job_cancelled` | Job was cancelled | Manual cancellation | Expected behavior |
| -143 | `invalid_job` | Invalid job specification | Null function, bad params | Validate job before submit |

### Queue Errors (-160 to -179)

| Code | Name | Description | Possible Causes | Resolution |
|------|------|-------------|-----------------|------------|
| -160 | `queue_full` | Job queue is full | Too many pending jobs | Wait or increase queue size |
| -161 | `queue_empty` | Job queue is empty | No pending jobs | Wait for new jobs |
| -162 | `queue_stopped` | Queue has stopped | Shutdown in progress | Expected during shutdown |

---

## logger_system Error Codes (-200 to -299)

Errors related to logging and log file management.

### File Errors (-200 to -219)

| Code | Name | Description | Possible Causes | Resolution |
|------|------|-------------|-----------------|------------|
| -200 | `file_open_failed` | Cannot open log file | Invalid path, permissions | Check path and permissions |
| -201 | `file_write_failed` | Cannot write to log file | Disk full, file locked | Check disk space |
| -202 | `file_close_failed` | Cannot close log file | System error | Check for resource leaks |
| -203 | `file_rotation_failed` | Log rotation failed | Disk full, permissions | Check disk/permissions |
| -204 | `file_permission_denied` | No permission for file | Wrong owner, ACL | Fix file permissions |

### Writer Errors (-220 to -239)

| Code | Name | Description | Possible Causes | Resolution |
|------|------|-------------|-----------------|------------|
| -220 | `writer_not_initialized` | Logger not initialized | Missing init call | Initialize logger first |
| -221 | `writer_stopped` | Logger writer stopped | Shutdown called | Restart logger |
| -222 | `writer_full` | Write buffer full | Too many log entries | Wait or flush |
| -223 | `async_writer_failed` | Async writer failed | Thread issue | Check async configuration |

### Format Errors (-240 to -259)

| Code | Name | Description | Possible Causes | Resolution |
|------|------|-------------|-----------------|------------|
| -240 | `invalid_format` | Invalid log format | Bad format string | Fix format string |
| -241 | `format_error` | Formatting failed | Type mismatch | Check argument types |

### Filter Errors (-260 to -279)

| Code | Name | Description | Possible Causes | Resolution |
|------|------|-------------|-----------------|------------|
| -260 | `invalid_filter` | Invalid filter config | Bad regex, null filter | Validate filter |
| -261 | `filter_rejected` | Log rejected by filter | Expected behavior | Adjust filter settings |

---

## monitoring_system Error Codes (-300 to -399)

Errors related to metrics, events, and profiling.

### Metric Errors (-300 to -319)

| Code | Name | Description | Possible Causes | Resolution |
|------|------|-------------|-----------------|------------|
| -300 | `metric_not_found` | Metric not registered | Typo, not created | Register metric first |
| -301 | `invalid_metric_type` | Wrong metric type | Counter vs Gauge | Use correct type |
| -302 | `metric_collection_failed` | Failed to collect | System error | Check permissions |

### Storage Errors (-320 to -339)

| Code | Name | Description | Possible Causes | Resolution |
|------|------|-------------|-----------------|------------|
| -320 | `storage_full` | Metric storage full | Too many metrics | Increase limit or prune |
| -321 | `storage_error` | Storage operation failed | I/O error | Check storage system |

### Event Errors (-340 to -359)

| Code | Name | Description | Possible Causes | Resolution |
|------|------|-------------|-----------------|------------|
| -340 | `event_publish_failed` | Cannot publish event | Bus not running | Start event bus |
| -341 | `event_subscribe_failed` | Cannot subscribe | Invalid handler | Fix handler function |
| -342 | `invalid_event_type` | Unknown event type | Type mismatch | Use correct event type |

### Profiler Errors (-360 to -379)

| Code | Name | Description | Possible Causes | Resolution |
|------|------|-------------|-----------------|------------|
| -360 | `profiler_not_enabled` | Profiler is disabled | Not enabled at build | Enable profiler |
| -361 | `profiler_error` | Profiler error | Resource exhaustion | Check profiler config |

---

## container_system Error Codes (-400 to -499)

Errors related to value containers and memory pools.

### Value Errors (-400 to -419)

| Code | Name | Description | Possible Causes | Resolution |
|------|------|-------------|-----------------|------------|
| -400 | `value_type_mismatch` | Value type mismatch | Wrong get<T>() type | Check stored type |
| -401 | `invalid_value_type` | Unsupported value type | Not registered | Register type first |
| -402 | `value_conversion_failed` | Type conversion failed | Incompatible types | Use compatible types |

### Serialization Errors (-420 to -439)

| Code | Name | Description | Possible Causes | Resolution |
|------|------|-------------|-----------------|------------|
| -420 | `serialization_failed` | Cannot serialize | Complex type, circular ref | Simplify data structure |
| -421 | `deserialization_failed` | Cannot deserialize | Corrupted data, version | Check data format |
| -422 | `invalid_format` | Invalid data format | Wrong encoding | Use correct format |

### Memory Pool Errors (-440 to -459)

| Code | Name | Description | Possible Causes | Resolution |
|------|------|-------------|-----------------|------------|
| -440 | `pool_exhausted` | Memory pool exhausted | High allocation rate | Increase pool size |
| -441 | `allocation_failed` | Allocation failed | Out of memory | Free resources |
| -442 | `invalid_allocation_size` | Invalid size requested | Zero or negative | Use valid size |

### Container Errors (-460 to -479)

| Code | Name | Description | Possible Causes | Resolution |
|------|------|-------------|-----------------|------------|
| -460 | `key_not_found` | Key not in container | Wrong key, deleted | Check key existence |
| -461 | `duplicate_key` | Key already exists | Double insert | Check before insert |
| -462 | `container_full` | Container at capacity | Size limit reached | Remove items or resize |

---

## database_system Error Codes (-500 to -599)

Errors related to database connections and queries.

### Connection Errors (-500 to -519)

| Code | Name | Description | Possible Causes | Resolution |
|------|------|-------------|-----------------|------------|
| -500 | `connection_failed` | Cannot connect to DB | Wrong host/port, DB down | Check connection string |
| -501 | `connection_lost` | Connection was lost | Network issue, timeout | Reconnect |
| -502 | `connection_timeout` | Connection timed out | Slow network, firewall | Increase timeout |
| -503 | `invalid_connection_string` | Bad connection string | Syntax error | Fix connection string |

### Pool Errors (-520 to -539)

| Code | Name | Description | Possible Causes | Resolution |
|------|------|-------------|-----------------|------------|
| -520 | `pool_exhausted` | Connection pool empty | Too many queries | Wait or increase pool |
| -521 | `pool_shutdown` | Pool is shutting down | Shutdown called | Don't request new connections |
| -522 | `pool_timeout` | Pool wait timed out | High contention | Increase pool size |

### Query Errors (-540 to -559)

| Code | Name | Description | Possible Causes | Resolution |
|------|------|-------------|-----------------|------------|
| -540 | `query_failed` | Query execution failed | DB error, constraints | Check query and data |
| -541 | `query_syntax_error` | Invalid SQL syntax | Typo, wrong dialect | Fix SQL syntax |
| -542 | `query_timeout` | Query timed out | Complex query, locks | Optimize query |

### Transaction Errors (-560 to -579)

| Code | Name | Description | Possible Causes | Resolution |
|------|------|-------------|-----------------|------------|
| -560 | `transaction_failed` | Transaction failed | Constraint violation | Check constraints |
| -561 | `transaction_rolled_back` | Transaction rolled back | Conflict, deadlock | Retry transaction |
| -562 | `transaction_timeout` | Transaction timed out | Long locks | Reduce transaction scope |

---

## network_system Error Codes (-600 to -699)

Errors related to network communication.

### Connection Errors (-600 to -619)

| Code | Name | Description | Possible Causes | Resolution |
|------|------|-------------|-----------------|------------|
| -600 | `connection_failed` | Cannot establish connection | Host unreachable, refused | Check host/port |
| -601 | `connection_refused` | Connection refused | Service not running | Start the service |
| -602 | `connection_timeout` | Connection timed out | Network latency, firewall | Increase timeout |
| -603 | `connection_closed` | Connection was closed | Remote closed, timeout | Reconnect |

### Session Errors (-620 to -639)

| Code | Name | Description | Possible Causes | Resolution |
|------|------|-------------|-----------------|------------|
| -620 | `session_not_found` | Session not found | Invalid session ID | Create new session |
| -621 | `session_expired` | Session has expired | Timeout, logout | Re-authenticate |
| -622 | `invalid_session` | Invalid session state | Corrupted, invalid token | Create new session |

### Send/Receive Errors (-640 to -659)

| Code | Name | Description | Possible Causes | Resolution |
|------|------|-------------|-----------------|------------|
| -640 | `send_failed` | Failed to send data | Connection lost, buffer full | Retry or reconnect |
| -641 | `receive_failed` | Failed to receive data | Connection lost, timeout | Retry or reconnect |
| -642 | `message_too_large` | Message exceeds size limit | Large payload | Split message or increase limit |

### Server Errors (-660 to -679)

| Code | Name | Description | Possible Causes | Resolution |
|------|------|-------------|-----------------|------------|
| -660 | `server_not_started` | Server not running | start() not called | Start the server |
| -661 | `server_already_running` | Server already started | Duplicate start() | Check state before start |
| -662 | `bind_failed` | Cannot bind to port | Port in use, permissions | Use different port |

---

## Usage Example

```cpp
#include <kcenon/common/error/error_codes.h>

using namespace kcenon::common::error;

void handle_error(int code) {
    // Get human-readable message
    auto message = get_error_message(code);

    // Get error category
    auto category = get_category_name(code);

    std::cerr << "[" << category << "] Error " << code
              << ": " << message << std::endl;
}

// Example usage
auto result = some_operation();
if (result.is_err()) {
    handle_error(result.error().code());
}
```

---

## Adding New Error Codes

When adding new error codes:

1. Choose the appropriate category namespace
2. Use the next available code within the range
3. Add a case to `get_error_message()` in error_codes.h
4. Update this documentation
5. Add compile-time validation if needed

See [ERROR_CODE_GUIDELINES.md](guides/ERROR_CODE_GUIDELINES.md) for detailed guidelines.

---

**Last Updated**: 2025-11-23
