// BSD 3-Clause License
// Copyright (c) 2021-2025, 🍀☀🌕🌥 🌊
// See the LICENSE file in the project root for full license information.

/// @file service_container_example.cpp
/// @example service_container_example.cpp
/// @brief Demonstrates DI service container with singleton, transient, and
///        scoped lifetimes.
///
/// Shows service registration with factory functions, resolution, scoped
/// containers, and container freezing for production safety.
///
/// @see kcenon::common::service_container

#include <kcenon/common/di/service_container.h>
#include <kcenon/common/patterns/result.h>

#include <iostream>
#include <memory>
#include <string>

using namespace kcenon::common;
using namespace kcenon::common::di;

// --- Interfaces ---

class IGreeter
{
public:
	virtual ~IGreeter() = default;
	virtual std::string greet(const std::string& name) const = 0;
};

class ICounter
{
public:
	virtual ~ICounter() = default;
	virtual int next() = 0;
};

// --- Implementations ---

class FriendlyGreeter : public IGreeter
{
public:
	std::string greet(const std::string& name) const override
	{
		return "Hello, " + name + "! Welcome aboard.";
	}
};

class SequentialCounter : public ICounter
{
public:
	int next() override { return ++count_; }

private:
	int count_ = 0;
};

int main()
{
	std::cout << "=== Service Container Example ===\n\n";

	auto& container = service_container::global();
	container.clear();

	// Register a singleton greeter (one instance shared)
	std::cout << "1. Registering services...\n";
	container.register_factory<IGreeter>(
		[](IServiceContainer&) { return std::make_shared<FriendlyGreeter>(); },
		service_lifetime::singleton);

	// Register a transient counter (new instance each time)
	container.register_factory<ICounter>(
		[](IServiceContainer&) { return std::make_shared<SequentialCounter>(); },
		service_lifetime::transient);

	// Resolve services
	std::cout << "2. Resolving services...\n";
	auto greeter = container.resolve<IGreeter>();
	if (greeter.is_ok())
	{
		std::cout << "   " << greeter.value()->greet("Developer") << "\n";
	}

	// Singleton: same instance
	auto greeter2 = container.resolve<IGreeter>();
	std::cout << "3. Singleton check: same instance = "
			  << (greeter.value().get() == greeter2.value().get() ? "true" : "false")
			  << "\n";

	// Transient: different instances
	auto counter1 = container.resolve<ICounter>();
	auto counter2 = container.resolve<ICounter>();
	std::cout << "4. Transient check: counter1=" << counter1.value()->next()
			  << ", counter2=" << counter2.value()->next() << "\n";

	// Scoped container
	std::cout << "5. Creating scoped container...\n";
	auto scope = container.create_scope();
	if (scope)
	{
		std::cout << "   Scope created successfully\n";
	}

	// Freeze container (prevent further registrations)
	std::cout << "6. Freezing container for production...\n";
	container.freeze();
	std::cout << "   Frozen = " << (container.is_frozen() ? "true" : "false") << "\n";

	// List registered services
	auto services = container.registered_services();
	std::cout << "7. Registered services: " << services.size() << "\n";

	container.clear();
	std::cout << "\nDone.\n";
	return 0;
}
