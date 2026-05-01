# kcenon-cmake-template

Reusable, project-agnostic CMake module suite that implements the canonical
build decomposition described in
[`docs/kcenon-system-layout.md`](../../docs/kcenon-system-layout.md).

The template is owned by `common_system` (Tier 0) and adopted by the other
seven kcenon ecosystem systems. Every system that follows this template
ends up with a thin root `CMakeLists.txt` that orchestrates eight
`include()`-able modules with the same names and the same responsibilities
across the ecosystem.

## Directory Layout

```
cmake/template/
├── README.md            # This file
├── VERSION              # Template version — track adoption per system
├── options.cmake        # Standard option set + small utility helpers
├── compiler.cmake       # C++ standard / build-type / IDE baseline
├── dependencies.cmake   # find_package / FetchContent helpers
├── warnings.cmake       # Per-target warning flags (no global propagation)
├── targets.cmake        # Target-creation conventions and helpers
├── install.cmake        # Install rules and config-package generation
├── testing.cmake        # Test framework (Google Test) registration helpers
└── examples.cmake       # Example registration helpers
```

The eight `*.cmake` modules correspond 1-to-1 to the modules listed in the
layout standard table. Their order in the root `CMakeLists.txt` is not
arbitrary — see *Adoption Checklist* below for the canonical sequence.

## Required Variables

The template expects the consumer's root `CMakeLists.txt` to set the
following before invoking the helper functions:

| Variable                                   | Meaning                                                           | Example                          |
|--------------------------------------------|-------------------------------------------------------------------|----------------------------------|
| `PROJECT_NAME` / `PROJECT_VERSION`         | Set by `project()`. Used by `install.cmake`.                      | `common_system` / `0.2.0`        |
| `<PREFIX>` (passed to helpers, not a var)  | Uppercase ASCII prefix for cache options                          | `COMMON`, `PACS`, `THREAD`       |

`<PREFIX>` is supplied at call time to:

* `kcenon_template_define_standard_options(<PREFIX>)`
* `kcenon_template_init_warnings(<PREFIX>)`
* `kcenon_template_setup_testing(<PREFIX>)`
* `kcenon_template_add_example_dirs(<PREFIX> SUBDIRS ...)`

The PREFIX produces canonical option names: `<PREFIX>_BUILD_TESTS`,
`<PREFIX>_BUILD_EXAMPLES`, `<PREFIX>_BUILD_BENCHMARKS`, `<PREFIX>_BUILD_DOCS`,
`<PREFIX>_BUILD_MODULES`, `<PREFIX>_WARNINGS_AS_ERRORS`.

## Adoption Checklist

For a new system adopting the template:

1. **Copy** the template directory (`cp -r common_system/cmake/template
   <new-system>/cmake/template`). Do NOT modify the template files in place
   — modifications belong upstream in `common_system`.
2. **Add** the template directory to `CMAKE_MODULE_PATH` near the top of
   the root `CMakeLists.txt`:

       list(APPEND CMAKE_MODULE_PATH
           "${CMAKE_CURRENT_SOURCE_DIR}/cmake/template")

3. **Include** every module by short name. CMake searches `CMAKE_MODULE_PATH`
   so each `include(<name>)` resolves to `cmake/template/<name>.cmake`:

       include(options)
       include(compiler)
       include(dependencies)
       include(warnings)
       include(targets)
       include(install)
       include(testing)
       include(examples)

4. **Set the standard baseline** before declaring options:

       kcenon_template_set_cpp_baseline(STANDARD 20)
       kcenon_template_set_default_build_type(TYPE Release)
       kcenon_template_enable_compile_commands()

5. **Declare options** with your project's prefix, then add any
   project-specific options:

       kcenon_template_define_standard_options(<PREFIX>)
       option(<PREFIX>_HEADER_ONLY "Use as header-only library" ON)
       # ... other project-specific options

