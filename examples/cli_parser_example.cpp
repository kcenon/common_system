// BSD 3-Clause License
// Copyright (c) 2021-2025, 🍀☀🌕🌥 🌊
// See the LICENSE file in the project root for full license information.

/// @file cli_parser_example.cpp
/// @example cli_parser_example.cpp
/// @brief Demonstrates CLI argument parsing with cli_config_parser.
///
/// Shows parsing of --config, --set key=value overrides, --help,
/// --version flags, and integration with unified_config.
///
/// @see kcenon::common::cli_config_parser
/// @see kcenon::common::parsed_args

#include <kcenon/common/config/cli_config_parser.h>

#include <iostream>
#include <string>
#include <vector>

using namespace kcenon::common;
using namespace kcenon::common::config;

int main(int argc, char* argv[])
{
	std::cout << "=== CLI Config Parser Example ===\n\n";

	// If no arguments provided, demonstrate with simulated args
	if (argc <= 1)
	{
		std::cout << "1. No arguments provided. Demonstrating with simulated args...\n\n";

		// Simulate: program --config app.yaml --set thread.pool_size=8 --set logger.level=debug
		std::vector<const char*> sim_args
			= {"cli_parser_example",  "--config",
			   "app.yaml",			  "--set",
			   "thread.pool_size=8",  "--set",
			   "logger.level=debug",  "positional_arg"};

		auto result
			= cli_config_parser::parse(static_cast<int>(sim_args.size()), const_cast<char**>(sim_args.data()));

		if (result.is_err())
		{
			std::cerr << "Parse error: " << result.error() << "\n";
			return 1;
		}

		auto args = result.value();

		std::cout << "   Parsed results:\n";
		std::cout << "   Config path: " << (args.config_path.empty() ? "(none)" : args.config_path)
				  << "\n";
		std::cout << "   Show help: " << (args.show_help ? "true" : "false") << "\n";
		std::cout << "   Show version: " << (args.show_version ? "true" : "false") << "\n";

		std::cout << "\n   Overrides (" << args.overrides.size() << "):\n";
		for (const auto& [key, value] : args.overrides)
		{
			std::cout << "     " << key << " = " << value << "\n";
		}

		std::cout << "\n   Positional args (" << args.positional_args.size() << "):\n";
		for (const auto& arg : args.positional_args)
		{
			std::cout << "     " << arg << "\n";
		}
	}
	else
	{
		// Parse actual command-line arguments
		std::cout << "1. Parsing actual command-line arguments...\n\n";

		auto result = cli_config_parser::parse(argc, argv);
		if (result.is_err())
		{
			std::cerr << "Parse error: " << result.error() << "\n";
			cli_config_parser::print_help(argv[0]);
			return 1;
		}

		auto args = result.value();

		if (args.show_help)
		{
			cli_config_parser::print_help(argv[0]);
			return 0;
		}

		if (args.show_version)
		{
			cli_config_parser::print_version();
			return 0;
		}

		std::cout << "   Config path: " << (args.config_path.empty() ? "(none)" : args.config_path)
				  << "\n";
		std::cout << "   Overrides: " << args.overrides.size() << "\n";
		for (const auto& [key, value] : args.overrides)
		{
			std::cout << "     " << key << " = " << value << "\n";
		}
	}

	// Show help output format
	std::cout << "\n2. Help output:\n";
	cli_config_parser::print_help("my_application");

	std::cout << "\nDone.\n";
	return 0;
}
