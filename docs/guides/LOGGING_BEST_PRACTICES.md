# Logging Best Practices

This guide covers best practices for logging in applications using the common_system runtime binding pattern.

## Overview

Effective logging is critical for:
- Debugging during development
- Monitoring in production
- Auditing and compliance
- Performance analysis

This guide helps you implement logging that is informative, performant, and maintainable.

## Using Logging Macros

### Always Prefer Macros Over Direct Calls

**Do:**
```cpp
#include <kcenon/common/logging/log_macros.h>

LOG_INFO("User logged in: " + user_id);
LOG_ERROR("Connection failed: " + error.message());
```

**Don't:**
```cpp
// Avoid direct logger access when macros are available
auto logger = GlobalLoggerRegistry::instance().get_default_logger();
logger->log(log_level::info, "User logged in: " + user_id);
```

**Why:** Macros automatically capture source location (file, line, function) and provide consistent behavior across the codebase.

### Use Appropriate Log Levels

| Level | Use Case | Examples |
|-------|----------|----------|
| `TRACE` | Detailed flow tracing | Function entry/exit, loop iterations |
| `DEBUG` | Development debugging | Variable values, state changes |
| `INFO` | Normal operations | Startup, shutdown, major events |
| `WARNING` | Potential issues | Deprecated usage, near-limit resources |
| `ERROR` | Recoverable failures | Failed operations that can continue |
| `CRITICAL` | Unrecoverable failures | System cannot continue |

**Examples:**

```cpp
LOG_TRACE("Entering function process_request");
LOG_DEBUG("Request payload size: " + std::to_string(payload.size()));
LOG_INFO("Server started on port " + std::to_string(port));
LOG_WARNING("Memory usage at 80%, consider scaling");
LOG_ERROR("Database query failed: " + db_error.message());
LOG_CRITICAL("Out of memory, shutting down");
```

### Use Conditional Logging for Expensive Messages

When message construction is expensive, use `LOG_IF` to avoid construction when logging is disabled:

**Do:**
```cpp
// Message only constructed if debug is enabled
LOG_IF(log_level::debug,
       "Request details: " + serialize_request(request));
```

**Don't:**
```cpp
// serialize_request is ALWAYS called, even when logging is disabled
LOG_DEBUG("Request details: " + serialize_request(request));
```

**Also Check Level Before Complex Operations:**
```cpp
if (LOG_IS_ENABLED(log_level::trace)) {
    auto analysis = perform_expensive_analysis(data);
    LOG_TRACE("Analysis result: " + analysis.to_string());
}
```

## Named Loggers for Subsystems

### Use Separate Loggers for Different Concerns

```cpp
// Registration at startup
SystemBootstrapper bootstrapper;
bootstrapper
    .with_default_logger(create_console_logger)
    .with_logger("network", [] {
        auto logger = std::make_shared<FileLogger>("network.log");
        logger->set_level(log_level::debug);
        return logger;
    })
    .with_logger("database", [] {
        auto logger = std::make_shared<RotatingFileLogger>("database.log");
        logger->set_level(log_level::info);
        return logger;
    })
    .with_logger("security", [] {
        auto logger = std::make_shared<FileLogger>("security.log");
        logger->set_level(log_level::warning);  // Only warnings and above
        return logger;
    })
    .with_logger("audit", [] {
        // Audit logs might need special handling
        return std::make_shared<AuditLogger>("audit.log");
    });
```

### Use Named Logger Macros

```cpp
// In network code
LOG_DEBUG_TO("network", "Received packet from " + client_addr);
LOG_INFO_TO("network", "Connection established");

// In database code
LOG_INFO_TO("database", "Query executed in " + std::to_string(ms) + "ms");

// In security code
LOG_WARNING_TO("security", "Failed login attempt for user: " + username);

// Audit events
LOG_INFO_TO("audit", "User " + user_id + " accessed resource " + resource_id);
```

## Message Content