6. **Initialise warnings** once the prefix is known:

       kcenon_template_init_warnings(<PREFIX>)

7. **Define your targets** in `cmake/<project>-targets.cmake` (or inline)
   following the conventions in `targets.cmake`. After each target, register
   the export alias:

       add_library(${PROJECT_NAME} INTERFACE)
       kcenon_template_setup_target_includes(${PROJECT_NAME} INTERFACE
           "${CMAKE_CURRENT_SOURCE_DIR}/include")
       kcenon_template_set_cxx_feature_std(${PROJECT_NAME} INTERFACE 20)
       kcenon_template_register_export_target(${PROJECT_NAME}
           ${PROJECT_NAME} <namespace>)

   For non-INTERFACE targets, also call:

       kcenon_template_apply_warnings(${PROJECT_NAME}_core)

8. **Register install rules** with one helper call:

       kcenon_template_install_headers("${CMAKE_CURRENT_SOURCE_DIR}/include")
       kcenon_template_install_package(
           PACKAGE_NAME    ${PROJECT_NAME}
           NAMESPACE       <namespace>
           TARGETS         ${PROJECT_NAME}
           CONFIG_TEMPLATE "${CMAKE_CURRENT_SOURCE_DIR}/cmake/${PROJECT_NAME}-config.cmake.in"
       )

9. **Set up tests** when the framework is available. Wrap the helper call
   in your project's own option check (single-flag projects use
   `if(<PREFIX>_BUILD_TESTS)`; projects with multiple test flags use a
   composite condition):

       if(<PREFIX>_BUILD_TESTS)
           kcenon_template_setup_testing()
           if(KCENON_TEMPLATE_GTEST_FOUND)
               add_subdirectory(tests)
           endif()
       endif()

10. **Register examples** under their option:

        kcenon_template_add_example_dirs(<PREFIX>
            SUBDIRS examples/hello examples/dump)

11. **Build and run tests** to verify the adoption:

        cmake --preset debug
        cmake --build build-debug
        cd build-debug && ctest --output-on-failure

12. **Record the template version** in your project's CHANGELOG entry so
    future upgrades can be traced.

## Version Policy

Template versioning follows [Semantic Versioning](https://semver.org/) and is
recorded in `cmake/template/VERSION` (currently `1.0.0`).

| Bump  | Trigger                                                                            |
|-------|-------------------------------------------------------------------------------------|
| MAJOR | Removing or renaming a public helper, changing required arguments incompatibly,     |
|       | changing the canonical module set, or changing variable contracts in a breaking way |
| MINOR | Adding a new helper, adding optional arguments to an existing helper, or adding     |
|       | a new module slot                                                                   |
| PATCH | Internal refactoring, documentation fixes, helper bug fixes that preserve contract  |

When a downstream system adopts the template, it pins to a specific version
(typically by recording the SHA of the template directory or its VERSION
string in the project README). Upgrades are coordinated by:

1. The template authors update files under `common_system/cmake/template/`
   and bump VERSION.
2. Downstream systems pull in the new template by re-copying the directory
   (or symlinking, when local development).
3. The downstream system bumps its own minor version if the template change
   exposed a new option or helper to its own consumers.

Major template upgrades are pre-announced via an issue in `common_system`
referencing every downstream repo so adoption can be scheduled together.

## Reference Adoption

`common_system` itself is the first adopter and serves as the reference
implementation. See
[`common_system/CMakeLists.txt`](../../CMakeLists.txt) for an end-to-end
example of a header-only INTERFACE library consuming every module of the
template.

## Cross-References

* Layout standard: [`docs/kcenon-system-layout.md`](../../docs/kcenon-system-layout.md)
* Cross-build CI (validates structural changes): tracked in
  [#660](https://github.com/kcenon/common_system/issues/660)
* Template extraction: this directory was extracted in
  [#659](https://github.com/kcenon/common_system/issues/659)
