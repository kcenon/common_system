# CLI Config Parser (`cli_config_parser.h`)

The `cli_config_parser` provides command-line argument parsing with configuration override support.

---

## Table of Contents

- [Argument Format](#argument-format)
- [Boolean Flags](#boolean-flags)
- [List Arguments](#list-arguments)
- [Help Text Generation](#help-text-generation)
- [Required vs Optional Arguments](#required-vs-optional-arguments)
- [Complete CLI Parsing Example](#complete-cli-parsing-example)

---

## Argument Format

Supported argument formats:

```bash
# Configuration file
--config=config.yaml
--config config.yaml

# Configuration overrides
--set logger.level=debug
--set key=value

# Help and version
--help
-h
--version
-v
```

**Key-value format**: `--set` accepts `key=value` format where:
- `key` is a dot-separated configuration path (e.g., `logger.level`)
- `value` is the value to set

## Boolean Flags

Boolean values can be specified in multiple formats:

```bash
# True values
--set logger.async=true
--set logger.async=1
--set logger.async=yes
--set logger.async=on

# False values
--set logger.async=false
--set logger.async=0
--set logger.async=no
--set logger.async=off
```

## List Arguments

List values (e.g., `logger.writers`) are specified as comma-separated values via environment variables:

```bash
# Via environment variable
export UNIFIED_LOGGER_WRITERS="console,file,json"

# In YAML file
logger:
  writers:
    - console
    - file
    - json
```

**CLI limitation**: The `--set` flag does not support list values directly. Use environment variables or YAML for lists.

## Help Text Generation

The parser automatically generates help text from configuration metadata:

```cpp
#include <kcenon/common/config/cli_config_parser.h>

using namespace kcenon::common::config;

// Print help message
cli_config_parser::print_help("myapp");
```

**Output**:

```
Usage: myapp [OPTIONS]

Options:
  --config=<path>     Load configuration from YAML file
  --set <key>=<value> Override a configuration value
  --help, -h          Show this help message
  --version, -v       Show version information

Configuration keys:
  thread.pool_size
    Number of worker threads (0 for auto) [env: UNIFIED_THREAD_POOL_SIZE]
  logger.level (trace|debug|info|warn|error|critical|off)
    Log level [env: UNIFIED_LOGGER_LEVEL]
  ...

Examples:
  myapp --config=config.yaml
  myapp --set logger.level=debug
  myapp --config=config.yaml --set thread.pool_size=16
```

## Required vs Optional Arguments

All CLI arguments are **optional**:

- If `--config` is not provided, configuration is loaded from environment variables and defaults
- If `--set` overrides are not provided, configuration from file/environment is used

**Positional arguments** are collected in `parsed_args::positional_args`:

```cpp
auto parse_result = cli_config_parser::parse(argc, argv);
if (parse_result.is_ok()) {
    auto args = parse_result.value();

    for (const auto& arg : args.positional_args) {
        std::cout << "Positional arg: " << arg << "\n";
    }
}
```

## Complete CLI Parsing Example

```cpp
#include <kcenon/common/config/cli_config_parser.h>

int main(int argc, char** argv) {
    using namespace kcenon::common::config;

    // Load configuration with CLI overrides
    auto result = cli_config_parser::load_with_cli_overrides(argc, argv);

    if (result.is_err()) {
        // Check for help/version requests
        if (result.error().message == "help_requested") {
            cli_config_parser::print_help(argv[0]);
            return 0;
        } else if (result.error().message == "version_requested") {
            cli_config_parser::print_version("1.0.0");
            return 0;
        }

        // Actual error
        std::cerr << "Configuration error: "
                  << result.error().message << "\n";
        return 1;
    }

    auto config = result.value();

    // Use configuration
    std::cout << "Log level: " << config.logger.level << "\n";
    std::cout << "Thread pool size: " << config.thread.pool_size << "\n";

    return 0;
}
```

---

## Related Documentation

- [Configuration Subsystem Guide](CONFIG_UNIFIED.md) — Overview, schema, and environment variables
- [Config Watcher](CONFIG_WATCHER.md) — Hot-reload file watching and patterns
- [Config Loader](CONFIG_LOADER.md) — YAML loading, validation, and troubleshooting

---

**Version**: 1.0.0
**Last Updated**: 2026-02-08
**License**: BSD 3-Clause (see LICENSE file)
