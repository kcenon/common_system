// BSD 3-Clause License
// Copyright (c) 2021-2025, 🍀☀🌕🌥 🌊
// See the LICENSE file in the project root for full license information.

/// @file feature_flags_example.cpp
/// @example feature_flags_example.cpp
/// @brief Demonstrates compile-time feature detection and conditional
///        compilation with feature flags.
///
/// Shows how to query compiler capabilities, C++ standard library features,
/// and ecosystem system availability at compile time.
///
/// @see kcenon/common/config/feature_flags.h

#include <kcenon/common/config/feature_flags.h>

#include <iostream>
#include <string>

// Helper to convert bool to string
constexpr const char* yes_no(bool v) { return v ? "yes" : "no"; }

int main()
{
	std::cout << "=== Feature Flags Example ===\n\n";

	// 1. C++ Standard Library Features
	std::cout << "1. C++ Standard Library Features:\n";

#if KCENON_HAS_SOURCE_LOCATION
	std::cout << "   std::source_location: supported\n";
#else
	std::cout << "   std::source_location: not supported (using polyfill)\n";
#endif

#if KCENON_HAS_JTHREAD
	std::cout << "   std::jthread: supported\n";
#else
	std::cout << "   std::jthread: not supported\n";
#endif

#ifdef __cpp_concepts
	std::cout << "   C++20 concepts: supported (__cpp_concepts=" << __cpp_concepts << ")\n";
#else
	std::cout << "   C++20 concepts: not supported\n";
#endif

#ifdef __cpp_lib_format
	std::cout << "   std::format: supported\n";
#else
	std::cout << "   std::format: not supported\n";
#endif

	// 2. Ecosystem System Dependencies
	std::cout << "\n2. Ecosystem System Dependencies:\n";

#if KCENON_WITH_THREAD_SYSTEM
	std::cout << "   thread_system: linked\n";
#else
	std::cout << "   thread_system: not linked\n";
#endif

#if KCENON_WITH_LOGGER_SYSTEM
	std::cout << "   logger_system: linked\n";
#else
	std::cout << "   logger_system: not linked\n";
#endif

#if KCENON_WITH_MONITORING_SYSTEM
	std::cout << "   monitoring_system: linked\n";
#else
	std::cout << "   monitoring_system: not linked\n";
#endif

#if KCENON_WITH_CONTAINER_SYSTEM
	std::cout << "   container_system: linked\n";
#else
	std::cout << "   container_system: not linked\n";
#endif

	// 3. Conditional compilation example
	std::cout << "\n3. Conditional compilation patterns:\n";

#if KCENON_HAS_SOURCE_LOCATION
	auto loc = std::source_location::current();
	std::cout << "   Called from: " << loc.file_name() << ":" << loc.line() << "\n";
#else
	std::cout << "   Source location: using fallback (__FILE__:__LINE__)\n";
	std::cout << "   Called from: " << __FILE__ << ":" << __LINE__ << "\n";
#endif

	// 4. Platform detection
	std::cout << "\n4. Platform:\n";
#if defined(_WIN32) || defined(_WIN64)
	std::cout << "   OS: Windows\n";
#elif defined(__APPLE__)
	std::cout << "   OS: macOS\n";
#elif defined(__linux__)
	std::cout << "   OS: Linux\n";
#else
	std::cout << "   OS: Unknown\n";
#endif

	std::cout << "\nDone.\n";
	return 0;
}
