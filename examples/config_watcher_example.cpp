// BSD 3-Clause License
// Copyright (c) 2021-2025, 🍀☀🌕🌥 🌊
// See the LICENSE file in the project root for full license information.

/// @file config_watcher_example.cpp
/// @example config_watcher_example.cpp
/// @brief Demonstrates configuration loading and hot-reload with config_watcher.
///
/// Shows unified_config structure, config_watcher file monitoring with
/// change callbacks, and config snapshot history.
///
/// @see kcenon::common::config_watcher
/// @see kcenon::common::unified_config

#include <kcenon/common/config/config_watcher.h>
#include <kcenon/common/config/unified_config.h>

#include <chrono>
#include <fstream>
#include <iostream>
#include <string>
#include <thread>

using namespace kcenon::common;
using namespace kcenon::common::config;

int main()
{
	std::cout << "=== Config Watcher Example ===\n\n";

	// Create a sample config file
	const std::string config_path = "example_config.yaml";
	{
		std::ofstream f(config_path);
		f << "thread:\n"
		  << "  pool_size: 4\n"
		  << "  queue_type: adaptive\n"
		  << "logger:\n"
		  << "  level: info\n"
		  << "monitoring:\n"
		  << "  enabled: true\n"
		  << "  metrics_interval: 30\n";
	}

	// Create watcher with history of 5 snapshots
	std::cout << "1. Creating config watcher for '" << config_path << "'...\n";
	config_watcher watcher(config_path, 5);

	// Register change callback
	watcher.on_change(
		[](const unified_config& old_cfg, const unified_config& new_cfg)
		{
			std::cout << "\n   [Change detected]\n";
			std::cout << "   Old pool_size: " << old_cfg.thread.pool_size << "\n";
			std::cout << "   New pool_size: " << new_cfg.thread.pool_size << "\n";
		});

	// Register error callback
	watcher.on_error([](const std::string& error)
					 { std::cerr << "   [Config error] " << error << "\n"; });

	// Start watching
	std::cout << "\n2. Starting file watcher...\n";
	auto start_result = watcher.start();
	if (start_result.is_ok())
	{
		std::cout << "   Watcher running = " << (watcher.is_running() ? "true" : "false") << "\n";
	}
	else
	{
		std::cout << "   Watcher start skipped (platform may not support file watching in this "
					 "environment)\n";
	}

	// Simulate a config change
	std::cout << "\n3. Modifying config file...\n";
	{
		std::ofstream f(config_path);
		f << "thread:\n"
		  << "  pool_size: 8\n"
		  << "  queue_type: lock_free\n"
		  << "logger:\n"
		  << "  level: debug\n"
		  << "monitoring:\n"
		  << "  enabled: true\n"
		  << "  metrics_interval: 10\n";
	}

	// Give the watcher time to detect the change
	std::this_thread::sleep_for(std::chrono::milliseconds(500));

	// Manual reload
	std::cout << "\n4. Manual reload...\n";
	auto reload_result = watcher.reload();
	std::cout << "   Reload: " << (reload_result.is_ok() ? "success" : "skipped") << "\n";

	// Stop watching
	std::cout << "\n5. Stopping watcher...\n";
	watcher.stop();
	std::cout << "   Watcher running = " << (watcher.is_running() ? "true" : "false") << "\n";

	// Cleanup
	std::remove(config_path.c_str());

	std::cout << "\nDone.\n";
	return 0;
}
