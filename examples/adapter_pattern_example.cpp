// BSD 3-Clause License
// Copyright (c) 2021-2025, 🍀☀🌕🌥 🌊
// See the LICENSE file in the project root for full license information.

/// @file adapter_pattern_example.cpp
/// @example adapter_pattern_example.cpp
/// @brief Demonstrates the adapter pattern for uniform pointer-like access.
///
/// Shows wrapping raw values, shared_ptr, and unique_ptr with adapters
/// that provide a unified access interface regardless of ownership model.
///
/// @see kcenon::common::adapter
/// @see kcenon::common::adapter_traits

#include <kcenon/common/adapters/adapter.h>

#include <iostream>
#include <memory>
#include <string>

using namespace kcenon::common;
using namespace kcenon::common::adapters;

// A sample service class
class DataService
{
public:
	explicit DataService(std::string name) : name_(std::move(name)) {}

	std::string get_name() const { return name_; }
	void process() { std::cout << "      " << name_ << " processing data...\n"; }

private:
	std::string name_;
};

// Generic function that works with any adapter
template <typename T>
void use_service(const adapter<T>& svc)
{
	std::cout << "   Pointer valid: " << (svc.get() != nullptr ? "true" : "false") << "\n";
	svc->process();
}

int main()
{
	std::cout << "=== Adapter Pattern Example ===\n\n";

	// 1. Wrap a shared_ptr
	std::cout << "1. Shared pointer adapter:\n";
	{
		auto service = std::make_shared<DataService>("SharedService");
		auto adapted = adapter(service);

		std::cout << "   Name via ->: " << adapted->get_name() << "\n";
		use_service(adapted);
	}

	// 2. Wrap a unique_ptr
	std::cout << "\n2. Unique pointer adapter:\n";
	{
		auto service = std::make_unique<DataService>("UniqueService");
		auto adapted = adapter(std::move(service));

		std::cout << "   Name via ->: " << adapted->get_name() << "\n";
		std::cout << "   Raw ptr: " << (adapted.get() != nullptr ? "valid" : "null") << "\n";
	}

	// 3. Use make_shared_adapter helper
	std::cout << "\n3. make_shared_adapter helper:\n";
	{
		auto adapted = make_shared_adapter<DataService>("FactoryService");
		std::cout << "   Name: " << adapted->get_name() << "\n";
		adapted->process();
	}

	// 4. Adapter traits inspection
	std::cout << "\n4. Adapter traits:\n";
	{
		using shared_traits = adapter_traits<std::shared_ptr<DataService>>;
		using unique_traits = adapter_traits<std::unique_ptr<DataService>>;

		std::cout << "   shared_ptr is_smart_pointer: "
				  << (shared_traits::is_smart_pointer ? "true" : "false") << "\n";
		std::cout << "   shared_ptr supports_weak: "
				  << (shared_traits::supports_weak ? "true" : "false") << "\n";
		std::cout << "   unique_ptr is_smart_pointer: "
				  << (unique_traits::is_smart_pointer ? "true" : "false") << "\n";
		std::cout << "   unique_ptr supports_weak: "
				  << (unique_traits::supports_weak ? "true" : "false") << "\n";
	}

	std::cout << "\nDone.\n";
	return 0;
}
