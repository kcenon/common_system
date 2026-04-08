// BSD 3-Clause License
// Copyright (c) 2021-2025, 🍀☀🌕🌥 🌊
// See the LICENSE file in the project root for full license information.

/// @file event_bus_example.cpp
/// @example event_bus_example.cpp
/// @brief Demonstrates thread-safe pub/sub messaging with simple_event_bus.
///
/// Shows event publishing, subscription, unsubscription, priority ordering,
/// and error callbacks.
///
/// @see kcenon::common::simple_event_bus

#include <kcenon/common/patterns/event_bus.h>

#include <iostream>
#include <string>

using namespace kcenon::common;

// Custom event types
struct user_logged_in
{
	std::string username;
	std::string ip_address;
};

struct order_placed
{
	int order_id;
	double total;
};

struct system_alert
{
	std::string message;
	int severity;
};

int main()
{
	std::cout << "=== Event Bus Example ===\n\n";

	auto& bus = get_event_bus();

	// Subscribe to user login events
	std::cout << "1. Subscribing to events...\n";
	auto login_sub = bus.subscribe<user_logged_in>(
		[](const user_logged_in& e)
		{ std::cout << "   [Auth] User '" << e.username << "' logged in from " << e.ip_address << "\n"; });

	auto audit_sub = bus.subscribe<user_logged_in>(
		[](const user_logged_in& e)
		{ std::cout << "   [Audit] Login recorded for '" << e.username << "'\n"; });

	// Subscribe to order events
	auto order_sub = bus.subscribe<order_placed>(
		[](const order_placed& e)
		{ std::cout << "   [Orders] Order #" << e.order_id << " placed, total: $" << e.total << "\n"; });

	// Set error callback
	bus.set_error_callback([](const std::string& error, size_t, uint64_t)
						   { std::cerr << "   [Error] Event bus error: " << error << "\n"; });

	// Publish events
	std::cout << "\n2. Publishing events...\n";
	bus.publish(user_logged_in{"alice", "192.168.1.100"});
	bus.publish(order_placed{1001, 59.99});

	// Unsubscribe audit logger
	std::cout << "\n3. Unsubscribing audit logger...\n";
	bus.unsubscribe(audit_sub);

	// Publish again — only auth handler receives
	std::cout << "\n4. Publishing after unsubscribe...\n";
	bus.publish(user_logged_in{"bob", "10.0.0.1"});

	// Priority ordering
	std::cout << "\n5. Priority-ordered publishing...\n";
	auto alert_sub = bus.subscribe<system_alert>(
		[](const system_alert& e)
		{ std::cout << "   [Alert] severity=" << e.severity << ": " << e.message << "\n"; });

	bus.publish(system_alert{"CPU usage high", 2}, event_priority::high);
	bus.publish(system_alert{"Disk space low", 1}, event_priority::normal);

	// Cleanup
	bus.unsubscribe(login_sub);
	bus.unsubscribe(order_sub);
	bus.unsubscribe(alert_sub);

	std::cout << "\nDone.\n";
	return 0;
}
