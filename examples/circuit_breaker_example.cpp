// BSD 3-Clause License
// Copyright (c) 2021-2025, 🍀☀🌕🌥 🌊
// See the LICENSE file in the project root for full license information.

/// @file circuit_breaker_example.cpp
/// @example circuit_breaker_example.cpp
/// @brief Demonstrates the circuit breaker resilience pattern.
///
/// Shows state transitions (CLOSED → OPEN → HALF_OPEN → CLOSED),
/// RAII guards, configuration, and stats reporting.
///
/// @see kcenon::common::circuit_breaker
/// @see kcenon::common::circuit_breaker_config

#include <kcenon/common/resilience/circuit_breaker.h>

#include <chrono>
#include <iostream>
#include <string>
#include <thread>

using namespace kcenon::common;
using namespace kcenon::common::resilience;

std::string state_name(circuit_state s)
{
	switch (s)
	{
	case circuit_state::CLOSED:
		return "CLOSED";
	case circuit_state::OPEN:
		return "OPEN";
	case circuit_state::HALF_OPEN:
		return "HALF_OPEN";
	default:
		return "UNKNOWN";
	}
}

// Simulated external service call
bool call_external_service(bool should_fail)
{
	if (should_fail)
	{
		throw std::runtime_error("Service unavailable");
	}
	return true;
}

int main()
{
	std::cout << "=== Circuit Breaker Example ===\n\n";

	// Configure: open after 3 failures, 1-second timeout
	circuit_breaker_config config;
	config.failure_threshold = 3;
	config.success_threshold = 2;
	config.timeout = std::chrono::seconds(1);

	circuit_breaker breaker(config);
	std::cout << "Initial state: " << state_name(breaker.get_state()) << "\n\n";

	// Phase 1: Simulate failures to trip the breaker
	std::cout << "1. Simulating failures...\n";
	for (int i = 0; i < 4; ++i)
	{
		if (!breaker.allow_request())
		{
			std::cout << "   Request " << i << ": REJECTED (circuit open)\n";
			continue;
		}

		try
		{
			auto guard = breaker.make_guard();
			call_external_service(true); // Will fail
			guard.record_success();
		}
		catch (const std::exception& e)
		{
			std::cout << "   Request " << i << ": FAILED (" << e.what() << ")\n";
		}

		std::cout << "   State: " << state_name(breaker.get_state()) << "\n";
	}

	// Phase 2: Wait for timeout and try again
	std::cout << "\n2. Waiting for timeout (1s)...\n";
	std::this_thread::sleep_for(std::chrono::milliseconds(1100));
	std::cout << "   State after timeout: " << state_name(breaker.get_state()) << "\n";

	// Phase 3: Recover with successful requests
	std::cout << "\n3. Recovering with successful requests...\n";
	for (int i = 0; i < 3; ++i)
	{
		if (!breaker.allow_request())
		{
			std::cout << "   Recovery " << i << ": REJECTED\n";
			continue;
		}

		auto guard = breaker.make_guard();
		call_external_service(false); // Success
		guard.record_success();
		std::cout << "   Recovery " << i << ": SUCCESS, state=" << state_name(breaker.get_state())
				  << "\n";
	}

	// Phase 4: Show stats
	std::cout << "\n4. Circuit breaker stats:\n";
	auto stats = breaker.get_stats();
	for (auto it = stats.begin(); it != stats.end(); ++it)
	{
		const auto& name = it->first;
		const auto& val = it->second;
		std::visit(
			[&name](const auto& v) { std::cout << "   " << name << ": " << v << "\n"; },
			val);
	}

	std::cout << "\nDone.\n";
	return 0;
}