### Be Specific and Actionable

**Do:**
```cpp
LOG_ERROR("Failed to connect to database at " + db_host + ":"
          + std::to_string(db_port) + ": " + error.message());
```

**Don't:**
```cpp
LOG_ERROR("Database error");
```

### Include Relevant Context

```cpp
LOG_INFO("Processing order: id=" + order_id
         + ", user=" + user_id
         + ", items=" + std::to_string(item_count));

LOG_ERROR("Payment failed: order_id=" + order_id
          + ", amount=" + std::to_string(amount)
          + ", gateway=" + gateway_name
          + ", error=" + error_code);
```

### Avoid Sensitive Information

**Never log:**
- Passwords or tokens
- Credit card numbers
- Personal identifiable information (PII) in plain text
- API keys or secrets

```cpp
// BAD - Never do this
LOG_DEBUG("User login: username=" + username + ", password=" + password);

// GOOD - Mask or omit sensitive data
LOG_DEBUG("User login: username=" + username);
LOG_INFO("User " + user_id + " authenticated successfully");

// GOOD - Use IDs instead of PII
LOG_INFO("Processing order for customer_id=" + customer_id);
```

### Use Consistent Message Formats

Establish a convention for your project:

```cpp
// Example format: [COMPONENT] Action: details
LOG_INFO("[OrderService] Processing order: id=" + order_id);
LOG_INFO("[PaymentService] Payment initiated: order_id=" + order_id);
LOG_INFO("[InventoryService] Stock reserved: sku=" + sku + ", qty=" + qty);
```

## Performance Considerations

### Log Level Check is O(1)

The level check is very fast, so conditional logging is efficient:

```cpp
// This is efficient - level check is O(1)
if (LOG_IS_ENABLED(log_level::debug)) {
    LOG_DEBUG("Message");
}
```

### Avoid String Concatenation in Hot Paths

**In hot paths, avoid:**
```cpp
for (const auto& item : items) {
    LOG_TRACE("Processing: " + item.to_string());  // String allocation each iteration
}
```

**Better:**
```cpp
if (LOG_IS_ENABLED(log_level::trace)) {
    for (const auto& item : items) {
        LOG_TRACE("Processing: " + item.to_string());
    }
}

// Or log at start/end only
LOG_TRACE("Processing " + std::to_string(items.size()) + " items");
```

### Use Compile-Time Level Control for Release Builds

Define `KCENON_MIN_LOG_LEVEL` to strip low-level logging from release builds:

```cpp
// In release build configuration
#define KCENON_MIN_LOG_LEVEL 2  // Disable TRACE and DEBUG
#include <kcenon/common/logging/log_macros.h>

// These become no-ops at compile time
LOG_TRACE("This is completely removed from binary");
LOG_DEBUG("This is completely removed from binary");
LOG_INFO("This still works");
```

### Flush Strategy

**Default Behavior:**
- Error and above: typically auto-flush
- Info and below: may be buffered

**Explicit Flush When Needed:**
```cpp
LOG_INFO("Critical state change");
LOG_FLUSH();  // Ensure it's written before crash
```

**Avoid Excessive Flushing:**
```cpp
// BAD - Performance killer
for (int i = 0; i < 10000; i++) {
    LOG_DEBUG("Iteration " + std::to_string(i));
    LOG_FLUSH();  // Don't do this in loops
}

// GOOD - Let buffering work
for (int i = 0; i < 10000; i++) {
    LOG_DEBUG("Iteration " + std::to_string(i));
}
LOG_FLUSH();  // Flush once at end if needed
```

## Error Handling

### Log Before and After Critical Operations

```cpp
LOG_INFO("Starting database migration");
auto result = run_migration();
if (result.is_ok()) {
    LOG_INFO("Database migration completed successfully");
} else {
    LOG_ERROR("Database migration failed: " + result.error().message);
}
```

### Log Stack Context on Errors

