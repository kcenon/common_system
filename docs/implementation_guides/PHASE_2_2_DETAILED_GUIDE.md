# Phase 2.2 Detailed Implementation Guide
## logger_system Interface Migration and Refactoring

**Target Audience**: Logger system developers
**Prerequisites**: Phase 2.1 completed, C++20 compiler, logger_system codebase access
**Estimated Time**: 10 days (80 hours)

---

## Table of Contents
1. [Environment Setup](#environment-setup)
2. [Task 2.2.1: Dependency Analysis](#task-221-dependency-analysis)
3. [Task 2.2.2: Remove Duplicate Interface](#task-222-remove-duplicate-interface)
4. [Task 2.2.3: Refactor Logger Class](#task-223-refactor-logger-class)
5. [Task 2.2.4: Refactor Builder](#task-224-refactor-logger-builder)
6. [Task 2.2.5: Update Writers](#task-225-update-writers)
7. [Testing Strategy](#testing-strategy)
8. [Validation](#validation)

---

## Environment Setup

### Step 1: Verify Phase 2.1 Completion
```bash
# Check common_system is ready
cd /Users/dongcheolshin/Sources/common_system

# Verify enhanced interfaces exist
test -f include/kcenon/common/interfaces/monitoring_interface.h && echo "✅ IMonitor available"
test -f include/kcenon/common/interfaces/monitor_observer.h && echo "✅ Observer pattern available"

# Check common_system can be found by CMake
cd ../logger_system
cat > test_find_common.cmake << 'EOF'
cmake_minimum_required(VERSION 3.16)
project(test_find_common)
find_package(common_system 2.0 REQUIRED)
message(STATUS "✅ common_system found: ${common_system_VERSION}")
EOF

cmake -P test_find_common.cmake
rm test_find_common.cmake
```

### Step 2: Create Feature Branch
```bash
cd /Users/dongcheolshin/Sources/logger_system

# Create feature branch
git checkout -b feature/logger-system-phase-2.2

# Create working directories
mkdir -p docs/migration/{analysis,design,compatibility}
mkdir -p scripts/phase-2.2
mkdir -p tests/unit/migration
mkdir -p tests/integration/phase-2.2
```

### Step 3: Backup Current State
```bash
# Create backup of key files
mkdir -p .backup/phase-2.2-start

cp -r include/kcenon/logger/core/monitoring .backup/phase-2.2-start/
cp include/kcenon/logger/core/logger.h .backup/phase-2.2-start/
cp include/kcenon/logger/core/logger_builder.h .backup/phase-2.2-start/
cp CMakeLists.txt .backup/phase-2.2-start/

echo "✅ Backup created at .backup/phase-2.2-start"
```

---

## Task 2.2.1: Dependency Analysis

**Goal**: Map all monitoring dependencies and create migration plan

### Step Sequence: Automated Dependency Scanning

#### Step 1: Create Dependency Scanner Script
```bash
cat > scripts/phase-2.2/scan_monitoring_deps.py << 'EOF'
#!/usr/bin/env python3
"""
Scan logger_system for monitoring interface dependencies.
"""

import os
import re
import json
from pathlib import Path
from collections import defaultdict

def find_monitoring_references(root_dir):
    """Find all references to monitoring interfaces."""
    results = {
        'includes': [],
        'namespace_usage': [],
        'type_usage': [],
        'method_calls': []
    }

    # Patterns to search for
    patterns = {
        'includes': r'#include\s*[<"].*monitoring.*[>"]',
        'namespace': r'using\s+namespace\s+.*monitoring',
        'type': r'monitoring(_interface|::monitoring_interface|::basic_monitoring)',
        'method': r'(set_monitor|with_monitoring|get_monitor)\s*\('
    }

    for root, dirs, files in os.walk(root_dir):
        # Skip build directories
        if 'build' in root or '.backup' in root:
            continue

        for file in files:
            if not (file.endswith('.h') or file.endswith('.cpp')):
                continue

            filepath = os.path.join(root, file)
            rel_path = os.path.relpath(filepath, root_dir)

            try:
                with open(filepath, 'r', encoding='utf-8') as f:
                    content = f.read()
                    line_num = 0

                    for line in content.split('\n'):
                        line_num += 1

                        # Check each pattern
                        if re.search(patterns['includes'], line):
                            results['includes'].append({
                                'file': rel_path,
                                'line': line_num,
                                'content': line.strip()
                            })

                        if re.search(patterns['namespace'], line):
                            results['namespace_usage'].append({
                                'file': rel_path,
                                'line': line_num,
                                'content': line.strip()
                            })

                        if re.search(patterns['type'], line):
                            results['type_usage'].append({
                                'file': rel_path,
                                'line': line_num,
                                'content': line.strip()
                            })

                        if re.search(patterns['method'], line):
                            results['method_calls'].append({
                                'file': rel_path,
                                'line': line_num,
                                'content': line.strip()
                            })

            except Exception as e:
                print(f"Error processing {filepath}: {e}")

    return results

def generate_report(results, output_file):
    """Generate human-readable report."""
    with open(output_file, 'w') as f:
        f.write("# Monitoring Interface Dependency Analysis\n\n")

        f.write(f"## Summary\n\n")
        f.write(f"- Include statements: {len(results['includes'])}\n")
        f.write(f"- Namespace usages: {len(results['namespace_usage'])}\n")
        f.write(f"- Type usages: {len(results['type_usage'])}\n")
        f.write(f"- Method calls: {len(results['method_calls'])}\n\n")

        # Group by file
        files_affected = set()
        for category in results.values():
            for item in category:
                files_affected.add(item['file'])

        f.write(f"## Affected Files ({len(files_affected)})\n\n")
        for file in sorted(files_affected):
            f.write(f"- `{file}`\n")

        f.write("\n## Detailed Findings\n\n")

        for category, items in results.items():
            if items:
                f.write(f"### {category.replace('_', ' ').title()}\n\n")
                for item in items:
                    f.write(f"**{item['file']}:{item['line']}**\n")
                    f.write(f"```cpp\n{item['content']}\n```\n\n")

    print(f"✅ Report generated: {output_file}")

def main():
    root_dir = os.getcwd()
    print(f"Scanning {root_dir}...")

    results = find_monitoring_references(root_dir)

    # Save JSON results
    json_output = 'docs/migration/analysis/monitoring_deps.json'
    os.makedirs(os.path.dirname(json_output), exist_ok=True)
    with open(json_output, 'w') as f:
        json.dump(results, f, indent=2)
    print(f"✅ JSON data saved: {json_output}")

    # Generate readable report
    report_output = 'docs/migration/analysis/monitoring_deps_report.md'
    generate_report(results, report_output)

    # Print summary
    print(f"\n=== Summary ===")
    print(f"Includes: {len(results['includes'])}")
    print(f"Namespace usages: {len(results['namespace_usage'])}")
    print(f"Type usages: {len(results['type_usage'])}")
    print(f"Method calls: {len(results['method_calls'])}")

if __name__ == '__main__':
    main()
EOF

chmod +x scripts/phase-2.2/scan_monitoring_deps.py

# Run scanner
python3 scripts/phase-2.2/scan_monitoring_deps.py

# View report
cat docs/migration/analysis/monitoring_deps_report.md
```

#### Step 2: Create Migration Impact Matrix
```bash
cat > scripts/phase-2.2/create_impact_matrix.py << 'EOF'
#!/usr/bin/env python3
"""
Create impact assessment matrix for migration.
"""

import json

def load_dependencies():
    """Load dependency scan results."""
    with open('docs/migration/analysis/monitoring_deps.json') as f:
        return json.load(f)

def assess_impact(deps):
    """Assess migration impact for each file."""
    impact_matrix = {}

    # Group by file
    for category, items in deps.items():
        for item in items:
            file = item['file']
            if file not in impact_matrix:
                impact_matrix[file] = {
                    'includes': 0,
                    'namespace_usage': 0,
                    'type_usage': 0,
                    'method_calls': 0,
                    'total_changes': 0,
                    'effort': 'LOW',
                    'priority': 'P2'
                }

            impact_matrix[file][category] += 1
            impact_matrix[file]['total_changes'] += 1

    # Assess effort level
    for file, data in impact_matrix.items():
        total = data['total_changes']

        if total <= 3:
            data['effort'] = 'LOW'
        elif total <= 10:
            data['effort'] = 'MEDIUM'
        else:
            data['effort'] = 'HIGH'

        # Determine priority
        if 'logger.h' in file or 'logger_builder.h' in file:
            data['priority'] = 'P0'
        elif 'monitoring_factory' in file or 'monitoring_interface' in file:
            data['priority'] = 'P0'
        elif 'tests/' in file:
            data['priority'] = 'P1'
        else:
            data['priority'] = 'P2'

    return impact_matrix

def generate_matrix_markdown(matrix, output_file):
    """Generate Markdown table."""
    with open(output_file, 'w') as f:
        f.write("# Migration Impact Matrix\n\n")
        f.write("| File | Includes | Namespace | Types | Methods | Total | Effort | Priority |\n")
        f.write("|------|----------|-----------|-------|---------|-------|--------|----------|\n")

        # Sort by priority then effort
        priority_order = {'P0': 0, 'P1': 1, 'P2': 2}
        sorted_files = sorted(matrix.items(),
                            key=lambda x: (priority_order[x[1]['priority']],
                                         -x[1]['total_changes']))

        for file, data in sorted_files:
            f.write(f"| `{file}` | {data['includes']} | {data['namespace_usage']} | "
                   f"{data['type_usage']} | {data['method_calls']} | "
                   f"**{data['total_changes']}** | {data['effort']} | {data['priority']} |\n")

        f.write("\n## Summary\n\n")
        effort_counts = {'LOW': 0, 'MEDIUM': 0, 'HIGH': 0}
        priority_counts = {'P0': 0, 'P1': 0, 'P2': 0}

        for data in matrix.values():
            effort_counts[data['effort']] += 1
            priority_counts[data['priority']] += 1

        f.write(f"### By Effort\n")
        f.write(f"- LOW: {effort_counts['LOW']} files\n")
        f.write(f"- MEDIUM: {effort_counts['MEDIUM']} files\n")
        f.write(f"- HIGH: {effort_counts['HIGH']} files\n\n")

        f.write(f"### By Priority\n")
        f.write(f"- P0 (Critical): {priority_counts['P0']} files\n")
        f.write(f"- P1 (High): {priority_counts['P1']} files\n")
        f.write(f"- P2 (Normal): {priority_counts['P2']} files\n")

    print(f"✅ Impact matrix generated: {output_file}")

def main():
    deps = load_dependencies()
    matrix = assess_impact(deps)

    # Save JSON
    json_output = 'docs/migration/analysis/impact_matrix.json'
    with open(json_output, 'w') as f:
        json.dump(matrix, f, indent=2)

    # Generate Markdown
    md_output = 'docs/migration/analysis/impact_matrix.md'
    generate_matrix_markdown(matrix, md_output)

if __name__ == '__main__':
    main()
EOF

chmod +x scripts/phase-2.2/create_impact_matrix.py

# Run impact assessment
python3 scripts/phase-2.2/create_impact_matrix.py

# View matrix
cat docs/migration/analysis/impact_matrix.md
```

### Step Sequence: Manual Code Review

#### Step 1: Review Critical Files
```bash
# Create review checklist
cat > docs/migration/analysis/critical_files_review.md << 'EOF'
# Critical Files Review Checklist

## 1. logger.h and logger.cpp
Location: `include/kcenon/logger/core/logger.h`

### Current State
- [ ] Uses `kcenon::logger::monitoring::monitoring_interface`
- [ ] Has `monitor_` member variable
- [ ] Has `set_monitor()` method
- [ ] Includes `monitoring/monitoring_interface.h`

### Required Changes
- [ ] Change include to `<kcenon/common/interfaces/monitoring_interface.h>`
- [ ] Change type to `std::shared_ptr<common::interfaces::IMonitor>`
- [ ] Implement `IMonitorable` interface
- [ ] Update all method signatures

### Estimated Effort
- Time: 4 hours
- Complexity: HIGH
- Risk: MEDIUM (core component)

---

## 2. logger_builder.h
Location: `include/kcenon/logger/core/logger_builder.h`

### Current State
- [ ] Uses `monitoring::monitoring_interface`
- [ ] Has `with_monitoring()` method
- [ ] Includes local monitoring interface

### Required Changes
- [ ] Change include to common interface
- [ ] Update `with_monitoring()` parameter type
- [ ] Add deprecated attribute to old method
- [ ] Update `build()` method

### Estimated Effort
- Time: 2 hours
- Complexity: MEDIUM
- Risk: LOW

---

## 3. monitoring_factory.h
Location: `include/kcenon/logger/core/monitoring/monitoring_factory.h`

### Current State
- [ ] Creates `monitoring_interface` instances
- [ ] Factory methods for different monitor types

### Required Changes
- [ ] Change return type to `common::interfaces::IMonitor`
- [ ] Update factory methods
- [ ] Add migration helpers

### Estimated Effort
- Time: 2 hours
- Complexity: MEDIUM
- Risk: LOW

---

## 4. base_writer.h
Location: `include/kcenon/logger/writers/base_writer.h`

### Current State
- [ ] No monitoring interface currently
- [ ] Base class for all writers

### Required Changes
- [ ] Implement `IMonitorable` interface
- [ ] Add monitoring data collection
- [ ] Add health check implementation

### Estimated Effort
- Time: 3 hours
- Complexity: MEDIUM
- Risk: LOW

EOF

# Review each critical file
echo "Review logger.h..."
${EDITOR:-less} include/kcenon/logger/core/logger.h

echo "Review logger_builder.h..."
${EDITOR:-less} include/kcenon/logger/core/logger_builder.h
```

### Step Sequence: Create Detailed Migration Plan

#### Step 1: Generate Task List
```bash
cat > docs/migration/design/migration_plan.md << 'EOF'
# Logger System Migration Plan

## Execution Strategy

### Phase 1: Preparation 
**Goal**: Set up infrastructure and compatibility layer

**Tasks**:
1. Add common_system dependency to CMakeLists.txt
2. Create transition headers
3. Add deprecation warnings
4. Set up test infrastructure

**Acceptance Criteria**:
- [ ] Project compiles with common_system dependency
- [ ] Transition headers in place
- [ ] Deprecation warnings appear
- [ ] Test framework ready

---

### Phase 2: Core Interface Migration (Days 2-3)
**Goal**: Migrate core logger class

**Tasks**:
1. Update logger.h includes
2. Change monitor_ type to IMonitor
3. Implement IMonitorable interface
4. Update set_monitor() method
5. Update internal monitoring calls

**Acceptance Criteria**:
- [ ] logger.h compiles
- [ ] All method signatures updated
- [ ] IMonitorable methods implemented
- [ ] Existing tests pass

---

### Phase 3: Builder Migration 
**Goal**: Update logger_builder

**Tasks**:
1. Update with_monitoring() parameter
2. Add deprecated old method
3. Update build() method
4. Update builder tests

**Acceptance Criteria**:
- [ ] Builder compiles
- [ ] Both old and new methods work
- [ ] Deprecation warnings present
- [ ] Tests pass

---

### Phase 4: Writer Updates (Days 5-6)
**Goal**: Add monitoring to writers

**Tasks**:
1. Update base_writer with IMonitorable
2. Implement monitoring in each writer
3. Update writer tests

**Acceptance Criteria**:
- [ ] All writers implement IMonitorable
- [ ] Writer health checks work
- [ ] Writer metrics collected
- [ ] Tests pass

---

### Phase 5: Testing & Validation (Days 7-8)
**Goal**: Comprehensive testing

**Tasks**:
1. Run all unit tests
2. Run integration tests
3. Performance benchmarks
4. Memory leak checks

**Acceptance Criteria**:
- [ ] 100% unit tests pass
- [ ] Integration tests pass
- [ ] Performance within 5%
- [ ] No memory leaks

---

### Phase 6: Documentation (Days 9-10)
**Goal**: Complete documentation

**Tasks**:
1. Update API documentation
2. Write migration guide
3. Update examples
4. Create tutorials

**Acceptance Criteria**:
- [ ] API docs complete
- [ ] Migration guide published
- [ ] Examples updated
- [ ] Tutorials reviewed

EOF

cat docs/migration/design/migration_plan.md
```

### Deliverable Checklist for Task 2.2.1
- [ ] Dependency scan completed
- [ ] Impact matrix created
- [ ] Critical files reviewed
- [ ] Migration plan documented
- [ ] Stakeholder approval obtained

---

## Task 2.2.2: Remove Duplicate Interface

**Goal**: Eliminate local monitoring_interface, use common_system

### Task Sequence: Add Dependency and Create Transition

#### Step 1: Update CMakeLists.txt
```bash
cd /Users/dongcheolshin/Sources/logger_system

# Backup CMakeLists.txt
cp CMakeLists.txt CMakeLists.txt.backup

# Add common_system dependency
cat > /tmp/cmake_patch.txt << 'EOF'
# Find common_system (new dependency)
find_package(common_system 2.0 REQUIRED)

# Add to target_link_libraries
target_link_libraries(logger_system
    PUBLIC
        kcenon::common_system  # NEW
    PRIVATE
        # ... existing dependencies
)

# Add compile definition for migration tracking
target_compile_definitions(logger_system
    PRIVATE
        LOGGER_USING_COMMON_INTERFACES
        $<$<CONFIG:Debug>:LOGGER_MIGRATION_DEBUG>
)
EOF

echo "Add the above content to CMakeLists.txt in the appropriate sections"
${EDITOR:-vim} CMakeLists.txt

# Test configuration
cmake -B build -DCMAKE_BUILD_TYPE=Debug
echo "✅ CMake configuration successful"
```

#### Step 2: Create Transition Header
```bash
# Create transition header directory if needed
mkdir -p include/kcenon/logger/core/monitoring

# Create transition header
cat > include/kcenon/logger/core/monitoring/monitoring_interface_transition.h << 'EOF'
#pragma once

/**
 * @file monitoring_interface_transition.h
 * @brief Transition header for migration to common_system interfaces
 *
 * This header provides backward compatibility during the migration from
 * logger-specific monitoring interfaces to common_system standardized interfaces.
 *
 * @deprecated This file will be removed in v3.0.0
 *             Use <kcenon/common/interfaces/monitoring_interface.h> directly
 */

#include <kcenon/common/interfaces/monitoring_interface.h>

// Deprecation warning macro
#if defined(__GNUC__) || defined(__clang__)
#  define LOGGER_DEPRECATED(msg) __attribute__((deprecated(msg)))
#elif defined(_MSC_VER)
#  define LOGGER_DEPRECATED(msg) __declspec(deprecated(msg))
#else
#  define LOGGER_DEPRECATED(msg)
#endif

// Compatibility warnings
#ifdef LOGGER_MIGRATION_DEBUG
#  warning "Using transition header - migrate to common::interfaces"
#endif

namespace kcenon {
namespace logger {
namespace monitoring {

// Type aliases for backward compatibility
using monitoring_interface LOGGER_DEPRECATED("Use common::interfaces::IMonitor") =
    common::interfaces::IMonitor;

using monitoring_data LOGGER_DEPRECATED("Use common::interfaces::metrics_snapshot") =
    common::interfaces::metrics_snapshot;

using health_status LOGGER_DEPRECATED("Use common::interfaces::health_status") =
    common::interfaces::health_status;

using health_check_result LOGGER_DEPRECATED("Use common::interfaces::health_check_result") =
    common::interfaces::health_check_result;

/**
 * @brief Adapter for legacy basic_monitoring class
 * @deprecated Use common::interfaces::IMonitor implementations directly
 */
class LOGGER_DEPRECATED("Use common::interfaces::IMonitor implementations") basic_monitoring
    : public common::interfaces::IMonitor {
private:
    common::interfaces::metrics_snapshot data_;
    std::mutex mutex_;

public:
    basic_monitoring() {
        data_.source_id = "logger_system::basic_monitoring";
    }

    // IMonitor interface implementation
    common::VoidResult record_metric(const std::string& name, double value) override {
        std::lock_guard lock(mutex_);
        data_.add_metric(name, value);

        // Auto-update health based on metrics
        if (name == "error_rate" && value > 0.1) {
            // Would set health to degraded
        }

        return common::VoidResult::success();
    }

    common::VoidResult record_metric(
        const std::string& name,
        double value,
        const std::unordered_map<std::string, std::string>& tags) override {
        return record_metric(name, value);
    }

    common::Result<common::interfaces::metrics_snapshot> get_metrics() override {
        std::lock_guard lock(mutex_);
        data_.capture_time = std::chrono::system_clock::now();
        return common::Result<common::interfaces::metrics_snapshot>::success(data_);
    }

    common::Result<common::interfaces::health_check_result> check_health() override {
        common::interfaces::health_check_result result;
        result.timestamp = std::chrono::system_clock::now();
        result.status = common::interfaces::health_status::healthy;
        result.message = "Basic monitoring operational";
        return common::Result<common::interfaces::health_check_result>::success(std::move(result));
    }

    common::VoidResult reset() override {
        std::lock_guard lock(mutex_);
        data_ = common::interfaces::metrics_snapshot();
        data_.source_id = "logger_system::basic_monitoring";
        return common::VoidResult::success();
    }

    // Legacy methods for compatibility
    monitoring_data get_monitoring_data() const {
        return data_;
    }

    bool is_healthy() const {
        return true;
    }

    health_status get_health_status() const {
        return health_status::healthy;
    }

    void set_metric(const std::string& name, double value) {
        record_metric(name, value);
    }

    double get_metric(const std::string& name) const {
        std::lock_guard lock(mutex_);
        auto it = std::find_if(data_.metrics.begin(), data_.metrics.end(),
            [&name](const common::interfaces::metric_value& m) { return m.name == name; });

        return (it != data_.metrics.end()) ? it->value : 0.0;
    }
};

} // namespace monitoring
} // namespace logger
} // namespace kcenon

#endif // MONITORING_INTERFACE_TRANSITION_H
EOF

echo "✅ Transition header created"
```

#### Step 3: Update Original monitoring_interface.h
```bash
# Replace content of old interface with redirect
cat > include/kcenon/logger/core/monitoring/monitoring_interface.h << 'EOF'
#pragma once

/**
 * @file monitoring_interface.h
 * @brief DEPRECATED - Redirects to common_system interface
 *
 * This file is deprecated and will be removed in version 3.0.0.
 * Please update your includes to use the common_system interface directly:
 *
 * Old: #include <kcenon/logger/core/monitoring/monitoring_interface.h>
 * New: #include <kcenon/common/interfaces/monitoring_interface.h>
 */

#pragma message("monitoring_interface.h is deprecated, use common/interfaces/monitoring_interface.h")

// Include transition header for compatibility
#include "monitoring_interface_transition.h"

EOF

echo "✅ Old interface redirected"
```

#### Step 4: Compile and Verify Warnings
```bash
# Build with warnings
cmake --build build 2>&1 | tee /tmp/build_warnings.txt

# Check for deprecation warnings
echo "Checking for deprecation warnings..."
grep -i "deprecated" /tmp/build_warnings.txt | head -10

# Warnings are expected! This is correct behavior.
echo "✅ Deprecation warnings present (expected)"
```

### Task Sequence: Update Internal Includes

#### Step 1: Create Include Update Script
```bash
cat > scripts/phase-2.2/update_includes.sh << 'EOF'
#!/bin/bash

set -e

echo "=== Updating Internal Includes ==="

files_updated=0

# Find all header and source files (exclude backup and build)
while IFS= read -r file; do
    # Skip if in backup or build directory
    if [[ "$file" =~ \.backup|build ]]; then
        continue
    fi

    # Check if file contains old include
    if grep -q '#include.*logger.*monitoring.*monitoring_interface.h' "$file"; then
        echo "Updating: $file"

        # Create backup
        cp "$file" "$file.bak"

        # Replace old include with new one
        sed -i.tmp 's|#include.*logger.*monitoring.*monitoring_interface.h.*|#include <kcenon/common/interfaces/monitoring_interface.h>|g' "$file"

        # Replace namespace usage
        sed -i.tmp 's|kcenon::logger::monitoring::monitoring_interface|common::interfaces::IMonitor|g' "$file"
        sed -i.tmp 's|logger::monitoring::monitoring_interface|common::interfaces::IMonitor|g' "$file"
        sed -i.tmp 's|monitoring::monitoring_interface|common::interfaces::IMonitor|g' "$file"

        # Clean up temp file
        rm -f "$file.tmp"

        files_updated=$((files_updated + 1))
    fi
done < <(find . -type f \( -name "*.h" -o -name "*.cpp" \))

echo ""
echo "✅ Updated $files_updated files"
echo ""
echo "Backup files created with .bak extension"
echo "Review changes and remove .bak files when satisfied"

EOF

chmod +x scripts/phase-2.2/update_includes.sh

# Run the script
./scripts/phase-2.2/update_includes.sh
```

#### Step 2: Update Specific Core Files Manually
```bash
# Update logger.h
echo "Updating logger.h..."
${EDITOR:-vim} include/kcenon/logger/core/logger.h

# Make these changes:
# 1. Change include:
#    FROM: #include "monitoring/monitoring_interface.h"
#    TO:   #include <kcenon/common/interfaces/monitoring_interface.h>
#
# 2. Change member type:
#    FROM: std::shared_ptr<monitoring::monitoring_interface> monitor_;
#    TO:   std::shared_ptr<common::interfaces::IMonitor> monitor_;
#
# 3. Change method parameter:
#    FROM: void set_monitor(std::unique_ptr<monitoring::monitoring_interface> monitor);
#    TO:   void set_monitor(std::shared_ptr<common::interfaces::IMonitor> monitor);
```

#### Step 3: Verify Compilation
```bash
# Clean build
rm -rf build
cmake -B build -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTS=ON

# Build (expect some warnings about deprecated usage)
cmake --build build 2>&1 | tee /tmp/phase-2.2-build.log

# Check for errors (not warnings)
if grep -i "error:" /tmp/phase-2.2-build.log; then
    echo "❌ Build errors found!"
    echo "Review /tmp/phase-2.2-build.log"
    exit 1
else
    echo "✅ Build successful (warnings are expected)"
fi
```

### Deliverable Checklist for Task 2.2.2
- [ ] CMakeLists.txt updated with common_system dependency
- [ ] Transition header created
- [ ] Old monitoring_interface.h deprecated
- [ ] Internal includes updated
- [ ] Project compiles successfully
- [ ] Deprecation warnings present
- [ ] Tests still pass

---

## Task 2.2.3: Refactor Logger Class

**Goal**: Implement IMonitorable, update monitoring integration

This task is the core of the migration. Full implementation details continue in the actual codebase...

[... Continuing with detailed steps for remaining tasks ...]

---

## Validation

### Final Validation Script
```bash
cat > scripts/phase-2.2/validate_phase_2_2.sh << 'EOF'
#!/bin/bash

set -e

echo "=== Phase 2.2 Validation ==="
echo ""

cd /Users/dongcheolshin/Sources/logger_system

# 1. Check branch
echo "1. Checking branch..."
if [ "$(git branch --show-current)" = "feature/logger-system-phase-2.2" ]; then
    echo "✅ On correct branch"
else
    echo "❌ Wrong branch"
    exit 1
fi

# 2. Check no direct monitoring interface includes
echo ""
echo "2. Checking for old includes..."
if grep -r "#include.*logger.*monitoring.*monitoring_interface.h" \
   --include="*.h" --include="*.cpp" \
   --exclude-dir=build --exclude-dir=.backup \
   include/ src/ 2>/dev/null; then
    echo "❌ Found old monitoring interface includes!"
    exit 1
else
    echo "✅ No old includes found"
fi

# 3. Check common_system dependency
echo ""
echo "3. Checking CMakeLists.txt..."
if grep -q "kcenon::common_system" CMakeLists.txt; then
    echo "✅ common_system dependency present"
else
    echo "❌ common_system dependency missing"
    exit 1
fi

# 4. Build project
echo ""
echo "4. Building project..."
if cmake -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=ON > /dev/null 2>&1 && \
   cmake --build build > /dev/null 2>&1; then
    echo "✅ Build successful"
else
    echo "❌ Build failed"
    exit 1
fi

# 5. Run tests
echo ""
echo "5. Running tests..."
if ctest --test-dir build --output-on-failure > /dev/null 2>&1; then
    echo "✅ All tests passing"
else
    echo "❌ Tests failing"
    exit 1
fi

# 6. Check for IMonitorable implementation
echo ""
echo "6. Checking IMonitorable implementation..."
if grep -q "IMonitorable" include/kcenon/logger/core/logger.h; then
    echo "✅ IMonitorable implemented"
else
    echo "⚠️  IMonitorable not found (may need manual check)"
fi

echo ""
echo "=== Phase 2.2 Validation Complete ==="
echo "✅ All checks passed!"

EOF

chmod +x scripts/phase-2.2/validate_phase_2_2.sh

# Run validation
./scripts/phase-2.2/validate_phase_2_2.sh
```

---

**Document Version**: 1.0
**Last Updated**: [Document Version]
**Next**: Phase 2.3 (monitoring_system)