# CI/CD Guide - common_system

This document describes the Continuous Integration and Continuous Deployment (CI/CD) pipeline for the `common_system` project, a header-only C++ library. The pipelines are designed to validate code quality, test functionality, and ensure documentation is always up-to-date.

## Table of Contents

1. [Overview](#overview)
2. [Workflow Triggers](#workflow-triggers)
3. [CI Pipeline](#ci-pipeline)
4. [Static Analysis Pipeline](#static-analysis-pipeline)
5. [Integration Tests Pipeline](#integration-tests-pipeline)
6. [Code Coverage Pipeline](#code-coverage-pipeline)
7. [Documentation Pipeline](#documentation-pipeline)
8. [Local Testing](#local-testing)
9. [Understanding Phases](#understanding-phases)
10. [Troubleshooting](#troubleshooting)

## Overview

The common_system project uses GitHub Actions for automated CI/CD. All workflows are located in `.github/workflows/` and are designed specifically for a header-only library, which means:

- No library artifacts are built; instead, we validate that headers compile correctly across platforms
- Tests are compiled and executed to verify functionality
- Static analysis tools check code quality without requiring object files
- Cross-platform compilation checks ensure portability (Linux GCC/Clang, macOS, Windows MSVC)

### Active Workflows

| Workflow | File | Purpose |
|----------|------|---------|
| **CI** | `ci.yml` | Cross-platform compilation and tests |
| **Static Analysis** | `static-analysis.yml` | Clang-Tidy, Cppcheck, and circular dependency detection |
| **Integration Tests** | `integration-tests.yml` | Integration and performance tests |
| **Code Coverage** | `coverage.yml` | Test coverage measurement |
| **Documentation** | `build-Doxygen.yaml` | API documentation generation |

## Workflow Triggers

### CI Workflow (`ci.yml`)

**When it runs:**
- On push to `main` or `phase-*` branches
- On pull requests targeting `main`

**What it does:**
1. Compiles the library headers across multiple platforms
2. Runs unit tests
3. Collects sanitizer results
4. Uploads build artifacts on failure

### Static Analysis Workflow (`static-analysis.yml`)

**When it runs:**
- On push to `main` or `phase-*` branches
- On pull requests targeting `main`

**What it does:**
1. Analyzes code with Clang-Tidy
2. Analyzes code with Cppcheck
3. Generates baseline reports for future comparison

### Integration Tests Workflow (`integration-tests.yml`)

**When it runs:**
- On push to `main`, `develop`, or `feat/**` branches
- On pull requests targeting `main` or `develop`

**What it does:**
1. Runs integration tests in Debug and Release modes
2. Executes performance benchmarks
3. Collects test results and coverage data
4. Uploads results to Codecov

### Code Coverage Workflow (`coverage.yml`)

**When it runs:**
- On push to `main` or `phase-*` branches
- On pull requests targeting `main`

**What it does:**
1. Compiles with coverage instrumentation
2. Executes all unit tests
3. Generates coverage reports using lcov
4. Uploads filtered coverage data to Codecov
5. Publishes HTML coverage report as artifacts

### Documentation Workflow (`build-Doxygen.yaml`)

**When it runs:**
- On push to `main`
- On pull requests targeting `main`
- On manual trigger (workflow_dispatch)

**What it does:**
1. Generates API documentation using Doxygen
2. Uploads documentation as artifacts
3. Deploys to GitHub Pages (only on push to main)

## CI Pipeline

### Workflow File: `.github/workflows/ci.yml`

This is the primary compilation validation workflow, testing cross-platform compatibility.

#### Matrix Configuration

The workflow tests the following combinations:

```yaml
matrix:
  include:
    - os: ubuntu-22.04
      compiler: gcc
    - os: ubuntu-22.04
      compiler: clang
    - os: macos-latest
      compiler: clang
    - os: windows-2022
      compiler: msvc
```

This ensures the header-only library compiles correctly with:
- **GCC** on Ubuntu
- **Clang** on Ubuntu and macOS
- **MSVC** on Windows

#### Build Steps

##### 1. Checkout and Dependencies

```bash
# Checkout code with submodules
git checkout --submodules=recursive

# Ubuntu
sudo apt-get install -y cmake ninja-build g++ clang libgtest-dev libgmock-dev

# macOS
brew install ninja googletest

# Windows
# Uses MSVC via GitHub Actions
```

##### 2. Compiler Setup

Each job sets the appropriate compiler:

```bash
# GCC
echo "CC=gcc" >> $GITHUB_ENV
echo "CXX=g++" >> $GITHUB_ENV

# Clang (Linux)
echo "CC=clang" >> $GITHUB_ENV
echo "CXX=clang++" >> $GITHUB_ENV

# Clang (macOS)
echo "CC=clang" >> $GITHUB_ENV
echo "CXX=clang++" >> $GITHUB_ENV

# MSVC
# Uses ilammy/msvc-dev-cmd@v1 action
```

##### 3. CMake Configuration

Unix systems:

```bash
cmake -B build -G Ninja \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCOMMON_BUILD_TESTS=ON \
  -DCOMMON_BUILD_EXAMPLES=ON \
  -DCOMMON_HEADER_ONLY=ON
```

Windows (PowerShell):

```powershell
cmake -B build -G Ninja `
  -DCMAKE_BUILD_TYPE=Debug `
  -DCOMMON_BUILD_TESTS=ON `
  -DCOMMON_BUILD_EXAMPLES=ON `
  -DCOMMON_HEADER_ONLY=ON
```

Key options:
- `-DCMAKE_BUILD_TYPE=Debug`: Debug symbols for better diagnostics
- `-DCOMMON_BUILD_TESTS=ON`: Include unit tests
- `-DCOMMON_BUILD_EXAMPLES=ON`: Include example code
- `-DCOMMON_HEADER_ONLY=ON`: Verify header-only compilation

##### 4. Build

```bash
cmake --build build --config Debug
```

##### 5. Test Execution

Unix:

```bash
cd build
ctest -C Debug --output-on-failure --verbose || true
```

Note: The `|| true` allows the job to continue even if tests fail (Phase 0 behavior).

##### 6. Artifact Upload on Failure

Failed builds upload logs for debugging:

```bash
build/CMakeFiles/*.log
build/Testing/Temporary/
```

#### Sanitizer Jobs

The workflow also includes a separate **sanitizer** job that tests with memory and undefined behavior sanitizers:

```yaml
matrix:
  sanitizer: [thread, address, undefined]
```

Configuration with sanitizers:

```bash
cmake -B build -G Ninja \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_CXX_COMPILER=clang++ \
  -DCMAKE_C_COMPILER=clang \
  -DCMAKE_CXX_FLAGS="-fsanitize=<sanitizer> -fno-omit-frame-pointer -g" \
  -DCOMMON_BUILD_TESTS=ON \
  -DCOMMON_BUILD_EXAMPLES=ON \
  -DCOMMON_HEADER_ONLY=ON
```

Environment variables for test execution:

```bash
ASAN_OPTIONS=detect_leaks=1 \
UBSAN_OPTIONS=print_stacktrace=1 \
ctest -C Debug --output-on-failure --verbose || true
```

## Static Analysis Pipeline

### Workflow File: `.github/workflows/static-analysis.yml`

This workflow runs static analysis tools to identify potential code quality issues and design problems.

### Clang-Tidy Analysis

#### Setup

```bash
sudo apt-get install -y cmake ninja-build clang clang-tidy libgtest-dev libgmock-dev
```

#### Configuration

```bash
cmake -B build -G Ninja \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
  -DCMAKE_CXX_COMPILER=clang++ \
  -DCMAKE_C_COMPILER=clang \
  -DCOMMON_BUILD_TESTS=ON \
  -DCOMMON_BUILD_EXAMPLES=ON \
  -DCOMMON_HEADER_ONLY=ON
```

The `-DCMAKE_EXPORT_COMPILE_COMMANDS=ON` flag is essential for Clang-Tidy to understand the compilation context.

#### Execution

```bash
find include -name "*.h" -o -name "*.hpp" | while read file; do
  echo "Analyzing: $file"
  clang-tidy "$file" -p=build -- -std=c++20 || true
done > clang-tidy-results.txt 2>&1
```

This:
- Finds all header files in the `include` directory
- Analyzes each file with the build database from the `build` directory
- Captures output to `clang-tidy-results.txt`
- Continues on errors (Phase 0 baseline collection)

#### Report Generation

```bash
# Count warnings by category
grep "warning:" clang-tidy-results.txt | \
  sed 's/.*\[\(.*\)\]/\1/' | \
  sort | uniq -c | sort -rn >> clang-tidy-summary.md
```

Artifacts:
- `clang-tidy-results.txt`: Full analysis output
- `clang-tidy-summary.md`: Summary with warning counts

### Cppcheck Analysis

#### Setup

```bash
sudo apt-get install -y cppcheck
```

#### Execution

```bash
cppcheck --enable=all \
  --std=c++20 \
  --suppress=missingIncludeSystem \
  --suppress=unusedFunction \
  --suppress=unmatchedSuppression \
  --inline-suppr \
  --xml \
  --xml-version=2 \
  -I include \
  include 2> cppcheck-results.xml || true
```

Configuration options:
- `--enable=all`: Run all checks
- `--std=c++20`: Specify C++ standard
- `--suppress=...`: Suppress known false positives
- `--inline-suppr`: Allow suppression comments in code
- `--xml`: Output in XML format for parsing

#### Report Generation

```bash
# Count issues by severity
grep -oP 'severity="\K[^"]+' cppcheck-results.xml | \
  sort | uniq -c | sort -rn >> cppcheck-summary.md
```

Artifacts:
- `cppcheck-results.xml`: Detailed analysis results
- `cppcheck-summary.md`: Summary with issue counts by severity

### Circular Dependency Check

The static analysis workflow includes a circular dependency detection job that analyzes header include relationships to prevent dependency cycles.

#### What It Checks

1. **Direct circular includes**: A includes B, and B includes A
2. **Transitive cycles**: A -> B -> C -> A chains

#### Execution

```bash
python3 scripts/check_circular_deps.py --output circular-deps-report.md
```

This Python script:
- Scans all `.h` and `.hpp` files in `include/`
- Builds a dependency graph from `#include` directives
- Detects both direct and transitive circular dependencies
- Generates a report with the dependency graph

#### Local Testing

```bash
# Run the check locally
python3 scripts/check_circular_deps.py --verbose

# Generate a report file
python3 scripts/check_circular_deps.py --output report.md
```

#### Artifacts

- `circular-deps-report.md`: Dependency analysis report

#### Failure Behavior

Unlike other static analysis checks (which use `continue-on-error: true`), the circular dependency check will **fail the CI** if cycles are detected. This is enforced because:
- Circular dependencies can cause build failures
- They indicate architectural issues that should be resolved immediately
- The common_system must remain the foundation with no upstream dependencies

## Integration Tests Pipeline

### Workflow File: `.github/workflows/integration-tests.yml`

This workflow validates integration functionality and measures performance.

### Integration Tests Job

#### Matrix Configuration

```yaml
matrix:
  os: [ubuntu-latest, macos-latest]
  build_type: [Debug, Release]
```

Tests across:
- **Platforms**: Linux and macOS
- **Build modes**: Debug and Release optimizations

#### Build Configuration

```bash
cmake -B build \
  -G Ninja \
  -DCMAKE_BUILD_TYPE=${{ matrix.build_type }} \
  -DCOMMON_BUILD_TESTS=ON \
  -DCOMMON_BUILD_INTEGRATION_TESTS=ON \
  -DENABLE_COVERAGE=${{ matrix.build_type == 'Debug' && 'ON' || 'OFF' }}
```

Coverage is enabled only for Debug builds to capture profiling data.

#### Test Execution

```bash
ctest --output-on-failure --tests-regex "integration_" --verbose
```

This runs only tests matching the `integration_` prefix.

#### Coverage Report Generation

Executed only for Debug builds on Linux:

```bash
cd build

# Capture coverage data
lcov --directory . --capture --output-file coverage.info \
  --ignore-errors mismatch,source

# Filter system headers and test files
lcov --remove coverage.info '/usr/*' '*/test/*' '*/tests/*' '*/examples/*' \
  --output-file coverage_filtered.info \
  --ignore-errors mismatch,source,unused

# Display summary
lcov --list coverage_filtered.info

# Generate HTML report (optional)
genhtml coverage_filtered.info --output-directory coverage_html
```

#### Codecov Upload

```bash
codecov/codecov-action@v3
  files: build/coverage_filtered.info
  flags: integration-tests
  name: integration-tests-coverage
```

### Performance Benchmarks Job

#### Configuration

```bash
cmake -B build \
  -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DCOMMON_BUILD_TESTS=ON \
  -DCOMMON_BUILD_INTEGRATION_TESTS=ON
```

Note: Release build for accurate performance measurement.

#### Execution

```bash
ctest --output-on-failure --tests-regex ".*Performance.*" --verbose
```

Runs tests with "Performance" in their name.

#### Results

Benchmark results are uploaded as artifacts for historical comparison:

```bash
name: benchmark-results-${{ matrix.os }}
path: build/Testing/Temporary/
```

## Code Coverage Pipeline

### Workflow File: `.github/workflows/coverage.yml`

Detailed coverage measurement and tracking.

#### Dependencies

```bash
sudo apt-get install -y cmake ninja-build g++ libgtest-dev libgmock-dev lcov
```

#### CMake Configuration

```bash
cmake -B build -G Ninja \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_CXX_FLAGS="--coverage -fprofile-arcs -ftest-coverage" \
  -DCMAKE_C_FLAGS="--coverage -fprofile-arcs -ftest-coverage" \
  -DCOMMON_BUILD_TESTS=ON \
  -DCOMMON_BUILD_EXAMPLES=ON \
  -DCOMMON_HEADER_ONLY=ON
```

Coverage flags enable profiling data collection during test execution.

#### Coverage Report Pipeline

```bash
cd build

# Capture coverage data from all instrumented code
lcov --directory . --capture --output-file coverage.info || true

# Remove system headers, tests, and examples from report
lcov --remove coverage.info '/usr/*' '*/test/*' '*/tests/*' '*/examples/*' \
  --output-file coverage_filtered.info || true

# Generate HTML report
genhtml coverage_filtered.info --output-directory coverage_html || true

# Print summary to console
lcov --list coverage_filtered.info || true
```

#### Codecov Integration

```bash
codecov/codecov-action@v5
  files: ./build/coverage_filtered.info
  flags: unittests
  name: common_system-coverage
  fail_ci_if_error: false
  verbose: true
  env:
    CODECOV_TOKEN: ${{ secrets.CODECOV_TOKEN }}
```

**Note**: The `CODECOV_TOKEN` secret must be configured in GitHub repository settings.

#### Artifacts

Coverage reports are retained for 7 days:

```
build/coverage.info              # Raw coverage data
build/coverage_filtered.info     # Filtered coverage data
build/coverage_html/             # HTML report directory
```

#### Reporting

Summary is added to the GitHub Actions step summary for easy visibility in pull request reviews:

```bash
echo "## Coverage Summary" >> $GITHUB_STEP_SUMMARY
echo "Phase 0 Target: Establish baseline coverage" >> $GITHUB_STEP_SUMMARY
echo "Phase 5 Target: 80%+ coverage" >> $GITHUB_STEP_SUMMARY
```

## Documentation Pipeline

### Workflow File: `.github/workflows/build-Doxygen.yaml`

Generates and deploys API documentation.

#### Triggers

- Push to `main` branch
- Pull requests to `main` branch
- Manual trigger via `workflow_dispatch`

#### Setup

```bash
sudo apt-get install -y doxygen graphviz
```

- **Doxygen**: Documentation generator
- **Graphviz**: Creates visual diagrams and graphs in documentation

#### Documentation Generation

```bash
doxygen Doxyfile
```

This runs Doxygen with the project's `Doxyfile` configuration.

#### Artifact Upload

Generated documentation is always uploaded:

```bash
name: documentation
path: documents/html/
retention-days: (default = 30 days)
```

#### GitHub Pages Deployment

Deployment occurs only on successful push to `main`:

```yaml
if: github.event_name == 'push' && github.ref == 'refs/heads/main'
uses: peaceiris/actions-gh-pages@v4
with:
  github_token: ${{ secrets.GITHUB_TOKEN }}
  publish_dir: ./documents/html
  enable_jekyll: false
  commit_message: "docs(gh-pages): update API docs via CI"
```

This:
- Publishes documentation to the `gh-pages` branch
- Disables Jekyll processing (pure HTML)
- Uses a conventional commit message for tracking

**Setup Required**: Enable GitHub Pages in repository settings:
1. Go to Settings → Pages
2. Set source to `Deploy from a branch`
3. Select `gh-pages` branch
4. Save

## Local Testing

To replicate CI/CD pipelines locally before pushing:

### Quick Build and Test

```bash
# Configure with header-only settings
cmake -B build -G Ninja \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCOMMON_BUILD_TESTS=ON \
  -DCOMMON_BUILD_EXAMPLES=ON \
  -DCOMMON_HEADER_ONLY=ON

# Build
cmake --build build --config Debug

# Run tests
cd build
ctest --output-on-failure --verbose
cd ..
```

### Static Analysis Locally

#### Clang-Tidy

```bash
# Configure with compile commands
cmake -B build -G Ninja \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
  -DCMAKE_CXX_COMPILER=clang++ \
  -DCMAKE_C_COMPILER=clang \
  -DCOMMON_BUILD_TESTS=ON \
  -DCOMMON_BUILD_EXAMPLES=ON \
  -DCOMMON_HEADER_ONLY=ON

cmake --build build

# Run analysis
find include -name "*.h" -o -name "*.hpp" | while read file; do
  clang-tidy "$file" -p=build -- -std=c++20
done
```

#### Cppcheck

```bash
cppcheck --enable=all \
  --std=c++20 \
  --suppress=missingIncludeSystem \
  --suppress=unusedFunction \
  --suppress=unmatchedSuppression \
  --inline-suppr \
  -I include \
  include
```

### Coverage Report Locally

```bash
# Configure with coverage flags
cmake -B build -G Ninja \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_CXX_FLAGS="--coverage -fprofile-arcs -ftest-coverage" \
  -DCMAKE_C_FLAGS="--coverage -fprofile-arcs -ftest-coverage" \
  -DCOMMON_BUILD_TESTS=ON \
  -DCOMMON_BUILD_EXAMPLES=ON \
  -DCOMMON_HEADER_ONLY=ON

cmake --build build
cd build
ctest --output-on-failure --verbose

# Generate coverage
lcov --directory . --capture --output-file coverage.info
lcov --remove coverage.info '/usr/*' '*/test/*' '*/tests/*' '*/examples/*' \
  --output-file coverage_filtered.info
genhtml coverage_filtered.info --output-directory coverage_html

# View results
open coverage_html/index.html  # macOS
# or
xdg-open coverage_html/index.html  # Linux
```

### Documentation Generation Locally

```bash
# Ensure Doxygen and Graphviz are installed
brew install doxygen graphviz  # macOS
# or
sudo apt-get install doxygen graphviz  # Linux

# Generate documentation
doxygen Doxyfile

# View documentation
open documents/html/index.html  # macOS
# or
xdg-open documents/html/index.html  # Linux
```

## Understanding Phases

The common_system project uses a phased approach to gradually enforce stricter quality standards:

### Phase 0 (Current)

**Objective**: Establish baseline metrics and identify issues without blocking CI.

**Characteristics**:
- CI tests may fail but don't block merges
- Static analysis continues on errors (`continue-on-error: true`)
- Sanitizer results collected but not enforced
- Coverage baseline being established
- Goal: Build comprehensive understanding of code quality

**Example**:
```yaml
- name: Run clang-tidy
  continue-on-error: true  # Phase 0: Allow failures, collect baseline
```

### Future Phases (Planned)

- **Phase 1**: Enforce no compilation warnings
- **Phase 2**: Require positive test results
- **Phase 3**: Enforce static analysis standards
- **Phase 4**: Require 60%+ coverage
- **Phase 5**: Require 80%+ coverage

The `phase-*` branch pattern in CI triggers allows testing new phase implementations before merging to `main`.

## Troubleshooting

### Build Fails on macOS

**Problem**: Missing dependencies after Homebrew installation.

**Solution**:
```bash
brew install cmake ninja googletest
```

Verify installation:
```bash
which clang++
which ninja
```

### Windows Build Fails with MSVC

**Problem**: MSVC compiler not found or environment not set.

**Solution**:
The workflow uses `ilammy/msvc-dev-cmd@v1` to set up the MSVC environment. Ensure you're running on `windows-2022` runner.

Local testing on Windows requires Visual Studio or Build Tools:
```powershell
# Use x64 Native Tools Command Prompt or similar
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build
```

### Clang-Tidy Produces False Positives

**Problem**: Analysis reports issues that aren't real.

**Solution**: Add inline suppressions in code:

```cpp
// NOLINTNEXTLINE(bugprone-*)
void my_function() { }
```

Or update `.clang-tidy` configuration file with appropriate checks.

### Coverage Report Not Generated

**Problem**: `lcov` command fails or produces empty results.

**Solution**:

1. Verify coverage flags are set:
```bash
cmake -DCMAKE_CXX_FLAGS="--coverage -fprofile-arcs -ftest-coverage" ...
```

2. Ensure tests actually run:
```bash
cd build
ctest --output-on-failure
```

3. Check for gcda files:
```bash
find build -name "*.gcda" -o -name "*.gcno"
```

If no profiling files exist, coverage wasn't collected.

### Codecov Token Issues

**Problem**: Coverage upload fails with authentication error.

**Solution**:

1. Verify token is set in repository secrets:
   - Go to Settings → Secrets and variables → Actions
   - Add `CODECOV_TOKEN` from https://codecov.io

2. Ensure Codecov is configured for the repository

3. Check workflow has access to secrets:
```yaml
env:
  CODECOV_TOKEN: ${{ secrets.CODECOV_TOKEN }}
```

### Documentation Deployment Fails

**Problem**: GitHub Pages deployment fails or doesn't update.

**Solution**:

1. Enable GitHub Pages:
   - Go to Settings → Pages
   - Set source to `Deploy from a branch`
   - Select `gh-pages` branch

2. Verify workflow permissions:
```yaml
permissions:
  contents: write
```

3. Check deployment status:
   - Go to Settings → Pages → Last deployment

4. Verify `documents/html/index.html` exists after build

### Tests Timeout

**Problem**: CI job timeout after 30 minutes.

**Solution**:

1. Check for hanging tests:
```bash
ctest --output-on-failure --verbose --timeout 10
```

2. Increase timeout in workflow (edit `timeout-minutes`):
```yaml
timeout-minutes: 45
```

3. Optimize slow tests:
   - Profile with `ctest --time`
   - Reduce test data sizes
   - Parallelize with `ctest -j <N>`

---

**Last Updated**: 2025-11-11
**Common System Version**: Phase 0 (Baseline Collection)