```cpp
void process_order(const Order& order) {
    LOG_DEBUG("Processing order: " + order.id());

    try {
        validate_order(order);
        reserve_inventory(order);
        process_payment(order);
        LOG_INFO("Order processed: " + order.id());
    } catch (const ValidationException& e) {
        LOG_ERROR("Order validation failed: order_id=" + order.id()
                  + ", error=" + e.what());
        throw;
    } catch (const InventoryException& e) {
        LOG_ERROR("Inventory reservation failed: order_id=" + order.id()
                  + ", error=" + e.what());
        throw;
    } catch (const PaymentException& e) {
        LOG_ERROR("Payment processing failed: order_id=" + order.id()
                  + ", error=" + e.what());
        throw;
    }
}
```

### Don't Log and Throw the Same Information

**Avoid:**
```cpp
void do_something() {
    LOG_ERROR("Operation failed: " + reason);
    throw std::runtime_error("Operation failed: " + reason);  // Duplicated
}
```

**Better:**
```cpp
void do_something() {
    throw std::runtime_error("Operation failed: " + reason);
}

// Log at catch point
try {
    do_something();
} catch (const std::exception& e) {
    LOG_ERROR("do_something failed: " + std::string(e.what()));
    // Handle or rethrow
}
```

## Testing

### Use NullLogger for Silent Tests

```cpp
class MyTest : public ::testing::Test {
protected:
    void SetUp() override {
        GlobalLoggerRegistry::instance().set_default_logger(
            GlobalLoggerRegistry::null_logger()
        );
    }

    void TearDown() override {
        GlobalLoggerRegistry::instance().clear();
    }
};
```

### Create Mock Logger for Verification

```cpp
class MockLogger : public ILogger {
public:
    struct LogEntry {
        log_level level;
        std::string message;
    };
    std::vector<LogEntry> entries;

    VoidResult log(log_level level, const std::string& message) override {
        entries.push_back({level, message});
        return VoidResult::ok({});
    }
    // ... other methods
};

TEST_F(MyTest, LogsErrorOnFailure) {
    auto mock = std::make_shared<MockLogger>();
    GlobalLoggerRegistry::instance().set_default_logger(mock);

    process_invalid_input();

    ASSERT_EQ(mock->entries.size(), 1);
    EXPECT_EQ(mock->entries[0].level, log_level::error);
    EXPECT_THAT(mock->entries[0].message, HasSubstr("invalid input"));
}
```

## Initialization

### Initialize Before Any Logging

```cpp
int main() {
    // First thing: set up logging
    SystemBootstrapper bootstrapper;
    bootstrapper
        .with_default_logger(create_logger)
        .initialize();

    // Now it's safe to log
    LOG_INFO("Application starting");

    // Rest of application...
}
```

### Handle Initialization Failures

```cpp
auto result = bootstrapper.initialize();
if (result.is_err()) {
    // Can't use LOG_* here - logger might not be registered
    std::cerr << "Failed to initialize logging: "
              << result.error().message << std::endl;
    return 1;
}

// Now safe to use logging
LOG_INFO("Logging system initialized");
```

## Summary Checklist

### Do:
- [ ] Use logging macros (`LOG_*`)
- [ ] Choose appropriate log levels
- [ ] Use conditional logging for expensive messages
- [ ] Use named loggers for subsystems
- [ ] Include relevant context in messages
- [ ] Log before and after critical operations
- [ ] Initialize logging before any other code
- [ ] Use NullLogger in tests

### Don't:
- [ ] Log sensitive information
- [ ] Log in tight loops without level checks
- [ ] Flush after every log statement
- [ ] Log and throw the same information
- [ ] Use direct logger calls when macros are available

## Related Documentation

- [Runtime Binding Architecture](../architecture/RUNTIME_BINDING.md)
- [Migration Guide](MIGRATION_RUNTIME_BINDING.md)
- [Troubleshooting](TROUBLESHOOTING_LOGGING.md)
