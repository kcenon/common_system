# IExecutor API Migration Guide

**Date**: 2025-11-09
**Status**: Deprecation Notice
**Target Removal**: Next major version (2.0.0)

---

## Overview

The function-based executor API (`submit()` and `submit_delayed()`) has been deprecated in favor of the job-based API (`execute()` and `execute_delayed()`). This document provides guidance on migrating your code.

---

## Why Migrate?

The job-based API provides several advantages:

1. **Better Error Handling**: Uses `Result<T>` for explicit error propagation
2. **Testability**: Jobs can be easily mocked and tested
3. **Observability**: Jobs have names and priorities for debugging
4. **Type Safety**: Compile-time type checking via interfaces
5. **Consistency**: Aligns with modern C++ best practices

---

## Migration Steps

### Step 1: Convert Function to Job

**Before (Deprecated)**:
```cpp
#include <kcenon/common/interfaces/executor_interface.h>

std::shared_ptr<IExecutor> executor = get_executor();

// Function-based API (deprecated)
auto future = executor->submit([]() {
    // Task logic here
    std::cout << "Task executed" << std::endl;
});
```

**After (Preferred)**:
```cpp
#include <kcenon/common/interfaces/executor_interface.h>

// Define a job class
class MyJob : public kcenon::common::interfaces::IJob {
public:
    VoidResult execute() override {
        // Task logic here
        std::cout << "Task executed" << std::endl;
        return VoidResult{};  // Success
    }

    std::string get_name() const override {
        return "MyJob";
    }
};

std::shared_ptr<IExecutor> executor = get_executor();

// Job-based API (preferred)
auto result = executor->execute(std::make_unique<MyJob>());
if (result.is_ok()) {
    auto future = result.value();
    // Use future...
} else {
    // Handle error
    std::cerr << "Execution failed: " << result.error().message << std::endl;
}
```

### Step 2: Handle Delayed Execution

**Before (Deprecated)**:
```cpp
auto future = executor->submit_delayed([]() {
    std::cout << "Delayed task" << std::endl;
}, std::chrono::seconds(5));
```

**After (Preferred)**:
```cpp
class DelayedJob : public kcenon::common::interfaces::IJob {
public:
    VoidResult execute() override {
        std::cout << "Delayed task" << std::endl;
        return VoidResult{};
    }

    std::string get_name() const override {
        return "DelayedJob";
    }
};

auto result = executor->execute_delayed(
    std::make_unique<DelayedJob>(),
    std::chrono::seconds(5)
);
```

### Step 3: Add Error Handling

One of the key benefits of the new API is explicit error handling:

```cpp
auto result = executor->execute(std::make_unique<MyJob>());

if (!result.is_ok()) {
    // Handle execution error
    switch (result.error().code) {
        case ExecutorError::QUEUE_FULL:
            // Retry logic
            break;
        case ExecutorError::SHUTDOWN:
            // Graceful degradation
            break;
        default:
            // Log and report
            break;
    }
    return;
}

// Use future
auto future = result.value();
```

---

## Advanced Patterns

### Pattern 1: Job with Priority

```cpp
class HighPriorityJob : public IJob {
public:
    VoidResult execute() override {
        // Critical task logic
        return VoidResult{};
    }

    std::string get_name() const override {
        return "HighPriorityJob";
    }

    int get_priority() const override {
        return 10;  // Higher priority
    }
};
```

### Pattern 2: Job with Parameters

```cpp
class ParameterizedJob : public IJob {
public:
    explicit ParameterizedJob(int value, std::string name)
        : value_(value), name_(std::move(name)) {}

    VoidResult execute() override {
        std::cout << "Processing: " << name_ << " with value " << value_ << std::endl;
        return VoidResult{};
    }

    std::string get_name() const override {
        return "ParameterizedJob[" + name_ + "]";
    }

private:
    int value_;
    std::string name_;
};

// Usage
executor->execute(std::make_unique<ParameterizedJob>(42, "test"));
```

### Pattern 3: Lambda Wrapper (Transition Helper)

If you need a quick transition, you can create a lambda wrapper:

```cpp
class LambdaJob : public IJob {
public:
    explicit LambdaJob(std::function<void()> func, std::string name = "LambdaJob")
        : func_(std::move(func)), name_(std::move(name)) {}

    VoidResult execute() override {
        func_();
        return VoidResult{};
    }

    std::string get_name() const override {
        return name_;
    }

private:
    std::function<void()> func_;
    std::string name_;
};

// Helper function
auto execute_lambda(IExecutor& executor, std::function<void()> func, std::string name = "lambda") {
    return executor.execute(std::make_unique<LambdaJob>(std::move(func), std::move(name)));
}

// Usage (transition helper)
execute_lambda(*executor, []() {
    std::cout << "Quick lambda" << std::endl;
});
```

---

## Timeline

| Version | Status | Description |
|---------|--------|-------------|
| 1.x (Current) | **Deprecated** | Function-based API deprecated, job-based API preferred |
| 2.0.0 (Future) | **Removed** | Function-based API will be removed completely |

**Deprecation Period**: Minimum 6 months from deprecation notice (2025-11-09)

**Recommended Action**: Start migrating to job-based API now to avoid breaking changes in version 2.0.0.

---

## Compiler Warnings

After upgrading, you may see warnings like:

```
warning: 'submit' is deprecated: Use execute() with IJob instead. Will be removed in next major version
```

These warnings are intentional to help you identify code that needs migration.

To suppress warnings temporarily (not recommended for long-term):
```cpp
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
auto future = executor->submit([]() { /* ... */ });
#pragma GCC diagnostic pop
```

---

## Testing

The job-based API is much easier to test:

```cpp
// Mock job for testing
class MockJob : public IJob {
public:
    MOCK_METHOD(VoidResult, execute, (), (override));
    MOCK_METHOD(std::string, get_name, (), (const, override));
};

// Test
TEST(ExecutorTest, JobExecution) {
    auto mock_job = std::make_unique<MockJob>();
    EXPECT_CALL(*mock_job, execute())
        .WillOnce(Return(VoidResult{}));

    auto result = executor->execute(std::move(mock_job));
    ASSERT_TRUE(result.is_ok());
}
```

---

## FAQ

**Q: Can I still use lambdas?**
A: Yes, you can wrap lambdas in a `LambdaJob` class (see Pattern 3 above).

**Q: Will my code break immediately?**
A: No, the deprecated API will continue to work until version 2.0.0 (minimum 6 months).

**Q: What if I don't migrate?**
A: Your code will break when upgrading to version 2.0.0.

**Q: Is there an automatic migration tool?**
A: Not currently, but the patterns above should cover most use cases.

---

## Support

If you encounter issues during migration:
1. Review the examples above
2. Check the API documentation
3. Create an issue on GitHub with your specific use case

---

**Last Updated**: 2025-11-09
