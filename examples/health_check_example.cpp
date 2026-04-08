// BSD 3-Clause License
// Copyright (c) 2021-2025, 🍀☀🌕🌥 🌊
// See the LICENSE file in the project root for full license information.

/// @file health_check_example.cpp
/// @example health_check_example.cpp
/// @brief Demonstrates the health check interface for service monitoring.
///
/// Shows implementing custom health checks for liveness, readiness,
/// and dependency probes, with type conversion utilities.
///
/// @see kcenon::common::health_check
/// @see kcenon::common::health_check_type

#include <kcenon/common/interfaces/monitoring/health_check.h>
#include <kcenon/common/patterns/result.h>

#include <chrono>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

using namespace kcenon::common;
using namespace kcenon::common::interfaces;

// Custom liveness check — is the service running?
class app_liveness_check : public health_check
{
public:
	std::string get_name() const override { return "app_liveness"; }
	health_check_type get_type() const override { return health_check_type::liveness; }
	bool is_critical() const override { return true; }

	health_check_result check() override
	{
		health_check_result result;
		result.status = health_status::healthy;
		result.message = "Application is running";
		return result;
	}
};

// Custom readiness check — ready to accept traffic?
class cache_readiness_check : public health_check
{
public:
	explicit cache_readiness_check(bool warmed) : cache_warmed_(warmed) {}

	std::string get_name() const override { return "cache_readiness"; }
	health_check_type get_type() const override { return health_check_type::readiness; }
	bool is_critical() const override { return false; }

	std::chrono::milliseconds get_timeout() const override
	{
		return std::chrono::milliseconds(2000);
	}

	health_check_result check() override
	{
		health_check_result result;
		if (cache_warmed_)
		{
			result.status = health_status::healthy;
			result.message = "Cache is warmed";
		}
		else
		{
			result.status = health_status::degraded;
			result.message = "Cache warming in progress";
		}
		return result;
	}

private:
	bool cache_warmed_;
};

// Custom dependency check — external service available?
class database_dependency_check : public health_check
{
public:
	explicit database_dependency_check(bool connected) : connected_(connected) {}

	std::string get_name() const override { return "database"; }
	health_check_type get_type() const override { return health_check_type::dependency; }
	bool is_critical() const override { return true; }

	health_check_result check() override
	{
		health_check_result result;
		if (connected_)
		{
			result.status = health_status::healthy;
			result.message = "Database connected";
		}
		else
		{
			result.status = health_status::unhealthy;
			result.message = "Database connection failed";
		}
		return result;
	}

private:
	bool connected_;
};

std::string status_string(health_status status)
{
	switch (status)
	{
	case health_status::healthy:
		return "HEALTHY";
	case health_status::degraded:
		return "DEGRADED";
	case health_status::unhealthy:
		return "UNHEALTHY";
	default:
		return "UNKNOWN";
	}
}

int main()
{
	std::cout << "=== Health Check Example ===\n\n";

	// 1. Health check type utilities
	std::cout << "1. Health check types:\n";
	std::cout << "   liveness -> \"" << to_string(health_check_type::liveness) << "\"\n";
	std::cout << "   readiness -> \"" << to_string(health_check_type::readiness) << "\"\n";
	std::cout << "   dependency -> \"" << to_string(health_check_type::dependency) << "\"\n";

	auto parsed = health_check_type_from_string("readiness");
	if (parsed.is_ok())
	{
		std::cout << "   \"readiness\" -> " << to_string(parsed.value()) << "\n";
	}

	// 2. Create health checks
	std::cout << "\n2. Running health checks:\n";
	std::vector<std::unique_ptr<health_check>> checks;
	checks.push_back(std::make_unique<app_liveness_check>());
	checks.push_back(std::make_unique<cache_readiness_check>(true));
	checks.push_back(std::make_unique<database_dependency_check>(true));

	bool all_healthy = true;
	for (const auto& hc : checks)
	{
		auto result = hc->check();
		std::cout << "   [" << to_string(hc->get_type()) << "] " << hc->get_name() << ": "
				  << status_string(result.status) << " - " << result.message
				  << " (critical=" << (hc->is_critical() ? "yes" : "no")
				  << ", timeout=" << hc->get_timeout().count() << "ms)\n";

		if (result.status == health_status::unhealthy && hc->is_critical())
		{
			all_healthy = false;
		}
	}

	std::cout << "\n   Overall: " << (all_healthy ? "HEALTHY" : "UNHEALTHY") << "\n";

	// 3. Simulate a failure scenario
	std::cout << "\n3. Simulating database failure:\n";
	database_dependency_check db_down(false);
	auto result = db_down.check();
	std::cout << "   " << db_down.get_name() << ": " << status_string(result.status) << " - "
			  << result.message << "\n";

	std::cout << "\nDone.\n";
	return 0;
}
