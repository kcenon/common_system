# Phase 2.1 Detailed Implementation Guide
## common_system Interface Consolidation

**Target Audience**: Developers implementing Phase 2.1
**Prerequisites**: Git, CMake 3.16+, C++17 compiler, Doxygen
**Estimated Time**: 5 days (40 hours)

---

## Table of Contents
1. [Environment Setup](#environment-setup)
2. [Task 2.1.1: Interface Review](#task-211-interface-review)
3. [Task 2.1.2: Interface Enhancement](#task-212-interface-enhancement)
4. [Task 2.1.3: Compatibility Layer](#task-213-compatibility-layer)
5. [Task 2.1.4: Documentation](#task-214-documentation)
6. [Troubleshooting](#troubleshooting)
7. [Validation](#validation)

---

## Environment Setup

### Step 1: Create Feature Branch
```bash
cd /Users/dongcheolshin/Sources
git checkout -b feature/common-system-phase-2.1

# Verify you're on the correct branch
git branch --show-current
# Expected: feature/common-system-phase-2.1
```

### Step 2: Install Dependencies
```bash
# macOS
brew install cmake doxygen graphviz clang-format

# Ubuntu/Debian
sudo apt-get install cmake doxygen graphviz clang-format

# Verify installations
cmake --version       # Should be >= 3.16
doxygen --version     # Should be >= 1.9
```

### Step 3: Set Up Development Environment
```bash
# Create build directories
cd common_system
mkdir -p build docs/generated

# Configure CMake with tests enabled
cmake -B build \
    -DCMAKE_BUILD_TYPE=Debug \
    -DBUILD_COMMON_TESTS=ON \
    -DBUILD_COMMON_EXAMPLES=ON \
    -DENABLE_COVERAGE=ON \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

# Verify configuration
ls build/compile_commands.json
# Should exist
```

### Step 4: Create Working Directories
```bash
mkdir -p docs/analysis
mkdir -p docs/design
mkdir -p scripts/phase-2.1
mkdir -p tests/unit/interfaces
mkdir -p examples/common_system
```

---

## Task 2.1.1: Interface Review

**Goal**: Complete analysis of existing interfaces

### Step Sequence: IMonitor Interface Analysis

#### Step 1: Extract Current Interface Definition
```bash
cd /Users/dongcheolshin/Sources/common_system

# Read the current IMonitor interface
cat include/kcenon/common/interfaces/monitoring_interface.h > \
    docs/analysis/current_imonitor.h

# Count methods
grep "virtual.*=" include/kcenon/common/interfaces/monitoring_interface.h | wc -l
# Note the count
```

#### Step 2: Compare with Logger System's Interface
```bash
# Extract logger_system's monitoring interface
cd ../logger_system
cat include/kcenon/logger/core/monitoring/monitoring_interface.h > \
    ../common_system/docs/analysis/logger_monitoring_interface.h

# Use diff to find differences
cd ../common_system/docs/analysis
diff -u current_imonitor.h logger_monitoring_interface.h > imonitor_diff.txt

# Review differences
less imonitor_diff.txt
```

#### Step 3: Create Gap Analysis Document
```bash
# Create analysis template
cat > docs/analysis/IMonitor_gap_analysis.md << 'EOF'
# IMonitor Gap Analysis

## Date: [Current Date]
## Analyst: [Your Name]

## Current State (common_system::IMonitor)
- Method count: [COUNT]
- Key features:
  * record_metric(name, value)
  * get_metrics()
  * check_health()
  * reset()

## Logger System's monitoring_interface
- Method count: [COUNT]
- Key features:
  * get_monitoring_data()
  * is_healthy()
  * get_health_status()
  * set_metric(name, value)
  * get_metric(name)

## Identified Gaps

### Gap 1: Metric Retrieval
**Current**: get_metrics() returns full snapshot
**Logger**: get_metric(name) returns single metric
**Action**: Add single metric query method

### Gap 2: Health Status Granularity
**Current**: health_check_result with multiple status levels
**Logger**: Simple is_healthy() boolean
**Action**: Keep both, ensure compatibility

### Gap 3: [Add more gaps]

## Proposed Enhancements
1. Add `get_metric(const std::string& name)` for single metric queries
2. Add `subscribe()` method for real-time metric notifications
3. Add `aggregate_metric()` for metric aggregation
4. Add batch recording `record_metrics(vector<metric>)`

## Breaking Changes
- None (all additions)

## Backward Compatibility Strategy
- All new methods are additions
- Existing methods unchanged
- No API breakage expected

## Approval
- [ ] Architecture Lead
- [ ] Logger System Team Lead
- [ ] Monitoring System Team Lead

EOF

# Open for editing
${EDITOR:-vim} docs/analysis/IMonitor_gap_analysis.md
```

#### Step 4: Analyze Method Signatures
```bash
# Create signature comparison script
cat > scripts/phase-2.1/compare_signatures.py << 'EOF'
#!/usr/bin/env python3
"""Compare method signatures between interfaces."""

import re
import sys

def extract_methods(file_path):
    """Extract virtual method signatures from header file."""
    methods = []
    with open(file_path, 'r') as f:
        content = f.read()
        # Match virtual method declarations
        pattern = r'virtual\s+(\w+(?:<[^>]+>)?)\s+(\w+)\s*\([^)]*\)'
        matches = re.findall(pattern, content)
        for return_type, method_name in matches:
            methods.append(f"{return_type} {method_name}(...)")
    return methods

def main():
    if len(sys.argv) != 3:
        print("Usage: compare_signatures.py <file1> <file2>")
        sys.exit(1)

    file1, file2 = sys.argv[1], sys.argv[2]

    methods1 = set(extract_methods(file1))
    methods2 = set(extract_methods(file2))

    print("=== Only in first file ===")
    for m in sorted(methods1 - methods2):
        print(f"  {m}")

    print("\n=== Only in second file ===")
    for m in sorted(methods2 - methods1):
        print(f"  {m}")

    print("\n=== Common methods ===")
    for m in sorted(methods1 & methods2):
        print(f"  {m}")

if __name__ == '__main__':
    main()
EOF

chmod +x scripts/phase-2.1/compare_signatures.py

# Run comparison
./scripts/phase-2.1/compare_signatures.py \
    include/kcenon/common/interfaces/monitoring_interface.h \
    docs/analysis/logger_monitoring_interface.h > \
    docs/analysis/signature_comparison.txt

# Review results
cat docs/analysis/signature_comparison.txt
```

### Step Sequence: ILogger Interface Review

#### Step 1: Validate ILogger Completeness
```bash
# Check ILogger interface
cat include/kcenon/common/interfaces/logger_interface.h

# Verify all log levels are defined
grep "enum class log_level" include/kcenon/common/interfaces/logger_interface.h

# Expected output:
# trace = 0,
# debug = 1,
# info = 2,
# warning = 3,
# error = 4,
# critical = 5,
# off = 6
```

#### Step 2: Create ILogger Compatibility Report
```bash
cat > docs/analysis/ILogger_compatibility_report.md << 'EOF'
# ILogger Compatibility Report

## Current Interface Status
- [x] Basic logging methods (log with level and message)
- [x] Source location support (file, line, function)
- [x] Log level checking (is_enabled)
- [x] Level management (get_level, set_level)
- [x] Buffer flushing (flush)

## Logger System Usage Patterns
1. **Synchronous logging**: Direct log() calls
2. **Asynchronous logging**: Queued log entries
3. **Structured logging**: log_entry struct
4. **Writer pattern**: Multiple output destinations

## Compatibility Assessment
✅ ILogger interface covers all essential patterns
✅ No breaking changes needed
✅ Extension points available for future enhancements

## Recommendations
1. Add optional structured logging helpers
2. Consider adding log context (correlation IDs)
3. Add convenience macros (LOG_INFO, LOG_ERROR, etc.)

## Approval
- [ ] Logger System Team Lead
- [ ] Architecture Lead

EOF
```

### Step Sequence: Provider Interface Review

#### Step 1: Review IMonitorProvider and ILoggerProvider
```bash
# Create provider interface analysis
cat > docs/analysis/provider_interfaces_analysis.md << 'EOF'
# Provider Interface Analysis

## IMonitorProvider
```cpp
class IMonitorProvider {
    virtual std::shared_ptr<IMonitor> get_monitor() = 0;
    virtual std::shared_ptr<IMonitor> create_monitor(const std::string& name) = 0;
};
```

### Assessment
- [x] Factory pattern implemented
- [x] Named monitor creation supported
- [ ] Monitor lifecycle management unclear
- [ ] Monitor registry/discovery not addressed

### Recommendations
1. Add `list_monitors()` method
2. Add `remove_monitor(name)` method
3. Document monitor lifecycle expectations

## ILoggerProvider
```cpp
class ILoggerProvider {
    virtual std::shared_ptr<ILogger> get_logger() = 0;
    virtual std::shared_ptr<ILogger> create_logger(const std::string& name) = 0;
};
```

### Assessment
- [x] Factory pattern implemented
- [x] Named logger creation supported
- [x] Consistent with IMonitorProvider

### Recommendations
1. Mirror enhancements from IMonitorProvider
2. Add hierarchical logger support (parent/child)

## Approval
- [ ] Architecture Lead

EOF
```

#### Step 2: Generate Interface UML Diagrams
```bash
# Install plantuml (if not already)
# brew install plantuml  # macOS
# sudo apt-get install plantuml  # Ubuntu

# Create UML diagram source
cat > docs/design/interfaces_uml.puml << 'EOF'
@startuml Common System Interfaces

interface IMonitor {
    + record_metric(name: string, value: double): VoidResult
    + record_metric(name: string, value: double, tags: map): VoidResult
    + get_metrics(): Result<metrics_snapshot>
    + check_health(): Result<health_check_result>
    + reset(): VoidResult
}

interface ILogger {
    + log(level: log_level, message: string): VoidResult
    + log(level, message, file, line, function): VoidResult
    + log(entry: log_entry): VoidResult
    + is_enabled(level: log_level): bool
    + set_level(level: log_level): VoidResult
    + get_level(): log_level
    + flush(): VoidResult
}

interface IMonitorable {
    + get_monitoring_data(): Result<metrics_snapshot>
    + health_check(): Result<health_check_result>
    + get_component_name(): string
}

interface IMonitorProvider {
    + get_monitor(): shared_ptr<IMonitor>
    + create_monitor(name: string): shared_ptr<IMonitor>
}

interface ILoggerProvider {
    + get_logger(): shared_ptr<ILogger>
    + create_logger(name: string): shared_ptr<ILogger>
}

IMonitorProvider --> IMonitor : creates
ILoggerProvider --> ILogger : creates
IMonitorable ..> IMonitor : reports to

@enduml
EOF

# Generate diagram
plantuml docs/design/interfaces_uml.puml -tpng

# View diagram
open docs/design/interfaces_uml.png  # macOS
# xdg-open docs/design/interfaces_uml.png  # Linux
```

### Deliverable Checklist for Task 2.1.1
- [ ] `IMonitor_gap_analysis.md` completed and reviewed
- [ ] `ILogger_compatibility_report.md` completed
- [ ] `provider_interfaces_analysis.md` completed
- [ ] `signature_comparison.txt` generated
- [ ] UML diagrams generated
- [ ] All analysis approved by stakeholders

---

## Task 2.1.2: Interface Enhancement

**Goal**: Implement enhanced interfaces with new methods

### Task Sequence: Step Sequence: Add New Metric Types

#### Step 1: Create Enhanced Metric Structures
```bash
cd /Users/dongcheolshin/Sources/common_system

# Backup original file
cp include/kcenon/common/interfaces/monitoring_interface.h \
   include/kcenon/common/interfaces/monitoring_interface.h.backup

# Open file for editing
${EDITOR:-code} include/kcenon/common/interfaces/monitoring_interface.h
```

**Add the following code after the existing `metric_value` struct:**

```cpp
/**
 * @brief Aggregation types for metrics
 */
enum class aggregation_type {
    sum,        ///< Sum of all values
    average,    ///< Average of all values
    min,        ///< Minimum value
    max,        ///< Maximum value
    count,      ///< Count of values
    percentile_50,  ///< 50th percentile (median)
    percentile_95,  ///< 95th percentile
    percentile_99   ///< 99th percentile
};

/**
 * @brief Counter metric - monotonically increasing value
 */
struct counter_metric : metric_value {
    uint64_t count;

    counter_metric(const std::string& name, uint64_t cnt)
        : metric_value(name, static_cast<double>(cnt))
        , count(cnt) {}

    counter_metric& operator+=(uint64_t delta) {
        count += delta;
        value = static_cast<double>(count);
        timestamp = std::chrono::system_clock::now();
        return *this;
    }
};

/**
 * @brief Gauge metric - point-in-time value
 */
struct gauge_metric : metric_value {
    gauge_metric(const std::string& name, double val)
        : metric_value(name, val) {}

    void set(double new_value) {
        value = new_value;
        timestamp = std::chrono::system_clock::now();
    }
};

/**
 * @brief Histogram metric - distribution of values
 */
struct histogram_metric {
    std::string name;
    std::vector<double> buckets;           ///< Bucket boundaries
    std::vector<uint64_t> counts;          ///< Count per bucket
    double sum{0.0};                       ///< Sum of all observed values
    uint64_t count{0};                     ///< Total number of observations
    std::chrono::system_clock::time_point timestamp;

    histogram_metric(const std::string& n, const std::vector<double>& bucket_boundaries)
        : name(n)
        , buckets(bucket_boundaries)
        , counts(bucket_boundaries.size() + 1, 0)
        , timestamp(std::chrono::system_clock::now()) {}

    void observe(double value) {
        sum += value;
        count++;

        // Find appropriate bucket
        size_t bucket_index = 0;
        for (size_t i = 0; i < buckets.size(); ++i) {
            if (value <= buckets[i]) {
                bucket_index = i;
                break;
            }
        }
        if (bucket_index == 0 && value > buckets.back()) {
            bucket_index = buckets.size();
        }

        counts[bucket_index]++;
        timestamp = std::chrono::system_clock::now();
    }

    double get_percentile(double percentile) const {
        if (count == 0) return 0.0;

        uint64_t target_count = static_cast<uint64_t>(count * (percentile / 100.0));
        uint64_t accumulated = 0;

        for (size_t i = 0; i < counts.size(); ++i) {
            accumulated += counts[i];
            if (accumulated >= target_count) {
                return i < buckets.size() ? buckets[i] : std::numeric_limits<double>::max();
            }
        }

        return buckets.back();
    }

    double get_average() const {
        return count > 0 ? sum / count : 0.0;
    }
};

/**
 * @brief Dependency health information
 */
struct dependency_health {
    std::string name;                      ///< Dependency name
    health_status status;                  ///< Health status
    std::string endpoint;                  ///< Connection endpoint (optional)
    std::chrono::milliseconds response_time{0};  ///< Response time
    std::string error_message;             ///< Error if unhealthy

    dependency_health() = default;

    dependency_health(const std::string& n, health_status s)
        : name(n), status(s) {}
};
```

#### Step 2: Extend IMonitor Interface
**Add these methods to the `IMonitor` class:**

```cpp
    /**
     * @brief Batch record multiple metrics at once
     * @param metrics Vector of metrics to record
     * @return VoidResult indicating success or error
     */
    virtual VoidResult record_metrics(
        const std::vector<metric_value>& metrics) = 0;

    /**
     * @brief Query metrics within a time range
     * @param name Metric name pattern (supports wildcards)
     * @param start Start time (inclusive)
     * @param end End time (inclusive)
     * @return Result containing matching metrics or error
     */
    virtual Result<std::vector<metric_value>> query_metrics(
        const std::string& name,
        std::chrono::system_clock::time_point start,
        std::chrono::system_clock::time_point end) = 0;

    /**
     * @brief Get single metric by name
     * @param name Exact metric name
     * @return Result containing metric value or error
     */
    virtual Result<metric_value> get_metric(const std::string& name) = 0;

    /**
     * @brief Aggregate a metric over time
     * @param name Metric name
     * @param type Aggregation type
     * @return Result containing aggregated value or error
     */
    virtual Result<metric_value> aggregate_metric(
        const std::string& name,
        aggregation_type type) = 0;

    /**
     * @brief Subscribe to metric changes
     * @param metric_pattern Metric name pattern (supports wildcards: "cpu.*", "memory*")
     * @param callback Function called when matching metric changes
     * @return VoidResult indicating success or error
     */
    virtual VoidResult subscribe(
        const std::string& metric_pattern,
        std::function<void(const metric_value&)> callback) = 0;

    /**
     * @brief Unsubscribe from metric changes
     * @param metric_pattern Pattern to unsubscribe
     * @return VoidResult indicating success or error
     */
    virtual VoidResult unsubscribe(const std::string& metric_pattern) = 0;
```

#### Step 3: Update health_check_result Structure
**Modify the existing `health_check_result` struct:**

```cpp
struct health_check_result {
    health_status status = health_status::unknown;
    std::string message;
    std::chrono::system_clock::time_point timestamp;
    std::chrono::milliseconds check_duration{0};
    std::unordered_map<std::string, std::string> metadata;

    // NEW: Dependency health tracking
    std::vector<dependency_health> dependencies;

    health_check_result()
        : timestamp(std::chrono::system_clock::now()) {}

    bool is_healthy() const {
        if (status != health_status::healthy) {
            return false;
        }

        // Check dependencies
        for (const auto& dep : dependencies) {
            if (dep.status == health_status::unhealthy) {
                return false;
            }
        }

        return true;
    }

    bool is_operational() const {
        return status == health_status::healthy ||
               status == health_status::degraded;
    }

    // NEW: Get worst dependency status
    health_status get_worst_dependency_status() const {
        health_status worst = health_status::healthy;
        for (const auto& dep : dependencies) {
            if (dep.status > worst) {
                worst = dep.status;
            }
        }
        return worst;
    }
};
```

#### Step 4: Compile and Test
```bash
# Compile the updated interface
cd /Users/dongcheolshin/Sources/common_system
cmake --build build --target common_system

# Should succeed (header-only library)
echo $?
# Expected: 0
```

### Task Sequence: Step Sequence: Create Observer Interface

#### Step 1: Create New Observer Header
```bash
cat > include/kcenon/common/interfaces/monitor_observer.h << 'EOF'
// BSD 3-Clause License
// Copyright (c) 2025, kcenon
// See the LICENSE file in the project root for full license information.

/**
 * @file monitor_observer.h
 * @brief Observer pattern interfaces for monitoring system
 *
 * Provides observer/observable pattern for monitoring events,
 * enabling loose coupling between monitors and consumers.
 */

#pragma once

#include <chrono>
#include <functional>
#include <memory>
#include <string>
#include <any>
#include <vector>

namespace common {
namespace interfaces {

/**
 * @enum monitor_event_type
 * @brief Types of monitoring events
 */
enum class monitor_event_type {
    metric_recorded,        ///< New metric recorded
    threshold_exceeded,     ///< Metric exceeded threshold
    health_changed,         ///< Health status changed
    monitor_started,        ///< Monitor started
    monitor_stopped,        ///< Monitor stopped
    alert_triggered,        ///< Alert condition met
    custom                  ///< Custom event type
};

/**
 * @struct monitor_event
 * @brief Event data structure for monitoring events
 */
struct monitor_event {
    monitor_event_type event_type;
    std::string source;                     ///< Source component name
    std::any payload;                       ///< Event-specific data
    std::chrono::system_clock::time_point timestamp;
    std::string description;                ///< Human-readable description

    monitor_event()
        : event_type(monitor_event_type::custom)
        , timestamp(std::chrono::system_clock::now()) {}

    monitor_event(monitor_event_type type, const std::string& src)
        : event_type(type)
        , source(src)
        , timestamp(std::chrono::system_clock::now()) {}

    /**
     * @brief Get payload as specific type
     * @tparam T Expected payload type
     * @return Pointer to payload or nullptr if type mismatch
     */
    template<typename T>
    const T* get_payload_as() const {
        try {
            return std::any_cast<T>(&payload);
        } catch (const std::bad_any_cast&) {
            return nullptr;
        }
    }
};

/**
 * @interface IMonitorObserver
 * @brief Observer interface for monitoring events
 *
 * Implement this interface to receive notifications of monitoring events.
 * Observers are notified asynchronously and should not block.
 */
class IMonitorObserver {
public:
    virtual ~IMonitorObserver() = default;

    /**
     * @brief Called when a monitoring event occurs
     * @param event The monitoring event
     * @note This method should not block or throw exceptions
     */
    virtual void on_monitor_event(const monitor_event& event) = 0;

    /**
     * @brief Get observer name for identification
     * @return Observer name
     */
    virtual std::string get_observer_name() const = 0;
};

/**
 * @interface IMonitorObservable
 * @brief Observable interface for monitoring systems
 *
 * Implement this interface to allow observers to subscribe to monitoring events.
 */
class IMonitorObservable {
public:
    virtual ~IMonitorObservable() = default;

    /**
     * @brief Attach an observer to receive events
     * @param observer Observer to attach (weak_ptr to avoid circular refs)
     */
    virtual void attach_observer(std::weak_ptr<IMonitorObserver> observer) = 0;

    /**
     * @brief Detach an observer
     * @param observer Observer to detach (by raw pointer for lookup)
     */
    virtual void detach_observer(IMonitorObserver* observer) = 0;

    /**
     * @brief Detach all observers
     */
    virtual void detach_all_observers() = 0;

    /**
     * @brief Get number of attached observers
     * @return Observer count
     */
    virtual size_t observer_count() const = 0;
};

/**
 * @brief Convert event type to string
 */
inline std::string to_string(monitor_event_type type) {
    switch(type) {
        case monitor_event_type::metric_recorded: return "METRIC_RECORDED";
        case monitor_event_type::threshold_exceeded: return "THRESHOLD_EXCEEDED";
        case monitor_event_type::health_changed: return "HEALTH_CHANGED";
        case monitor_event_type::monitor_started: return "MONITOR_STARTED";
        case monitor_event_type::monitor_stopped: return "MONITOR_STOPPED";
        case monitor_event_type::alert_triggered: return "ALERT_TRIGGERED";
        case monitor_event_type::custom: return "CUSTOM";
        default: return "UNKNOWN";
    }
}

} // namespace interfaces
} // namespace common

EOF
```

#### Step 2: Create Example Observer Implementation
```bash
mkdir -p examples/common_system

cat > examples/common_system/observer_example.cpp << 'EOF'
/**
 * @file observer_example.cpp
 * @brief Example of using the monitor observer pattern
 */

#include <kcenon/common/interfaces/monitor_observer.h>
#include <kcenon/common/interfaces/monitoring_interface.h>
#include <iostream>
#include <memory>
#include <vector>
#include <mutex>

using namespace common::interfaces;

/**
 * @brief Example observer that logs events to console
 */
class console_observer : public IMonitorObserver {
public:
    void on_monitor_event(const monitor_event& event) override {
        std::cout << "[" << to_string(event.event_type) << "] "
                  << "Source: " << event.source << " - "
                  << event.description << std::endl;
    }

    std::string get_observer_name() const override {
        return "console_observer";
    }
};

/**
 * @brief Example observable monitor
 */
class observable_monitor : public IMonitor, public IMonitorObservable {
private:
    std::vector<std::weak_ptr<IMonitorObserver>> observers_;
    std::mutex observers_mutex_;
    std::unordered_map<std::string, metric_value> metrics_;

public:
    // IMonitorObservable implementation
    void attach_observer(std::weak_ptr<IMonitorObserver> observer) override {
        std::lock_guard lock(observers_mutex_);
        observers_.push_back(observer);
    }

    void detach_observer(IMonitorObserver* observer) override {
        std::lock_guard lock(observers_mutex_);
        observers_.erase(
            std::remove_if(observers_.begin(), observers_.end(),
                [observer](const std::weak_ptr<IMonitorObserver>& weak) {
                    auto shared = weak.lock();
                    return !shared || shared.get() == observer;
                }),
            observers_.end()
        );
    }

    void detach_all_observers() override {
        std::lock_guard lock(observers_mutex_);
        observers_.clear();
    }

    size_t observer_count() const override {
        std::lock_guard lock(observers_mutex_);
        return observers_.size();
    }

    // IMonitor implementation
    VoidResult record_metric(const std::string& name, double value) override {
        metrics_[name] = metric_value(name, value);

        // Notify observers
        notify_observers(monitor_event_type::metric_recorded, name,
            "Metric '" + name + "' recorded with value " + std::to_string(value));

        return VoidResult::success();
    }

    VoidResult record_metric(const std::string& name, double value,
                             const std::unordered_map<std::string, std::string>& tags) override {
        return record_metric(name, value);
    }

    Result<metrics_snapshot> get_metrics() override {
        metrics_snapshot snapshot;
        for (const auto& [name, metric] : metrics_) {
            snapshot.metrics.push_back(metric);
        }
        return Result<metrics_snapshot>::success(std::move(snapshot));
    }

    Result<health_check_result> check_health() override {
        health_check_result result;
        result.status = health_status::healthy;
        result.message = "Observable monitor operational";
        return Result<health_check_result>::success(std::move(result));
    }

    VoidResult reset() override {
        metrics_.clear();
        return VoidResult::success();
    }

private:
    void notify_observers(monitor_event_type type, const std::string& source,
                         const std::string& description) {
        monitor_event event(type, source);
        event.description = description;

        std::lock_guard lock(observers_mutex_);
        for (auto& weak_observer : observers_) {
            if (auto observer = weak_observer.lock()) {
                try {
                    observer->on_monitor_event(event);
                } catch (const std::exception& e) {
                    // Log error but don't propagate to avoid disrupting monitor
                    std::cerr << "Observer " << observer->get_observer_name()
                              << " threw exception: " << e.what() << std::endl;
                }
            }
        }
    }
};

int main() {
    // Create observable monitor
    auto monitor = std::make_shared<observable_monitor>();

    // Create and attach observer
    auto observer = std::make_shared<console_observer>();
    monitor->attach_observer(observer);

    std::cout << "Attached observers: " << monitor->observer_count() << std::endl;

    // Record metrics (will trigger observer notifications)
    monitor->record_metric("requests_total", 100.0);
    monitor->record_metric("cpu_usage", 45.5);
    monitor->record_metric("memory_usage", 78.2);

    // Detach observer
    monitor->detach_observer(observer.get());

    std::cout << "\nAfter detach, attached observers: "
              << monitor->observer_count() << std::endl;

    // This won't trigger notifications
    monitor->record_metric("silent_metric", 999.0);

    return 0;
}
EOF
```

#### Step 3: Update CMakeLists.txt for Examples
```bash
# Add to common_system/CMakeLists.txt
cat >> CMakeLists.txt << 'EOF'

# Examples
if(BUILD_COMMON_EXAMPLES)
    add_executable(observer_example examples/common_system/observer_example.cpp)
    target_link_libraries(observer_example PRIVATE common_system)
    target_compile_features(observer_example PRIVATE cxx_std_17)
endif()
EOF

# Reconfigure and build
cmake --build build --target observer_example

# Run example
./build/observer_example
```

**Expected Output:**
```
Attached observers: 1
[METRIC_RECORDED] Source: requests_total - Metric 'requests_total' recorded with value 100.000000
[METRIC_RECORDED] Source: cpu_usage - Metric 'cpu_usage' recorded with value 45.500000
[METRIC_RECORDED] Source: memory_usage - Metric 'memory_usage' recorded with value 78.200000

After detach, attached observers: 0
```

### Task Sequence: Step Sequence: Write Unit Tests

#### Step 1: Create Test Infrastructure
```bash
cd /Users/dongcheolshin/Sources/common_system

# Create test directory
mkdir -p tests/unit/interfaces

# Create test file for enhanced monitoring interface
cat > tests/unit/interfaces/monitoring_interface_enhanced_test.cpp << 'EOF'
#include <gtest/gtest.h>
#include <kcenon/common/interfaces/monitoring_interface.h>
#include <kcenon/common/interfaces/monitor_observer.h>
#include <memory>
#include <thread>
#include <chrono>

using namespace common::interfaces;

// Mock monitor implementation for testing
class mock_monitor : public IMonitor, public IMonitorObservable {
private:
    std::unordered_map<std::string, metric_value> metrics_;
    std::vector<std::weak_ptr<IMonitorObserver>> observers_;
    std::mutex mutex_;

public:
    VoidResult record_metric(const std::string& name, double value) override {
        std::lock_guard lock(mutex_);
        metrics_[name] = metric_value(name, value);
        notify_metric_recorded(name, value);
        return VoidResult::success();
    }

    VoidResult record_metric(const std::string& name, double value,
                            const std::unordered_map<std::string, std::string>& tags) override {
        return record_metric(name, value);
    }

    VoidResult record_metrics(const std::vector<metric_value>& metrics) override {
        std::lock_guard lock(mutex_);
        for (const auto& m : metrics) {
            metrics_[m.name] = m;
        }
        return VoidResult::success();
    }

    Result<std::vector<metric_value>> query_metrics(
        const std::string& name,
        std::chrono::system_clock::time_point start,
        std::chrono::system_clock::time_point end) override {

        std::vector<metric_value> results;
        std::lock_guard lock(mutex_);

        for (const auto& [metric_name, metric] : metrics_) {
            if (metric_name == name && metric.timestamp >= start && metric.timestamp <= end) {
                results.push_back(metric);
            }
        }

        return Result<std::vector<metric_value>>::success(std::move(results));
    }

    Result<metric_value> get_metric(const std::string& name) override {
        std::lock_guard lock(mutex_);
        auto it = metrics_.find(name);
        if (it != metrics_.end()) {
            return Result<metric_value>::success(it->second);
        }
        return Result<metric_value>::failure(1, "Metric not found");
    }

    Result<metric_value> aggregate_metric(const std::string& name,
                                         aggregation_type type) override {
        // Simplified aggregation for testing
        auto metric_result = get_metric(name);
        if (!metric_result) {
            return metric_result;
        }

        // For testing, just return the metric as-is
        return metric_result;
    }

    VoidResult subscribe(const std::string& metric_pattern,
                        std::function<void(const metric_value&)> callback) override {
        // Simplified subscription for testing
        return VoidResult::success();
    }

    VoidResult unsubscribe(const std::string& metric_pattern) override {
        return VoidResult::success();
    }

    Result<metrics_snapshot> get_metrics() override {
        std::lock_guard lock(mutex_);
        metrics_snapshot snapshot;
        for (const auto& [name, metric] : metrics_) {
            snapshot.metrics.push_back(metric);
        }
        return Result<metrics_snapshot>::success(std::move(snapshot));
    }

    Result<health_check_result> check_health() override {
        health_check_result result;
        result.status = health_status::healthy;
        result.message = "Mock monitor healthy";
        return Result<health_check_result>::success(std::move(result));
    }

    VoidResult reset() override {
        std::lock_guard lock(mutex_);
        metrics_.clear();
        return VoidResult::success();
    }

    // IMonitorObservable implementation
    void attach_observer(std::weak_ptr<IMonitorObserver> observer) override {
        std::lock_guard lock(mutex_);
        observers_.push_back(observer);
    }

    void detach_observer(IMonitorObserver* observer) override {
        std::lock_guard lock(mutex_);
        observers_.erase(
            std::remove_if(observers_.begin(), observers_.end(),
                [observer](const std::weak_ptr<IMonitorObserver>& weak) {
                    auto shared = weak.lock();
                    return !shared || shared.get() == observer;
                }),
            observers_.end()
        );
    }

    void detach_all_observers() override {
        std::lock_guard lock(mutex_);
        observers_.clear();
    }

    size_t observer_count() const override {
        std::lock_guard lock(mutex_);
        return observers_.size();
    }

private:
    void notify_metric_recorded(const std::string& name, double value) {
        monitor_event event(monitor_event_type::metric_recorded, name);
        event.description = "Metric recorded: " + name;
        event.payload = metric_value(name, value);

        for (auto& weak_obs : observers_) {
            if (auto obs = weak_obs.lock()) {
                obs->on_monitor_event(event);
            }
        }
    }
};

// Test fixture
class MonitoringInterfaceEnhancedTest : public ::testing::Test {
protected:
    std::shared_ptr<mock_monitor> monitor_;

    void SetUp() override {
        monitor_ = std::make_shared<mock_monitor>();
    }
};

TEST_F(MonitoringInterfaceEnhancedTest, RecordMetrics_BatchRecording) {
    std::vector<metric_value> metrics = {
        metric_value("metric1", 10.0),
        metric_value("metric2", 20.0),
        metric_value("metric3", 30.0)
    };

    auto result = monitor_->record_metrics(metrics);
    ASSERT_TRUE(result);

    auto snapshot = monitor_->get_metrics();
    ASSERT_TRUE(snapshot);
    EXPECT_EQ(snapshot.value().metrics.size(), 3);
}

TEST_F(MonitoringInterfaceEnhancedTest, GetMetric_SingleMetricRetrieval) {
    monitor_->record_metric("test_metric", 42.0);

    auto result = monitor_->get_metric("test_metric");
    ASSERT_TRUE(result);
    EXPECT_EQ(result.value().name, "test_metric");
    EXPECT_DOUBLE_EQ(result.value().value, 42.0);
}

TEST_F(MonitoringInterfaceEnhancedTest, GetMetric_NotFound) {
    auto result = monitor_->get_metric("nonexistent");
    EXPECT_FALSE(result);
}

TEST_F(MonitoringInterfaceEnhancedTest, QueryMetrics_TimeRange) {
    auto start = std::chrono::system_clock::now();

    monitor_->record_metric("test", 1.0);
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    monitor_->record_metric("test", 2.0);

    auto end = std::chrono::system_clock::now();

    auto result = monitor_->query_metrics("test", start, end);
    ASSERT_TRUE(result);
    EXPECT_GE(result.value().size(), 1);
}

TEST_F(MonitoringInterfaceEnhancedTest, HealthCheckResult_DependencyTracking) {
    health_check_result result;
    result.status = health_status::healthy;

    // Add healthy dependency
    dependency_health dep1("database", health_status::healthy);
    result.dependencies.push_back(dep1);

    EXPECT_TRUE(result.is_healthy());
    EXPECT_EQ(result.get_worst_dependency_status(), health_status::healthy);

    // Add unhealthy dependency
    dependency_health dep2("cache", health_status::unhealthy);
    result.dependencies.push_back(dep2);

    EXPECT_FALSE(result.is_healthy());
    EXPECT_EQ(result.get_worst_dependency_status(), health_status::unhealthy);
}

// Observer tests
class test_observer : public IMonitorObserver {
public:
    int event_count = 0;
    std::vector<monitor_event> received_events;

    void on_monitor_event(const monitor_event& event) override {
        event_count++;
        received_events.push_back(event);
    }

    std::string get_observer_name() const override {
        return "test_observer";
    }
};

TEST_F(MonitoringInterfaceEnhancedTest, Observer_ReceivesEvents) {
    auto observer = std::make_shared<test_observer>();
    monitor_->attach_observer(observer);

    EXPECT_EQ(monitor_->observer_count(), 1);

    monitor_->record_metric("test_metric", 100.0);

    EXPECT_EQ(observer->event_count, 1);
    EXPECT_EQ(observer->received_events[0].event_type,
              monitor_event_type::metric_recorded);
}

TEST_F(MonitoringInterfaceEnhancedTest, Observer_DetachWorks) {
    auto observer = std::make_shared<test_observer>();
    monitor_->attach_observer(observer);

    monitor_->record_metric("before_detach", 1.0);
    EXPECT_EQ(observer->event_count, 1);

    monitor_->detach_observer(observer.get());
    EXPECT_EQ(monitor_->observer_count(), 0);

    monitor_->record_metric("after_detach", 2.0);
    EXPECT_EQ(observer->event_count, 1);  // Still 1, no new events
}

// Metric type tests
TEST(MetricTypesTest, CounterMetric_Increment) {
    counter_metric counter("requests_total", 0);

    counter += 10;
    EXPECT_EQ(counter.count, 10);
    EXPECT_DOUBLE_EQ(counter.value, 10.0);

    counter += 5;
    EXPECT_EQ(counter.count, 15);
    EXPECT_DOUBLE_EQ(counter.value, 15.0);
}

TEST(MetricTypesTest, GaugeMetric_SetValue) {
    gauge_metric gauge("temperature", 20.0);

    EXPECT_DOUBLE_EQ(gauge.value, 20.0);

    gauge.set(25.5);
    EXPECT_DOUBLE_EQ(gauge.value, 25.5);
}

TEST(MetricTypesTest, HistogramMetric_Observe) {
    histogram_metric histogram("response_time", {0.1, 0.5, 1.0, 5.0});

    histogram.observe(0.05);  // Bucket 0
    histogram.observe(0.3);   // Bucket 0
    histogram.observe(0.8);   // Bucket 1
    histogram.observe(2.0);   // Bucket 2
    histogram.observe(10.0);  // Bucket 3 (overflow)

    EXPECT_EQ(histogram.count, 5);
    EXPECT_DOUBLE_EQ(histogram.sum, 13.15);
    EXPECT_NEAR(histogram.get_average(), 2.63, 0.01);
}

TEST(MetricTypesTest, HistogramMetric_Percentile) {
    histogram_metric histogram("latency", {10, 50, 100, 500, 1000});

    for (int i = 0; i < 100; ++i) {
        histogram.observe(i * 10.0);  // 0, 10, 20, ..., 990
    }

    double p50 = histogram.get_percentile(50);
    double p95 = histogram.get_percentile(95);

    EXPECT_GT(p50, 0);
    EXPECT_GT(p95, p50);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
EOF
```

#### Step 2: Update CMakeLists.txt for Tests
```bash
# Add Google Test dependency
cat >> CMakeLists.txt << 'EOF'

# Testing
if(BUILD_COMMON_TESTS)
    find_package(GTest REQUIRED)
    enable_testing()

    add_executable(monitoring_interface_enhanced_test
        tests/unit/interfaces/monitoring_interface_enhanced_test.cpp
    )

    target_link_libraries(monitoring_interface_enhanced_test
        PRIVATE
            common_system
            GTest::gtest
            GTest::gtest_main
    )

    target_compile_features(monitoring_interface_enhanced_test PRIVATE cxx_std_17)

    add_test(NAME MonitoringInterfaceEnhanced
             COMMAND monitoring_interface_enhanced_test)
endif()
EOF

# Reconfigure
cmake -B build -DBUILD_COMMON_TESTS=ON

# Build tests
cmake --build build --target monitoring_interface_enhanced_test

# Run tests
ctest --test-dir build --output-on-failure -R MonitoringInterfaceEnhanced
```

**Expected Output:**
```
Test project /Users/dongcheolshin/Sources/common_system/build
    Start 1: MonitoringInterfaceEnhanced
1/1 Test #1: MonitoringInterfaceEnhanced ......   Passed    0.05 sec

100% tests passed, 0 tests failed out of 1
```

### Task Sequence: Step Sequence: Generate Documentation

#### Step 1: Configure Doxygen
```bash
cd /Users/dongcheolshin/Sources/common_system

# Generate Doxygen configuration
doxygen -g docs/Doxyfile

# Customize configuration
cat >> docs/Doxyfile << 'EOF'

# Project settings
PROJECT_NAME           = "common_system"
PROJECT_NUMBER         = "2.0.0"
PROJECT_BRIEF          = "Common interfaces for kcenon ecosystem"
OUTPUT_DIRECTORY       = docs/generated

# Input settings
INPUT                  = include/kcenon/common
RECURSIVE              = YES
FILE_PATTERNS          = *.h *.hpp
EXCLUDE_PATTERNS       = */.backup/* */build/*

# Output settings
GENERATE_HTML          = YES
GENERATE_LATEX         = NO
HTML_OUTPUT            = html
HTML_COLORSTYLE_HUE    = 220
HTML_COLORSTYLE_SAT    = 100
HTML_COLORSTYLE_GAMMA  = 80

# Extraction settings
EXTRACT_ALL            = YES
EXTRACT_PRIVATE        = NO
EXTRACT_STATIC         = YES

# Diagram settings
HAVE_DOT               = YES
DOT_NUM_THREADS        = 4
UML_LOOK               = YES
TEMPLATE_RELATIONS     = YES
CALL_GRAPH             = YES
CALLER_GRAPH           = YES

# Warnings
WARNINGS               = YES
WARN_IF_UNDOCUMENTED   = YES
WARN_IF_DOC_ERROR      = YES
WARN_NO_PARAMDOC       = YES

EOF

# Generate documentation
doxygen docs/Doxyfile

# Open documentation
open docs/generated/html/index.html  # macOS
# xdg-open docs/generated/html/index.html  # Linux
```

#### Step 2: Verify Documentation Coverage
```bash
# Count documented vs undocumented entities
grep -r "\\brief" include/kcenon/common/interfaces/*.h | wc -l > /tmp/documented.txt
grep -r "class\|struct\|enum" include/kcenon/common/interfaces/*.h | wc -l > /tmp/total.txt

echo "Documentation coverage:"
echo "Documented: $(cat /tmp/documented.txt)"
echo "Total entities: $(cat /tmp/total.txt)"

# Calculate percentage
python3 << 'EOF'
with open('/tmp/documented.txt') as f:
    documented = int(f.read().strip())
with open('/tmp/total.txt') as f:
    total = int(f.read().strip())

coverage = (documented / total * 100) if total > 0 else 0
print(f"Coverage: {coverage:.1f}%")

if coverage < 100:
    print(f"⚠️  Target is 100%, currently at {coverage:.1f}%")
else:
    print("✅ Documentation coverage target met!")
EOF
```

### Deliverable Checklist for Task 2.1.2
- [ ] New metric types added (counter, gauge, histogram)
- [ ] IMonitor interface extended with new methods
- [ ] Observer pattern interfaces created
- [ ] Unit tests written and passing (100%)
- [ ] Example code working
- [ ] Documentation generated
- [ ] Code review completed
- [ ] All commits pushed to feature branch

---

## Validation

### Final Validation Checklist

```bash
# Run this script to validate all Phase 2.1 deliverables
cat > scripts/phase-2.1/validate_phase_2.1.sh << 'EOF'
#!/bin/bash

set -e

echo "=== Phase 2.1 Validation Script ==="
echo ""

cd /Users/dongcheolshin/Sources/common_system

# 1. Check feature branch
echo "1. Checking feature branch..."
current_branch=$(git branch --show-current)
if [ "$current_branch" = "feature/common-system-phase-2.1" ]; then
    echo "✅ On correct feature branch"
else
    echo "❌ Wrong branch: $current_branch"
    exit 1
fi

# 2. Check files exist
echo ""
echo "2. Checking required files..."
required_files=(
    "include/kcenon/common/interfaces/monitoring_interface.h"
    "include/kcenon/common/interfaces/monitor_observer.h"
    "docs/analysis/IMonitor_gap_analysis.md"
    "tests/unit/interfaces/monitoring_interface_enhanced_test.cpp"
    "examples/common_system/observer_example.cpp"
)

for file in "${required_files[@]}"; do
    if [ -f "$file" ]; then
        echo "✅ $file exists"
    else
        echo "❌ $file missing"
        exit 1
    fi
done

# 3. Build tests
echo ""
echo "3. Building tests..."
cmake -B build -DBUILD_COMMON_TESTS=ON -DBUILD_COMMON_EXAMPLES=ON > /dev/null 2>&1
cmake --build build --target monitoring_interface_enhanced_test > /dev/null 2>&1
echo "✅ Tests built successfully"

# 4. Run tests
echo ""
echo "4. Running tests..."
if ctest --test-dir build --output-on-failure -R MonitoringInterfaceEnhanced > /dev/null 2>&1; then
    echo "✅ All tests passing"
else
    echo "❌ Tests failing"
    exit 1
fi

# 5. Build examples
echo ""
echo "5. Building examples..."
cmake --build build --target observer_example > /dev/null 2>&1
echo "✅ Examples built successfully"

# 6. Check documentation
echo ""
echo "6. Checking documentation..."
if [ -d "docs/generated/html" ]; then
    echo "✅ Documentation generated"
else
    echo "❌ Documentation not generated"
    exit 1
fi

# 7. Code coverage
echo ""
echo "7. Checking code coverage..."
# (Coverage check would go here)
echo "⚠️  Code coverage check not implemented yet"

echo ""
echo "=== Phase 2.1 Validation Complete ==="
echo "✅ All checks passed!"
EOF

chmod +x scripts/phase-2.1/validate_phase_2.1.sh

# Run validation
./scripts/phase-2.1/validate_phase_2.1.sh
```

---

## Troubleshooting

### Common Issues

#### Issue 1: CMake Can't Find common_system
**Symptom**:
```
CMake Error: Could not find a package configuration file provided by "common_system"
```

**Solution**:
```bash
# Install common_system locally
cd /Users/dongcheolshin/Sources/common_system
cmake -B build -DCMAKE_INSTALL_PREFIX=~/.local
cmake --build build
cmake --install build

# Update CMAKE_PREFIX_PATH
export CMAKE_PREFIX_PATH=~/.local:$CMAKE_PREFIX_PATH
```

#### Issue 2: Google Test Not Found
**Symptom**:
```
CMake Error: Could not find a package configuration file provided by "GTest"
```

**Solution**:
```bash
# macOS
brew install googletest

# Ubuntu/Debian
sudo apt-get install libgtest-dev

# Build and install
cd /usr/src/googletest
sudo cmake -B build
sudo cmake --build build
sudo cmake --install build
```

#### Issue 3: Compilation Errors with std::any
**Symptom**:
```
error: 'any' is not a member of 'std'
```

**Solution**:
```bash
# Ensure C++17 is enabled
# Add to CMakeLists.txt:
target_compile_features(your_target PRIVATE cxx_std_17)
```

#### Issue 4: Doxygen Warnings
**Symptom**:
```
warning: documented symbol 'IMonitor' was not declared or defined
```

**Solution**:
```bash
# Check Doxygen INPUT path
grep "INPUT" docs/Doxyfile

# Should be: INPUT = include/kcenon/common

# Regenerate
doxygen docs/Doxyfile
```

---

## Next Steps

After completing Phase 2.1:

1. **Create Pull Request**
   ```bash
   git add .
   git commit -m "feat(common_system): Phase 2.1 - Interface consolidation complete

   - Enhanced IMonitor with new methods
   - Added observer pattern interfaces
   - Created comprehensive unit tests
   - Generated API documentation

   Closes #[ISSUE_NUMBER]"

   git push origin feature/common-system-phase-2.1

   # Create PR via GitHub CLI
   gh pr create --title "Phase 2.1: common_system Interface Consolidation" \
                --body "$(cat docs/analysis/phase_2.1_summary.md)"
   ```

2. **Notify Dependent Teams**
   - Email logger_system team
   - Email monitoring_system team
   - Share documentation links

3. **Proceed to Phase 2.2**
   - Begin logger_system refactoring
   - Use updated common_system interfaces

---

**Document Version**: 1.0
**Last Updated**: [Document Version]
**Next Review**: After Phase 2.1 completion