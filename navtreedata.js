/*
 @licstart  The following is the entire license notice for the JavaScript code in this file.

 The MIT License (MIT)

 Copyright (C) 1997-2020 by Dimitri van Heesch

 Permission is hereby granted, free of charge, to any person obtaining a copy of this software
 and associated documentation files (the "Software"), to deal in the Software without restriction,
 including without limitation the rights to use, copy, modify, merge, publish, distribute,
 sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in all copies or
 substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
 BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

 @licend  The above is the entire license notice for the JavaScript code in this file
*/
var NAVTREE =
[
  [ "Common System", "index.html", [
    [ "System Overview", "index.html#overview", null ],
    [ "Key Features", "index.html#features", null ],
    [ "Architecture Diagram", "index.html#architecture", null ],
    [ "Quick Start", "index.html#quickstart", null ],
    [ "Installation", "index.html#installation", [
      [ "CMake FetchContent (Recommended)", "index.html#install_fetchcontent", null ],
      [ "vcpkg", "index.html#install_vcpkg", null ],
      [ "Header-Only (Manual)", "index.html#install_manual", null ]
    ] ],
    [ "Module Overview", "index.html#modules", null ],
    [ "Examples", "index.html#examples", null ],
    [ "Learning Resources", "index.html#learning_resources", null ],
    [ "Related Systems", "index.html#related", null ],
    [ "README", "md_README.html", [
      [ "Common System", "md_README.html#autotoc_md1", [
        [ "Table of Contents", "md_README.html#autotoc_md2", null ],
        [ "Overview", "md_README.html#autotoc_md4", [
          [ "API Stability", "md_README.html#autotoc_md5", null ]
        ] ],
        [ "Key Features", "md_README.html#autotoc_md7", null ],
        [ "Quick Start", "md_README.html#autotoc_md9", null ],
        [ "Requirements", "md_README.html#autotoc_md11", [
          [ "Compiler Requirements", "md_README.html#autotoc_md12", null ],
          [ "Ecosystem-Wide Compiler Requirements", "md_README.html#autotoc_md13", null ],
          [ "Dependency Flow", "md_README.html#autotoc_md14", null ]
        ] ],
        [ "Installation", "md_README.html#autotoc_md16", [
          [ "Installation via vcpkg", "md_README.html#autotoc_md17", null ],
          [ "CMake FetchContent (Recommended)", "md_README.html#autotoc_md18", null ],
          [ "Header-Only Usage (Simplest)", "md_README.html#autotoc_md19", null ],
          [ "C++20 Modules", "md_README.html#autotoc_md20", null ]
        ] ],
        [ "Architecture", "md_README.html#autotoc_md22", [
          [ "Module Structure", "md_README.html#autotoc_md23", null ],
          [ "Ecosystem Position", "md_README.html#autotoc_md24", null ]
        ] ],
        [ "Core Concepts", "md_README.html#autotoc_md26", [
          [ "Result<T> Pattern", "md_README.html#autotoc_md27", null ],
          [ "IExecutor Interface", "md_README.html#autotoc_md28", null ],
          [ "Health Monitoring", "md_README.html#autotoc_md29", null ],
          [ "Error Code Registry", "md_README.html#autotoc_md30", null ],
          [ "Circuit Breaker", "md_README.html#autotoc_md31", null ]
        ] ],
        [ "API Overview", "md_README.html#autotoc_md33", null ],
        [ "Examples", "md_README.html#autotoc_md35", [
          [ "Running Examples", "md_README.html#autotoc_md36", null ]
        ] ],
        [ "Performance", "md_README.html#autotoc_md38", null ],
        [ "Ecosystem Integration", "md_README.html#autotoc_md40", [
          [ "Ecosystem Dependency Map", "md_README.html#autotoc_md41", null ],
          [ "Ecosystem CI Verification", "md_README.html#autotoc_md42", null ],
          [ "Integration Example", "md_README.html#autotoc_md43", null ],
          [ "Documentation", "md_README.html#autotoc_md44", null ]
        ] ],
        [ "Contributing", "md_README.html#autotoc_md46", [
          [ "Quick Links", "md_README.html#autotoc_md47", null ],
          [ "Support", "md_README.html#autotoc_md48", null ]
        ] ],
        [ "License", "md_README.html#autotoc_md50", null ]
      ] ]
    ] ],
    [ "Ecosystem Guide: Which System Do I Need?", "ecosystem_guide.html", [
      [ "At a Glance", "ecosystem_guide.html#ecosystem_overview", null ],
      [ "Decision Tree", "ecosystem_guide.html#selection_decision_tree", null ],
      [ "Selection by Need", "ecosystem_guide.html#selection_by_need", null ],
      [ "Adoption Order", "ecosystem_guide.html#selection_by_dependency", null ],
      [ "Minimal Combinations", "ecosystem_guide.html#selection_minimal_combos", null ],
      [ "When NOT to Use", "ecosystem_guide.html#selection_anti_patterns", null ],
      [ "Cross-Navigation", "ecosystem_guide.html#selection_navigation", null ]
    ] ],
    [ "Tutorial: Result<T> Error Handling", "tutorial_result.html", [
      [ "Goal", "tutorial_result.html#result_goal", null ],
      [ "Prerequisites", "tutorial_result.html#result_prereq", null ],
      [ "Step 1: Creating Result values", "tutorial_result.html#result_step1", null ],
      [ "Step 2: Consuming Result values", "tutorial_result.html#result_step2", null ],
      [ "Step 3: Chaining operations", "tutorial_result.html#result_step3", null ],
      [ "Common Mistakes", "tutorial_result.html#result_mistakes", null ],
      [ "Next Steps", "tutorial_result.html#result_next", null ]
    ] ],
    [ "Tutorial: Dependency Injection", "tutorial_di.html", [
      [ "Goal", "tutorial_di.html#di_goal", null ],
      [ "Prerequisites", "tutorial_di.html#di_prereq", null ],
      [ "Step 1: Define a service interface", "tutorial_di.html#di_step1", null ],
      [ "Step 2: Register and resolve", "tutorial_di.html#di_step2", null ],
      [ "Step 3: Swap implementations in tests", "tutorial_di.html#di_step3", null ],
      [ "Common Mistakes", "tutorial_di.html#di_mistakes", null ],
      [ "Next Steps", "tutorial_di.html#di_next", null ]
    ] ],
    [ "Tutorial: Event Bus", "tutorial_eventbus.html", [
      [ "Goal", "tutorial_eventbus.html#eb_goal", null ],
      [ "Prerequisites", "tutorial_eventbus.html#eb_prereq", null ],
      [ "Step 1: Define an event type", "tutorial_eventbus.html#eb_step1", null ],
      [ "Step 2: Subscribe a handler", "tutorial_eventbus.html#eb_step2", null ],
      [ "Step 3: Publish from another component", "tutorial_eventbus.html#eb_step3", null ],
      [ "Common Mistakes", "tutorial_eventbus.html#eb_mistakes", null ],
      [ "Next Steps", "tutorial_eventbus.html#eb_next", null ]
    ] ],
    [ "Frequently Asked Questions", "faq.html", [
      [ "Result<T> Questions", "faq.html#faq_result", [
        [ "When should I use Result<T> instead of exceptions?", "faq.html#faq_result_vs_exceptions", null ],
        [ "How do I chain Result<T> operations without nesting?", "faq.html#faq_result_chain", null ],
        [ "Why doesn't .value() just return a default on error?", "faq.html#faq_result_unwrap", null ]
      ] ],
      [ "Dependency Injection Questions", "faq.html#faq_di", [
        [ "How does the service container lifecycle work?", "faq.html#faq_di_container_lifecycle", null ],
        [ "Can I use common_system without the rest of the ecosystem?", "faq.html#faq_di_standalone", null ]
      ] ],
      [ "Build and Integration Questions", "faq.html#faq_build", [
        [ "What C++ version is required?", "faq.html#faq_cpp_version", null ],
        [ "How do I integrate with existing exception-based code?", "faq.html#faq_integration", null ]
      ] ],
      [ "Threading Questions", "faq.html#faq_threading", [
        [ "Is the service container thread-safe?", "faq.html#faq_threading_safety", null ],
        [ "Does the event bus dispatch on a worker thread?", "faq.html#faq_eventbus_thread", null ]
      ] ],
      [ "Testing Questions", "faq.html#faq_testing", [
        [ "How do I mock a service for testing?", "faq.html#faq_test_mock", null ],
        [ "How do I test Result-returning functions?", "faq.html#faq_test_result", null ]
      ] ],
      [ "Performance Questions", "faq.html#faq_performance", [
        [ "Is Result<T> expensive compared to exceptions?", "faq.html#faq_result_cost", null ]
      ] ],
      [ "Deployment Questions", "faq.html#faq_deploy", [
        [ "Is common_system header-only?", "faq.html#faq_header_only", null ]
      ] ]
    ] ],
    [ "Troubleshooting Guide", "troubleshooting.html", [
      [ "Build Issues", "troubleshooting.html#ts_build", [
        [ "\"error: 'format' is not a member of 'std'\"", "troubleshooting.html#ts_build_cpp20", null ],
        [ "\"concept 'invocable' was not declared\"", "troubleshooting.html#ts_build_concept", null ]
      ] ],
      [ "CMake Configuration Issues", "troubleshooting.html#ts_cmake", [
        [ "\"Could not find package kcenon-common-system\"", "troubleshooting.html#ts_cmake_notfound", null ]
      ] ],
      [ "vcpkg Integration Issues", "troubleshooting.html#ts_vcpkg", [
        [ "\"port name not recognized\"", "troubleshooting.html#ts_vcpkg_port", null ]
      ] ],
      [ "Linker Issues", "troubleshooting.html#ts_linker", [
        [ "\"undefined reference to kcenon::common::event_bus::publish\"", "troubleshooting.html#ts_linker_undef", null ]
      ] ],
      [ "Runtime Issues", "troubleshooting.html#ts_runtime", [
        [ "\"service_container::resolve: type not registered\"", "troubleshooting.html#ts_runtime_resolve", null ],
        [ "\"bad_result_access thrown from Result::value()\"", "troubleshooting.html#ts_runtime_value", null ]
      ] ]
    ] ],
    [ "Modules", "modules.html", [
      [ "Modules List", "modules.html", "modules_dup" ],
      [ "Module Members", "modulemembers.html", [
        [ "All", "modulemembers.html", null ],
        [ "Variables", "modulemembers_vars.html", null ]
      ] ]
    ] ],
    [ "Namespaces", "namespaces.html", [
      [ "Namespace List", "namespaces.html", "namespaces_dup" ],
      [ "Namespace Members", "namespacemembers.html", [
        [ "All", "namespacemembers.html", "namespacemembers_dup" ],
        [ "Functions", "namespacemembers_func.html", null ],
        [ "Variables", "namespacemembers_vars.html", null ],
        [ "Typedefs", "namespacemembers_type.html", null ],
        [ "Enumerations", "namespacemembers_enum.html", null ]
      ] ]
    ] ],
    [ "Concepts", "concepts.html", "concepts" ],
    [ "Classes", "annotated.html", [
      [ "Class List", "annotated.html", "annotated_dup" ],
      [ "Class Index", "classes.html", null ],
      [ "Class Hierarchy", "hierarchy.html", "hierarchy" ],
      [ "Class Members", "functions.html", [
        [ "All", "functions.html", "functions_dup" ],
        [ "Functions", "functions_func.html", "functions_func" ],
        [ "Variables", "functions_vars.html", "functions_vars" ],
        [ "Typedefs", "functions_type.html", null ],
        [ "Enumerations", "functions_enum.html", null ],
        [ "Enumerator", "functions_eval.html", null ],
        [ "Related Symbols", "functions_rela.html", null ]
      ] ]
    ] ],
    [ "Files", "files.html", [
      [ "File List", "files.html", "files_dup" ],
      [ "File Members", "globals.html", [
        [ "All", "globals.html", null ],
        [ "Functions", "globals_func.html", null ],
        [ "Variables", "globals_vars.html", null ],
        [ "Macros", "globals_defs.html", null ]
      ] ]
    ] ],
    [ "Examples", "examples.html", "examples" ]
  ] ]
];

