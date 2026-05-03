cmake_minimum_required(VERSION 3.20)
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/VERSION" PROJECT_VERSION_RAW)
string(STRIP "${PROJECT_VERSION_RAW}" PROJECT_VERSION)
project(common_system VERSION ${PROJECT_VERSION} LANGUAGES CXX)

# =============================================================================
# BUILD ORDER DOCUMENTATION
# =============================================================================
#
# common_system is the foundation library for the unified_system ecosystem.
# It has NO dependencies on other unified_system modules.
#
# Dependency Order (common_system must be built FIRST):
#
#   common_system (this project)
#       └─> thread_system     (depends on common_system)
#       └─> logger_system     (depends on common_system)
#       └─> network_system    (depends on common_system)
#       └─> monitoring_system (depends on common_system)
#       └─> database_system   (depends on common_system)
#       └─> container_system  (depends on common_system)
#       └─> messaging_system  (depends on common_system)
#
# Interfaces provided by common_system:
#   - IHttpClient, IUdpClient (transport interfaces)
#   - IMetricCollector (monitoring interface)
#   - Result<T>, Error (patterns)
#   - Service container and DI utilities
#   - Configuration management
#   - Event bus (standalone implementation)
#
# For circular dependency prevention, see:
#   - scripts/check_circular_deps.py
#   - docs/CROSS_MODULE_INTEGRATION.md
#
# =============================================================================

# -----------------------------------------------------------------------------
# Module path: template (under cmake/template/) + project-specific (cmake/)
# -----------------------------------------------------------------------------
# common_system is the reference adopter of the template suite at
# cmake/template/. See cmake/template/README.md for the canonical adoption
# checklist; the calls below mirror that checklist 1-to-1. Project-specific
# layers (common-* modules) live directly under cmake/ and are included
# explicitly.
list(APPEND CMAKE_MODULE_PATH
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/template"
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake"
)

# -----------------------------------------------------------------------------
# Step 1-3: canonical template includes (ordered as in cmake/template/README.md)
# -----------------------------------------------------------------------------
include(options)
include(compiler)
include(dependencies)
include(warnings)
include(targets)
include(install)
include(testing)
include(examples)

# -----------------------------------------------------------------------------
# Step 4: standard baseline
# -----------------------------------------------------------------------------
kcenon_template_set_cpp_baseline(STANDARD 20)
kcenon_template_set_default_build_type(TYPE Release)
kcenon_template_enable_compile_commands()

# -----------------------------------------------------------------------------
# Step 5a: pre-set cache defaults that diverge from the template defaults so
# the subsequent option() calls inside `kcenon_template_define_standard_options`
# act as no-ops while preserving common_system's historical behaviour
# (examples ON by default, header-only ON by default).
# -----------------------------------------------------------------------------
set(COMMON_BUILD_EXAMPLES ON CACHE BOOL "Build example programs")

# Step 5b: standard options
kcenon_template_define_standard_options(COMMON)

# Step 5c: project-specific options
option(COMMON_BUILD_INTEGRATION_TESTS "Build integration tests for common_system" ON)
option(COMMON_HEADER_ONLY              "Use as header-only library"                ON)
option(ENABLE_COVERAGE                 "Enable code coverage"                      OFF)
option(BUILD_WITH_YAML_CPP             "Enable YAML configuration file support"    OFF)

# Step 6: warnings (header-only INTERFACE targets are skipped by the helper)
kcenon_template_init_warnings(COMMON)

# -----------------------------------------------------------------------------
# common_system-specific helpers (not part of the template)
# -----------------------------------------------------------------------------
include(features)
include(KcenonCompilerRequirements)
kcenon_check_compiler_requirements()

# -----------------------------------------------------------------------------
# Project-specific build layers (cmake/common-*.cmake)
# -----------------------------------------------------------------------------
# Each module owns one slice of the previously-inline root logic. Order
# matters: ABI generation must run before targets, targets before modules
# (which reuse the ABI version), targets before install, and tests/extras
# can run after the core targets are defined.
include(common-abi)
include(common-targets)
include(common-modules)
include(common-install)
include(common-tests)
include(common-extras)
include(common-summary)
