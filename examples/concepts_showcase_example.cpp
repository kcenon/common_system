// BSD 3-Clause License
// Copyright (c) 2021-2025, 🍀☀🌕🌥 🌊
// See the LICENSE file in the project root for full license information.

/// @file concepts_showcase_example.cpp
/// @example concepts_showcase_example.cpp
/// @brief Demonstrates C++20 concepts for type-safe generic programming.
///
/// Shows Resultable, Unwrappable, MonadicResult, and other concepts
/// used to constrain template parameters throughout the ecosystem.
///
/// @see kcenon::common::concepts

#include <kcenon/common/patterns/result.h>
#include <kcenon/common/concepts/core.h>

#include <iostream>
#include <string>
#include <type_traits>

using namespace kcenon::common;

// Constrained function: only accepts Resultable types
template <concepts::Resultable R>
void inspect_result(const R& result, const std::string& label)
{
	std::cout << "   " << label << ": "
			  << (result.is_ok() ? "OK" : "ERROR") << "\n";
}

// Unwrap with fallback using Result API directly
template <typename T>
T safe_unwrap(const Result<T>& val, const T& fallback)
{
	return val.unwrap_or(fallback);
}

// Constrained function: full monadic chain
template <concepts::MonadicResult R>
auto transform_chain(R result)
{
	return result
		.map([](const auto& v) { return v * 2; })
		.map([](const auto& v) { return v + 10; });
}

// Concept-based overload: demonstrate SFINAE replacement
template <typename T>
void process(const T& value)
{
	if constexpr (concepts::Resultable<T>)
	{
		std::cout << "   Processing Result type: "
				  << (value.is_ok() ? "success" : "failure") << "\n";
	}
	else
	{
		std::cout << "   Processing non-Result type: " << value << "\n";
	}
}

int main()
{
	std::cout << "=== C++20 Concepts Showcase ===\n\n";

	// 1. Resultable concept
	std::cout << "1. Resultable concept:\n";
	auto success = ok(42);
	auto failure = make_error<int>(error_codes::INVALID_ARGUMENT, "bad input", "demo");

	inspect_result(success, "ok(42)");
	inspect_result(failure, "error");

	// Compile-time check
	static_assert(concepts::Resultable<Result<int>>,
				  "Result<int> must satisfy Resultable");
	std::cout << "   static_assert: Result<int> is Resultable\n";

	// 2. Unwrappable concept
	std::cout << "\n2. Unwrappable concept:\n";
	auto val = safe_unwrap(success, -1);
	std::cout << "   safe_unwrap(ok(42), -1) = " << val << "\n";

	auto val2 = safe_unwrap(failure, -1);
	std::cout << "   safe_unwrap(error, -1) = " << val2 << "\n";

	// 3. MonadicResult concept (map + and_then)
	std::cout << "\n3. MonadicResult concept:\n";
	auto chained = transform_chain(ok(5));
	if (chained.is_ok())
	{
		std::cout << "   transform_chain(ok(5)): 5 * 2 + 10 = " << chained.value() << "\n";
	}

	auto chained_err = transform_chain(
		make_error<int>(error_codes::INTERNAL_ERROR, "oops", "demo"));
	std::cout << "   transform_chain(error): is_ok = "
			  << (chained_err.is_ok() ? "true" : "false") << "\n";

	// 4. Concept-based dispatch
	std::cout << "\n4. Concept-based dispatch:\n";
	process(ok(100));
	process(42);
	process(std::string("hello"));

	// 5. Compile-time concept checks
	std::cout << "\n5. Compile-time type traits:\n";
	std::cout << "   Result<int> is Resultable: "
			  << concepts::Resultable<Result<int>> << "\n";
	std::cout << "   Result<int> is Mappable: "
			  << concepts::Mappable<Result<int>> << "\n";
	std::cout << "   int is Resultable: "
			  << concepts::Resultable<int> << "\n";

	std::cout << "\nDone.\n";
	return 0;
}