var NAVTREEINDEX =
[
"abi__version__example_8cpp.html",
"classkcenon_1_1common_1_1Result.html#a46ab6f5e930cb72a1e75ef3d9bd674f0",
"classkcenon_1_1common_1_1bootstrap_1_1SystemBootstrapper.html#a93b9cfe62a6b5affdc008a98768bdb17",
"classkcenon_1_1common_1_1di_1_1IBootstrapper.html#a82a3259c097d8168baffafd14f8f0f2c",
"classkcenon_1_1common_1_1di_1_1unified__bootstrapper.html#a44fe4956ad532e5565e831e239847276",
"classkcenon_1_1common_1_1interfaces_1_1IHttpClient.html#ae9ce4c5b08c80a4797021316411d5c56",
"classkcenon_1_1common_1_1interfaces_1_1IThreadPool.html#a78e6939320791d2683511b637e756380",
"classkcenon_1_1common_1_1interfaces_1_1health__check__builder.html#ac8ae49a4724a7c9f15c863f490bc3844",
"classkcenon_1_1common_1_1patterns_1_1SimpleEventBus.html#a2af8701a43a6defbc701b55c5772a5bd",
"classkcenon_1_1common_1_1utils_1_1CircularBuffer.html#aadb893c295d491d92b130f5068272d2d",
"conceptkcenon_1_1common_1_1concepts_1_1ExecutorLike.html",
"error_8cppm.html",
"feature__flags__core_8h.html",
"logger_8cppm.html#af76a50cdea659a8e6f425a0dfcb965c0a7e85bcb66fb9a809d5ab4f62a8b8bea8",
"module__kcenon_8common.html#a6258b9a34378c25aa0b2b0c89d5c2d55",
"module__kcenon_8common.html#ae22b76ef99468396d9751e8e6cdd7bd6",
"namespacekcenon_1_1common_1_1error.html#a7f8dcbed821435a2c26267c2a5f50bd8a9efab2399c7c560b34de477b9aa0a465",
"namespacekcenon_1_1common_1_1interfaces.html#af76a50cdea659a8e6f425a0dfcb965c0acaf9b6b99962bf5c2264824231d7a40c",
"structkcenon_1_1common_1_1config_1_1cli__option.html#a452cbe49c45ae9747d9d25e585ea84be",
"structkcenon_1_1common_1_1di_1_1service__container_1_1service__entry.html",
"structkcenon_1_1common_1_1interfaces_1_1http__request.html#ad46c8132d2db4df7c43992536e1e7bcb",
"structkcenon_1_1common_1_1interfaces_1_1worker__metrics.html#adcca6847d1b8d52d98c326514682b761"
];

var SYNCONMSG = 'click to disable panel synchronisation';
var SYNCOFFMSG = 'click to enable panel synchronisation';